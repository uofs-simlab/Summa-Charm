#include "file_access_chare.hpp"
#include "FileAccessChare.decl.h"
#include "JobChare.decl.h"        // For CProxy_JobChare
#include "pup_stl.h"              // For STL serialization
#include "settings_functions.hpp" // For FileAccessActorSettings

FileAccessChare::FileAccessChare(NumGRUInfo num_gru_info,
                                 FileAccessActorSettings fa_settings,
                                 CkChareID job_chare_proxy)
    : CBase_FileAccessChare(), num_gru_info_(num_gru_info),
      fa_settings_(fa_settings), job_chare_proxy_(job_chare_proxy)
{

  CkPrintf("\n----------File_Access_Chare Started----------\n");

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
  CkPrintf("File Access Actor: Initializing\n");
  num_hru_ = num_hru;
  f_getNumTimeSteps(num_steps_);
  forcing_files_ = std::make_unique<forcingFileContainer>();

  if (forcing_files_->initForcingFiles() != 0)
    return -1;
  // Initialize output buffer
  output_buffer_ = std::make_unique<OutputBuffer>(
      fa_settings_, num_gru_info_, num_hru_, num_steps_);
  int chunk_return = output_buffer_->setChunkSize();
  CkPrintf("Chunk Size = %d\n", chunk_return);
  err = output_buffer_->defOutput(std::to_string(thishandle.onPE));

  // err = output_buffer_->defOutput("FileAccessChare");
  if (err != 0)
  {
    CkPrintf("File Access Actor: Error defOutput\n"
             "\tMessage = Can't define output file\n");
    return err;
  }
  err = output_buffer_->allocateOutputBuffer(num_steps_);

  timing_info_.updateEndPoint("init_duration");

  return num_steps_;
}

void FileAccessChare::accessForcing(int i_file, CkChareID gru_chare)
{
  if (forcing_files_->allFilesLoaded())
  {
    CProxy_GruChare(gru_chare).newForcingFile(forcing_files_->getNumSteps(i_file), i_file);
    return;
  }
  auto err = forcing_files_->loadForcingFile(i_file, start_gru_, num_gru_);
  if (err != 0)
  {
    CkPrintf("File Access Actor: Error loadForcingFile\n"
             "\tMessage = Can't load forcing file\n");
    CProxy_JobChare(job_chare_proxy_).handleGruChareError(0, 0, err, "Can't load forcing file\n");
    return;
  }

  // Load files behind the scenes
  accessForcingInternal(i_file + 1);
  CProxy_GruChare(gru_chare).newForcingFile(forcing_files_->getNumSteps(i_file), i_file);
}

void FileAccessChare::accessForcingInternal(int i_file)
{
  if (forcing_files_->allFilesLoaded())
    return;
  auto err = forcing_files_->loadForcingFile(i_file, start_gru_, num_gru_);
  if (err != 0)
  {
    CkPrintf("File Access Actor: Error loadForcingFile\n"
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

void FileAccessChare::writeOutput(int index_gru, CkChareID gru_chare)
{
  timing_info_.updateStartPoint("write_duration");

  auto update_status = output_buffer_->writeOutput(index_gru, gru_chare);

  // Do nothing if optional is emtpy
  if (!update_status.has_value())
  {
    timing_info_.updateEndPoint("write_duration");
    return;
  }

  // If error, send error message to parent
  if (update_status.value()->err != 0)
  {
    CkPrintf("File Access Actor: Error writeOutput\n"
             "\tMessage = %s\n",
             update_status.value()->message.c_str());
    CProxy_JobChare(job_chare_proxy_).handleGruChareError(0, 0, update_status.value()->err, update_status.value()->message);
    return;
  }

  for (auto gru : update_status.value()->actor_to_update)
  {
    CProxy_GruChare(gru).setNumStepsBeforeWrite(update_status.value()->num_steps_update);
    CProxy_GruChare(gru).runHRU();
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
  CkPrintf("File Access Actor: Restarting Failed GRUs\n");
  int sleep = output_buffer_->reconstruct();
  return sleep;
}

void FileAccessChare::runFailure(int index_gru_job)
{
  timing_info_.updateStartPoint("write_duration");
  auto update_status = output_buffer_->addFailedGRU(index_gru_job);

  if (!update_status.has_value())
  {
    timing_info_.updateEndPoint("write_duration");
    return;
  }

  if (update_status.value()->err != 0)
  {
    CkPrintf("File Access Actor: Error writeOutput\n"
             "\tMessage = %s\n",
             update_status.value()->message.c_str());
    CProxy_JobChare(job_chare_proxy_).handleGruChareError(0, 0, update_status.value()->err, update_status.value()->message);
    return;
  }

  for (auto gru : update_status.value()->actor_to_update)
  {
    CProxy_GruChare(gru).setNumStepsBeforeWrite(update_status.value()->num_steps_update);
    CProxy_GruChare(gru).runHRU();
  }

  timing_info_.updateEndPoint("write_duration");
}



std::tuple<double, double> FileAccessChare::finalize()
{
  CkPrintf("\n________________"
           "FILE_ACCESS_ACTOR TIMING INFO RESULTS________________\n"
           "Total Read Duration = %f\n"
           "Total Write Duration = %f\n"
           "\n__________________________________________________\n",
           forcing_files_->getReadDuration(),
           timing_info_.getDuration("write_duration").value_or(-1.0));

  // output_buffer_.reset();

  return std::make_tuple(forcing_files_->getReadDuration(),
                         timing_info_.getDuration("write_duration")
                             .value_or(-1.0));
}

void FileAccessChare::error(int err_code, std::string err_msg)
{
  CkPrintf("FileAccessChare: Error %d: %s\n", err_code, err_msg.c_str());
  // TODO: Implement error handling
}

#include "FileAccessChare.def.h"