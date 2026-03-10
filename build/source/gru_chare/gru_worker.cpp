#include "gru_worker.hpp"

#include "FileAccessChare.decl.h"
#include "JobChare.decl.h"

#include <algorithm>

GruWorker::GruWorker(int worker_id, int num_steps,
                     HRUChareSettings hru_chare_settings, int num_output_steps,
                     CkChareID file_access_chare, CkChareID parent)
    : worker_id_(worker_id), num_steps_(num_steps),
      num_steps_output_buffer_(num_output_steps),
      hru_chare_settings_(hru_chare_settings),
      file_access_chare_(file_access_chare), parent_(parent) {}

GruWorker::GruWorker(CkMigrateMessage *msg) {}

GruTaskState *GruWorker::getTask(int job_index)
{
  auto it = tasks_.find(job_index);
  if (it == tasks_.end())
  {
    return nullptr;
  }
  return &(it->second);
}

void GruWorker::cleanupTask(int job_index)
{
  queued_jobs_.erase(job_index);
  tasks_.erase(job_index);
}

void GruWorker::enqueueRunnable(int job_index)
{
  if (tasks_.find(job_index) == tasks_.end())
  {
    return;
  }
  if (queued_jobs_.insert(job_index).second)
  {
    runnable_jobs_.push_back(job_index);
  }
  if (!scheduler_active_)
  {
    thisProxy[thisIndex].runHRU();
  }
}

void GruWorker::maybeRequestMoreWork()
{
  if (work_request_pending_)
  {
    return;
  }
  work_request_pending_ = true;
  CProxy_JobChare(parent_).requestMoreWork(worker_id_);
}

void GruWorker::assignTask(int netcdf_index, int job_index,
                           ToleranceSettings tolerance_settings,
                           int dt_init_factor, bool default_tol)
{
  work_request_pending_ = false;
  cleanupTask(job_index);

  GruTaskState task;
  task.netcdf_index = netcdf_index;
  task.job_index = job_index;
  task.tolerance_settings = tolerance_settings;
  task.dt_init_factor = dt_init_factor;
  task.default_tol = default_tol;
  task.num_steps_until_write = num_steps_output_buffer_;

  int err = 0;
  f_getNumHruInGru(task.job_index, task.num_hrus);
  task.gru_data.reset(new_handle_gru_type(task.num_hrus));

  std::unique_ptr<char[]> message(new char[256]);
  f_initGru(task.job_index, task.gru_data.get(), num_steps_output_buffer_, err,
           &message);
  if (err != 0)
  {
    handleErr(task.job_index, task.timestep, err, message.get());
    return;
  }
  std::fill(message.get(), message.get() + 256, '\0');

  setupGRU_fortran(task.job_index, task.gru_data.get(), err, &message);
  if (err != 0)
  {
    handleErr(task.job_index, task.timestep, err, message.get());
    return;
  }
  std::fill(message.get(), message.get() + 256, '\0');

  readGRURestart_fortran(task.job_index, task.gru_data.get(), err, &message);
  if (err != 0)
  {
    handleErr(task.job_index, task.timestep, err, message.get());
    return;
  }

  f_setGruTolerances(
      task.gru_data.get(), task.tolerance_settings.be_steps_,
      task.tolerance_settings.rel_tol_temp_cas_,
      task.tolerance_settings.rel_tol_temp_veg_,
      task.tolerance_settings.rel_tol_wat_veg_,
      task.tolerance_settings.rel_tol_temp_soil_snow_,
      task.tolerance_settings.rel_tol_wat_snow_,
      task.tolerance_settings.rel_tol_matric_,
      task.tolerance_settings.rel_tol_aquifr_,
      task.tolerance_settings.abs_tol_temp_cas_,
      task.tolerance_settings.abs_tol_temp_veg_,
      task.tolerance_settings.abs_tol_wat_veg_,
      task.tolerance_settings.abs_tol_temp_soil_snow_,
      task.tolerance_settings.abs_tol_wat_snow_,
      task.tolerance_settings.abs_tol_matric_,
      task.tolerance_settings.abs_tol_aquifr_);

  task.waiting_for_forcing = true;
  task.waiting_for_output = false;
  tasks_.emplace(task.job_index, std::move(task));

  CProxy_FileAccessChare(file_access_chare_).accessForcing(1, job_index);
}

void GruWorker::newForcingFile(int job_index, int num_forc_steps, int iFile)
{
  GruTaskState *task = getTask(job_index);
  if (!task)
  {
    return;
  }

  int err = 0;
  std::unique_ptr<char[]> message(new char[256]);
  task->iFile = iFile;
  task->stepsInCurrentFFile = num_forc_steps;
  setTimeZoneOffsetGRU_fortran(task->iFile, task->gru_data.get(), err, &message);
  if (err != 0)
  {
    handleErr(task->job_index, task->timestep, err, message.get());
    return;
  }
  task->forcingStep = 1;
  task->waiting_for_forcing = false;
  enqueueRunnable(job_index);
}

void GruWorker::setNumStepsBeforeWrite(int job_index, int num_steps)
{
  std::vector<int> one_job(1, job_index);
  setNumStepsBeforeWriteBatch(one_job, num_steps);
}

void GruWorker::setNumStepsBeforeWriteBatch(std::vector<int> job_indices,
                                            int num_steps)
{
  bool runnable_added = false;
  for (int job_index : job_indices)
  {
    GruTaskState *task = getTask(job_index);
    if (!task)
    {
      continue;
    }
    task->num_steps_until_write = num_steps;
    task->output_step = 1;
    task->waiting_for_output = false;
    if (queued_jobs_.insert(job_index).second)
    {
      runnable_jobs_.push_back(job_index);
      runnable_added = true;
    }
  }

  if (runnable_added && !scheduler_active_)
  {
    thisProxy[thisIndex].runHRU();
  }
}

void GruWorker::runTaskSlice(GruTaskState &task)
{
  if (task.timestep > num_steps_)
  {
    const int finished_job = task.job_index;
    cleanupTask(finished_job);
    CProxy_JobChare(parent_).doneHRUJob(finished_job, worker_id_);
    return;
  }

  int err = 0;
  std::unique_ptr<char[]> message(new char[256]);

  while (task.num_steps_until_write > 0)
  {
    if (task.forcingStep > task.stepsInCurrentFFile)
    {
      task.waiting_for_forcing = true;
      CProxy_FileAccessChare(file_access_chare_).accessForcing(task.iFile + 1,
                                                               task.job_index);
      return;
    }

    task.num_steps_until_write--;
    if (hru_chare_settings_.print_output_ &&
        task.timestep % hru_chare_settings_.output_frequency_ == 0)
    {
      CkPrintf("GRU Worker %d running GRU %d: timestep=%d, forcingStep=%d, iFile=%d\n",
               worker_id_, task.job_index, task.timestep, task.forcingStep,
               task.iFile);
    }

    readGRUForcing_fortran(task.job_index, task.timestep, task.forcingStep,
                           task.iFile, task.gru_data.get(), err, &message);
    if (err != 0)
    {
      handleErr(task.job_index, task.timestep, err, message.get());
      return;
    }
    std::fill(message.get(), message.get() + 256, '\0');

    runGRU_fortran(task.job_index, task.timestep, task.gru_data.get(),
                   task.dt_init_factor, err, &message);
    if (err != 0)
    {
      handleErr(task.job_index, task.timestep, err, message.get());
      return;
    }
    std::fill(message.get(), message.get() + 256, '\0');

    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    writeGRUOutput_fortran(task.job_index, task.timestep, task.output_step,
                           task.gru_data.get(), err, &message, year, month, day,
                           hour);
    if (err != 0)
    {
      handleErr(task.job_index, task.timestep, err, message.get());
      return;
    }

    task.timestep++;
    task.forcingStep++;
    task.output_step++;
    if (task.timestep > num_steps_)
    {
      break;
    }
  }

  if (task.timestep > num_steps_)
  {
    const int finished_job = task.job_index;
    cleanupTask(finished_job);
    CProxy_JobChare(parent_).doneHRUJob(finished_job, worker_id_);
    return;
  }

  if (task.num_steps_until_write <= 0)
  {
    task.waiting_for_output = true;
    CProxy_FileAccessChare(file_access_chare_).writeOutput(task.job_index,
                                                            task.job_index);
  }
}

void GruWorker::runHRU()
{
  if (scheduler_active_)
  {
    return;
  }

  scheduler_active_ = true;
  while (!runnable_jobs_.empty())
  {
    const int job_index = runnable_jobs_.front();
    runnable_jobs_.pop_front();
    queued_jobs_.erase(job_index);

    GruTaskState *task = getTask(job_index);
    if (!task)
    {
      continue;
    }
    if (task->waiting_for_forcing || task->waiting_for_output)
    {
      continue;
    }
    runTaskSlice(*task);
  }
  scheduler_active_ = false;

  if (runnable_jobs_.empty())
  {
    maybeRequestMoreWork();
  }
}

void GruWorker::handleErr(int job_index, int timestep, int err,
                          const std::string &message)
{
  CkPrintf("GRU Worker %d: Error running GRU job_index=%d netcdf=%d at timestep %d\n",
           worker_id_, job_index, tasks_.count(job_index) ? tasks_[job_index].netcdf_index : -1,
           timestep);
  CProxy_JobChare(parent_).handleGruChareError(job_index, timestep, err, message);
  cleanupTask(job_index);
  maybeRequestMoreWork();
}
