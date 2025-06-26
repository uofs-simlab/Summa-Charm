#include "FileAccessChare.decl.h" // Include this first to resolve CBase_FileAccessChare
#include "GruChare.decl.h"        // Include this first to resolve CBase_GruChare
#include "job_chare.hpp"
#include "SummaChare.decl.h"
#include "gru_struc.hpp"

#include "summa_init_struc.hpp" // Re-enable the include
#include <limits.h>
#include <unistd.h>

JobChare::JobChare(Batch batch, bool enable_logging,
                   JobActorSettings job_actor_settings,
                   FileAccessActorSettings fa_actor_settings,
                   HRUActorSettings hru_actor_settings,
                   CkChareID summa_chare_proxy, int file_gru)
    : CBase_JobChare(), batch_(batch), file_gru_(file_gru),
      summa_chare_proxy_(summa_chare_proxy), enable_logging_(enable_logging),
      job_actor_settings_(job_actor_settings),
      fa_actor_settings_(fa_actor_settings),
      hru_actor_settings_(hru_actor_settings)
{
  std::string err_msg;
  CkPrintf("JobChare: Started");

  // Get hostname for logging
  gethostname(hostname_, HOST_NAME_MAX);

  // Timing Information
  timing_info_ = TimingInfo();
  timing_info_.addTimePoint("total_duration");
  timing_info_.updateStartPoint("total_duration");
  timing_info_.addTimePoint("init_duration");
  timing_info_.updateStartPoint("init_duration");

  // Create Loggers
  if (enable_logging_)
  {
    logger_ = std::make_unique<Logger>(batch_.getLogDir() + "batch_" +
                                       std::to_string(batch_.getBatchID()));
    err_logger_ = std::make_unique<ErrorLogger>(batch_.getLogDir());
    success_logger_ = std::make_unique<SuccessLogger>(batch_.getLogDir());
  }
  else
  {
    logger_ = std::make_unique<Logger>("");
    err_logger_ = std::make_unique<ErrorLogger>("");
    success_logger_ = std::make_unique<SuccessLogger>("");
  }

  // GruStruc Initialization
  gru_struc_ =
      std::make_unique<GruStruc>(batch_.getStartHRU(), batch_.getNumHRU(),
                                 job_actor_settings_.max_run_attempts_);
  if (gru_struc_->readDimension())
  {
    err_msg = "ERROR: Job_Actor - ReadDimension\n";
    CProxy_SummaChare(summa_chare_proxy_).reportError(-2, err_msg);
    return;
  }
  if (gru_struc_->readIcondNlayers())
  {
    err_msg = "ERROR: Job_Actor - ReadIcondNlayers\n";
    CProxy_SummaChare(summa_chare_proxy_).reportError(-2, err_msg);
    return;
  }
  gru_struc_->getNumHrusPerGru();

  // SummaInitStruc Initialization
  summa_init_struc_ = std::make_unique<SummaInitStruc>();
  if (summa_init_struc_->allocate(batch_.getNumHRU()) != 0)
  {
    err_msg = "ERROR -- Job_Actor: SummaInitStruc allocation failed\n";
    CProxy_SummaChare(summa_chare_proxy_).reportError(-2, err_msg);
    return;
  }
  if (summa_init_struc_->summa_paramSetup() != 0)
  {
    err_msg = "ERROR -- Job_Actor: SummaInitStruc paramSetup failed\n";
    CProxy_SummaChare(summa_chare_proxy_).reportError(-2, err_msg);
    return;
  }
  if (summa_init_struc_->summa_readRestart() != 0)
  {
    err_msg = "ERROR -- Job_Actor: SummaInitStruc readRestart failed\n";
    CProxy_SummaChare(summa_chare_proxy_).reportError(-2, err_msg);
    return;
  }
  summa_init_struc_->getInitTolerance(rel_tol_, abs_tol_);

  num_gru_info_ =
      NumGRUInfo(batch_.getStartHRU(), batch_.getStartHRU(), batch_.getNumHRU(),
                 batch_.getNumHRU(), gru_struc_->getFileGru(), false);

  // Set the file_access_actor settings depending on data assimilation mode
  if (job_actor_settings_.data_assimilation_mode_)
  {
    fa_actor_settings_.num_partitions_in_output_buffer_ = 1;
    fa_actor_settings_.num_timesteps_in_output_buffer_ = 2;
  }

  // Start File Access Actor and Become User Selected Mode

  file_access_chare_ = CProxy_FileAccessChare::ckNew(
      num_gru_info_, fa_actor_settings_, thishandle);

  int num_timesteps = file_access_chare_.initFileAccessChare(file_gru_, batch_.getNumHRU());

  // int num_timesteps = 10;
  if (num_timesteps < 0)
  {
    std::string err_msg =
        "ERROR: JobChare: FileAccessChare initialization failed\n";
    CkPrintf("JobChare: %s", err_msg.c_str());
    // this->handleError(-2, err_msg);
    CProxy_SummaChare(summa_chare_proxy_).reportError(-2, err_msg);
    return;
  }

  timing_info_.updateEndPoint("init_duration");

  // Start JobActor in User Selected Mode
  logger_->log("JobActor Initialized");
  CkPrintf("JobActor Initialized: Running %d Steps\n", num_timesteps);
  logger_->log("Async Mode: File Access Actor Ready");

  // TODO: Implement the data assimilation mode logic if needed
  num_steps_ = num_timesteps;
  spawnGruActors();
}

// ------------------------ Member Functions ------------------------
void JobChare::spawnGruActors()
{
  CkPrintf("JobChare: Spawning GRU Actors\n");
  // TODO: Implement f_getRelTol and f_getAbsTol
  if (hru_actor_settings_.rel_tol_ > 0)
  {
    // f_getRelTol();
    rel_tol_ = hru_actor_settings_.rel_tol_;
  }

  if (hru_actor_settings_.abs_tol_ > 0)
  {
    // f_getAbsTol();
    abs_tol_ = hru_actor_settings_.abs_tol_;
  }

  CkChareID fileAccessChareID = file_access_chare_.ckGetChareID();

  int num_gru = gru_struc_->getNumGru();
  CkPrintf("JobChare: About to create %d GRU actors\n", num_gru);

  for (int i = 0; i < num_gru; i++)
  {
    auto netcdf_index = gru_struc_->getStartGru() + i;
    auto job_index = i + 1;

    CProxy_GruChare gru_chare_proxy =
        CProxy_GruChare::ckNew(netcdf_index, job_index, num_steps_, hru_actor_settings_,
                               fa_actor_settings_.num_timesteps_in_output_buffer_, fileAccessChareID, thishandle);
    std::unique_ptr<GRU> gru_obj = std::make_unique<GRU>(
        netcdf_index, job_index, dt_init_factor_, rel_tol_,
        abs_tol_, job_actor_settings_.max_run_attempts_);
    gru_struc_->addGRU(std::move(gru_obj));
  }
  gru_struc_->decrementRetryAttempts();
}

// Entry method implementation for processGRU
void JobChare::processGRU(int gru_id)
{
  counter_++;
  if (counter_ == gru_struc_->getNumGru())
    finalizeJob(); // Finalize if all GRUs are processed
  // TODO: Implement GRU processing logic
}

// Entry method implementation for finalize
void JobChare::finalize()
{
  CkPrintf("JobChare: Entry method finalize() called\n");
  finalizeJob(); // Call the implementation method
}

// Entry method implementation for fileAccessReady
void JobChare::fileAccessReady(int num_steps)
{
  CkPrintf("JobChare: File access ready with %d steps\n", num_steps);
  // TODO: Implement file access ready logic
}

// Implementation method for finalization
void JobChare::finalizeJob()
{
  timing_info_.updateEndPoint("total_duration");

  CkPrintf("JobChare: Finalizing...\n");

  double total_duration =
      timing_info_.getDuration("total_duration").value_or(-1.0);
  double init_duration =
      timing_info_.getDuration("init_duration").value_or(-1.0);

  CkPrintf("JobChare: Timing results:\n");
  CkPrintf("  Total Duration: %f seconds\n", total_duration);
  CkPrintf("  Init Duration: %f seconds\n", init_duration);

  CkPrintf("JobChare: Finalization completed\n");

  // Notify the SummaChare that we're done
  CkPrintf("JobChare: Notifying SummaChare of completion...\n");

  CProxy_SummaChare summa_chare(summa_chare_proxy_);
  summa_chare.doneJob(0, total_duration, init_duration,
                      0.0); // Placeholder values
}

void JobChare::doneHRU(int job_index)
{
  handleFinishedGRU(job_index);
}

void JobChare::handleFinishedGRU(int job_index)
{
  gru_struc_->incrementNumGruDone();
  gru_struc_->getGRU(job_index)->setSuccess();
  success_logger_->logSuccess(gru_struc_->getGRU(job_index)->getIndexNetcdf(),
                              gru_struc_->getGRU(job_index)->getIndexJob(),
                              rel_tol_, abs_tol_);
  std::string update_str =
      "GRU Finished: " + std::to_string(gru_struc_->getNumGruDone()) + "/" +
      std::to_string(gru_struc_->getNumGru()) + " -- GlobalGRU=" +
      std::to_string(gru_struc_->getGRU(job_index)->getIndexNetcdf()) +
      " -- LocalGRU=" +
      std::to_string(gru_struc_->getGRU(job_index)->getIndexJob()) +
      " -- NumFailed=" + std::to_string(gru_struc_->getNumGruFailed());
  logger_->log(update_str);
  CkPrintf("%s", update_str.c_str());

  if (gru_struc_->isDone())
  {
    gru_struc_->hasFailures() && gru_struc_->shouldRetry() ? restartFailures() : finalizeJob();
  }
}

void JobChare::restartFailures()
{
  logger_->log("Async Mode: Restarting Failed GRUs");
  CkPrintf("Async Mode: Restarting Failed GRUs\n");
  if (rel_tol_ > 0 && abs_tol_ > 0)
  {
    rel_tol_ /= 10;
    hru_actor_settings_.rel_tol_ = rel_tol_;
    abs_tol_ /= 10;
    hru_actor_settings_.abs_tol_ = abs_tol_;
  }
  else
  {
    dt_init_factor_ *= 2;
  }

  // notify file_access_actor
  // TODO: Make it sync type in .ci so it waits untill finishing reconstruct()
  int sleep = file_access_chare_.restartFailures();

  err_logger_->nextAttempt();
  success_logger_->nextAttempt();

  while (gru_struc_->getNumGruFailed() > 0)
  {
    int job_index = gru_struc_->getFailedIndex();
    logger_->log("Async Mode: Restarting GRU: " +
                 std::to_string(job_index));
    CkPrintf("Async Mode: Restarting GRU: %s", std::to_string(job_index).c_str());
    int netcdf_index = job_index + gru_struc_->getStartGru() - 1;
    CProxy_GruChare gru_chare_proxy =
        CProxy_GruChare::ckNew(netcdf_index, job_index, num_steps_, hru_actor_settings_,
                               fa_actor_settings_.num_timesteps_in_output_buffer_, file_access_chare_, thishandle);
    gru_struc_->decrementNumGruFailed();
    std::unique_ptr<GRU> gru_obj = std::make_unique<GRU>(
        netcdf_index, job_index, dt_init_factor_, rel_tol_,
        abs_tol_, job_actor_settings_.max_run_attempts_);
    gru_struc_->addGRU(std::move(gru_obj));
    gru_chare_proxy.updateHRU();
  }
  gru_struc_->decrementRetryAttempts();
}

// void JobChare::pup(PUP::er &p) {
//   CBase_JobChare::pup(p);

//   // Serialize basic types only for now
//   p | batch_;
//   p | file_gru_;
//   p | enable_logging_;
//   p | num_steps_;
//   p | timestep_;
//   p | rel_tol_;
//   p | abs_tol_;
//   p | dt_init_factor_;
//   p | forcing_step_;
//   p | output_step_;
//   p | num_gru_done_timestep_;
//   p | num_write_msgs_;
//   p | da_paused_;
//   p | iFile_;
//   p | steps_in_ffile_;

//   // Note: Settings objects, timing_info_, gru_struc_, and other complex objects
//   // need special handling for migration - commented out for now
//   // p | job_actor_settings_;
//   // p | fa_actor_settings_;
//   // p | hru_actor_settings_;
//   // p | timing_info_;
// }

// Implementation of handleError method
void JobChare::handleError(int err_code, std::string err_msg)
{
  CkPrintf("JobChare::handleError: Code %d, Message: %s\n", err_code, err_msg.c_str());
  // Report error back to SummaChare
  CProxy_SummaChare(summa_chare_proxy_).reportError(err_code, err_msg);
}

void JobChare::handleGruChareError(int job_index, int timestep, int err_code,
                                   std::string err_msg)
{
  (job_index == 0) ? handleFileAccessError(err_code, err_msg) : handleGRUError(err_code, job_index, timestep, err_msg);
}

// ------------------------ERROR HANDLING FUNCTIONS ------------------------
void JobChare::handleGRUError(int err_code, int job_index, int timestep,
                              std::string err_msg)
{
  gru_struc_->getGRU(job_index)->setFailed();
  gru_struc_->incrementNumGruFailed();
  file_access_chare_.runFailure(job_index);
  if (gru_struc_->isDone())
  {
    gru_struc_->hasFailures() && gru_struc_->shouldRetry() ? restartFailures() : finalizeJob();
  }
}

void JobChare::handleFileAccessError(int err_code, std::string err_msg)
{
  logger_->log("JobActor: File_Access_Actor Error:" + err_msg);
  CkPrintf("JobActor: File_Access_Actor Error: %s\n", err_msg.c_str());
  if (err_code != -1)
  {
    logger_->log("JobActor: Have to Quit");
    CkPrintf("JobActor: Have to Quit");
    return;
  }
}

#include "JobChare.def.h"