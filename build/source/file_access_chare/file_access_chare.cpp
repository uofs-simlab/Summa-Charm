#include "file_access_chare.hpp"
#include "FileAccessChare.decl.h"
#include "GruWorker.decl.h"       // For direct CProxy_GruWorker routing
#include "JobChare.decl.h"        // For CProxy_JobChare (fallback path)
#include "pup_stl.h"              // For STL serialization
#include "settings_functions.hpp" // For FileAccessChareSettings

FileAccessChare::FileAccessChare(NumGRUInfo num_gru_info,
                                 FileAccessChareSettings fa_settings,
                                 CkChareID job_chare_proxy)
    : CBase_FileAccessChare(), num_gru_info_(num_gru_info),
      fa_settings_(fa_settings), job_chare_proxy_(job_chare_proxy)
{
  // Timing Info
  timing_info_ = TimingInfo();
  timing_info_.addTimePoint("write_duration");

  // Set GRU info based on settings
  if (num_gru_info_.use_global_for_data_structures)
  {
    start_gru_ = num_gru_info_.start_gru_global;
    num_gru_ = num_gru_info_.num_gru_global;
  }
  else
  {
    start_gru_ = num_gru_info_.start_gru_local;
    num_gru_ = num_gru_info_.num_gru_local;
  }
}

int FileAccessChare::initFileAccessChare(int file_gru, int num_hru)
{
  int err = 0;
  num_hru_ = num_hru;
  f_getNumTimeSteps(num_steps_);
  forcing_files_ = std::make_unique<forcingFileContainer>();

  if (forcing_files_->initForcingFiles() != 0)
    return -1;
  // Initialize output buffer
  output_buffer_ = std::make_unique<OutputBuffer>(
      fa_settings_, num_gru_info_, num_hru_, num_steps_);
  int chunk_return = output_buffer_->setChunkSize();
  err = output_buffer_->defOutput(std::to_string(thishandle.onPE));

  // err = output_buffer_->defOutput("FileAccessChare");
  if (err != 0)
  {
    CkPrintf("File Access Chare: Error defOutput\n"
             "\tMessage = Can't define output file\n");
    return err;
  }
  err = output_buffer_->allocateOutputBuffer(num_steps_);

  timing_info_.updateEndPoint("init_duration");

  return num_steps_;
}

void FileAccessChare::setGruWorkerProxy(CkArrayID gru_worker_id,
                                        int num_jobs, int num_workers)
{
  gru_worker_array_id_ = gru_worker_id;
  gru_worker_initialized_ = true;
  num_workers_in_pool_ = num_workers;
  job_to_worker_.assign(static_cast<size_t>(num_jobs), -1);
}

void FileAccessChare::updateJobWorkerMapping(int job_index, int worker_id)
{
  if (job_index >= 0 &&
      static_cast<size_t>(job_index) < job_to_worker_.size())
  {
    job_to_worker_[static_cast<size_t>(job_index)] = worker_id;
  }
}

void FileAccessChare::accessForcing(int i_file, int gru_job_index)
{
  // Helper: send newForcingFile directly to the owning GruWorker when the
  // mapping is known; fall back to JobChare forwarding otherwise.
  auto notify_forcing = [&](int steps, int file_idx) {
    const int wid =
        (gru_job_index >= 0 &&
         static_cast<size_t>(gru_job_index) < job_to_worker_.size())
            ? job_to_worker_[static_cast<size_t>(gru_job_index)]
            : -1;
    if (gru_worker_initialized_ && wid >= 0) {
      CProxy_GruWorker(gru_worker_array_id_)[wid].newForcingFile(
          gru_job_index, steps, file_idx);
    } else {
      CProxy_JobChare(job_chare_proxy_)
          .forwardNewForcingFile(gru_job_index, steps, file_idx);
    }
  };

  if (forcing_files_->allFilesLoaded())
  {
    notify_forcing(forcing_files_->getNumSteps(i_file), i_file);
    return;
  }
  auto err = forcing_files_->loadForcingFile(i_file, start_gru_, num_gru_);
  if (err != 0)
  {
    CkPrintf("File Access Chare: Error loadForcingFile\n"
             "\tMessage = Can't load forcing file\n");
    CProxy_JobChare(job_chare_proxy_).handleGruChareError(0, 0, err, "Can't load forcing file\n");
    return;
  }

  // Load files behind the scenes
  accessForcingInternal(i_file + 1);
  notify_forcing(forcing_files_->getNumSteps(i_file), i_file);
}

void FileAccessChare::accessForcingInternal(int i_file)
{
  if (forcing_files_->allFilesLoaded())
    return;
  auto err = forcing_files_->loadForcingFile(i_file, start_gru_, num_gru_);
  if (err != 0)
  {
    CkPrintf("File Access Chare: Error loadForcingFile\n"
             "\tMessage = Can't load forcing file\n");
    CProxy_JobChare(job_chare_proxy_).handleGruChareError(0, 0, err, "Can't load forcing file\n");
    return;
  }
  accessForcingInternal(i_file + 1);
}

int FileAccessChare::getNumOutputSteps(int job_index)
{
  return output_buffer_->getNumStepsBuffer(job_index);
}

void FileAccessChare::writeOutput(int index_gru, int gru_job_index)
{
  write_output_calls_++;
  timing_info_.updateStartPoint("write_duration");

  auto update_status = output_buffer_->writeOutput(index_gru, gru_job_index);

  // Do nothing if optional is emtpy
  if (!update_status.has_value())
  {
    timing_info_.updateEndPoint("write_duration");
    return;
  }

  // If error, send error message to parent
  if (update_status.value()->err != 0)
  {
    CkPrintf("File Access Chare: Error writeOutput\n"
             "\tMessage = %s\n",
             update_status.value()->message.c_str());
    CProxy_JobChare(job_chare_proxy_).handleGruChareError(0, 0, update_status.value()->err, update_status.value()->message);
    return;
  }

  write_flushes_++;
  write_resume_batches_++;
  write_resume_jobs_ +=
      static_cast<int>(update_status.value()->job_indices_to_update.size());

  // Route resume messages directly to GruWorkers, grouped by worker_id.
  if (gru_worker_initialized_ && num_workers_in_pool_ > 0) {
    std::vector<std::vector<int>> jobs_by_worker(
        static_cast<size_t>(num_workers_in_pool_));
    std::vector<int> fallback_jobs;
    for (int ji : update_status.value()->job_indices_to_update) {
      const int wid =
          (ji >= 0 && static_cast<size_t>(ji) < job_to_worker_.size())
              ? job_to_worker_[static_cast<size_t>(ji)]
              : -1;
      if (wid >= 0 && wid < num_workers_in_pool_)
        jobs_by_worker[static_cast<size_t>(wid)].push_back(ji);
      else
        fallback_jobs.push_back(ji);
    }
    for (int w = 0; w < num_workers_in_pool_; ++w) {
      if (!jobs_by_worker[static_cast<size_t>(w)].empty())
        CProxy_GruWorker(gru_worker_array_id_)[w].setNumStepsBeforeWriteBatch(
            jobs_by_worker[static_cast<size_t>(w)],
            update_status.value()->num_steps_update);
    }
    if (!fallback_jobs.empty())
      CProxy_JobChare(job_chare_proxy_)
          .forwardSetNumStepsBeforeWriteBatch(
              fallback_jobs, update_status.value()->num_steps_update);
  } else {
    CProxy_JobChare(job_chare_proxy_)
        .forwardSetNumStepsBeforeWriteBatch(
            update_status.value()->job_indices_to_update,
            update_status.value()->num_steps_update);
  }

  timing_info_.updateEndPoint("write_duration");
}

void FileAccessChare::writeRestartOutput(int gru, int gru_timestep, int gru_checkpoint,
                                         int output_structure_index, int year, int month, int day, int hour)
{
  // update hru progress vecs
  int gru_index = abs(start_gru_ - gru);
  hru_timesteps_[gru_index] = gru_timestep;
  hru_checkpoints_[gru_index] = gru_checkpoint;
  // find slowest time step of all hrus in job, stored hru_timesteps_
  int slowest_timestep = *std::min_element(
      hru_timesteps_.begin(), hru_timesteps_.end());
  int slowest_checkpoint = *std::min_element(
      hru_checkpoints_.begin(), hru_checkpoints_.end());

  // if the slowest hru is past the ith checkpoint (current threshold)
  if (slowest_checkpoint >= completed_checkpoints_)
  {
    // Output_Partition *output_partition =  output_container_->getOutputPartition(gru - 1);
    // writeRestart(output_partition, start_gru_, num_gru_,
    //              output_structure_index, year, month, day, hour);
    completed_checkpoints_++;
  }
}

int FileAccessChare::restartFailures()
{
  CkPrintf("File Access Chare: Restarting Failed GRUs\n");
  int sleep = output_buffer_->reconstruct();
  return sleep;
}

void FileAccessChare::runFailure(int index_gru_job)
{
  write_output_calls_++;
  timing_info_.updateStartPoint("write_duration");
  auto update_status = output_buffer_->addFailedGRU(index_gru_job);

  if (!update_status.has_value())
  {
    timing_info_.updateEndPoint("write_duration");
    return;
  }

  if (update_status.value()->err != 0)
  {
    CkPrintf("File Access Chare: Error writeOutput\n"
             "\tMessage = %s\n",
             update_status.value()->message.c_str());
    CProxy_JobChare(job_chare_proxy_).handleGruChareError(0, 0, update_status.value()->err, update_status.value()->message);
    return;
  }

  write_flushes_++;
  write_resume_batches_++;
  write_resume_jobs_ +=
      static_cast<int>(update_status.value()->job_indices_to_update.size());

  // Same direct routing as writeOutput
  if (gru_worker_initialized_ && num_workers_in_pool_ > 0) {
    std::vector<std::vector<int>> jobs_by_worker(
        static_cast<size_t>(num_workers_in_pool_));
    std::vector<int> fallback_jobs;
    for (int ji : update_status.value()->job_indices_to_update) {
      const int wid =
          (ji >= 0 && static_cast<size_t>(ji) < job_to_worker_.size())
              ? job_to_worker_[static_cast<size_t>(ji)]
              : -1;
      if (wid >= 0 && wid < num_workers_in_pool_)
        jobs_by_worker[static_cast<size_t>(wid)].push_back(ji);
      else
        fallback_jobs.push_back(ji);
    }
    for (int w = 0; w < num_workers_in_pool_; ++w) {
      if (!jobs_by_worker[static_cast<size_t>(w)].empty())
        CProxy_GruWorker(gru_worker_array_id_)[w].setNumStepsBeforeWriteBatch(
            jobs_by_worker[static_cast<size_t>(w)],
            update_status.value()->num_steps_update);
    }
    if (!fallback_jobs.empty())
      CProxy_JobChare(job_chare_proxy_)
          .forwardSetNumStepsBeforeWriteBatch(
              fallback_jobs, update_status.value()->num_steps_update);
  } else {
    CProxy_JobChare(job_chare_proxy_)
        .forwardSetNumStepsBeforeWriteBatch(
            update_status.value()->job_indices_to_update,
            update_status.value()->num_steps_update);
  }

  timing_info_.updateEndPoint("write_duration");
}

std::tuple<double, double> FileAccessChare::finalize()
{
  CkPrintf("\n________________"
           "FILE_ACCESS_CHARE TIMING INFO RESULTS________________\n"
           "Total Read Duration = %f\n"
           "Total Write Duration = %f\n"
           "Write Output Calls = %d\n"
           "Write Flushes = %d\n"
           "Resume Batches = %d\n"
           "Resume Jobs = %d\n"
           "\n__________________________________________________\n",
           forcing_files_->getReadDuration(),
           timing_info_.getDuration("write_duration").value_or(-1.0),
           write_output_calls_, write_flushes_, write_resume_batches_,
           write_resume_jobs_);

  output_buffer_.reset();

  return std::make_tuple(forcing_files_->getReadDuration(),
                         timing_info_.getDuration("write_duration")
                             .value_or(-1.0));
}

void FileAccessChare::error(int err_code, std::string err_msg)
{
  CkPrintf("FileAccessChare: Error %d: %s\n", err_code, err_msg.c_str());
  // TODO: Implement error handling
}
