#include "FileAccessChare.decl.h" // Include this first to resolve CBase_FileAccessChare
#include "GruChare.decl.h"        // Include this first to resolve CBase_GruChare
#include "job_chare.hpp"
#include "SummaChare.decl.h"
#include "gru_struc.hpp"

#include "summa_init_struc.hpp" // Re-enable the include
#include <limits.h>
#include <unistd.h>

JobChare::JobChare(Batch batch, bool enable_logging,
                   JobChareSettings job_chare_settings,
                   FileAccessChareSettings fa_chare_settings,
                   HRUChareSettings hru_chare_settings,
                   CkChareID summa_chare_proxy)
    : CBase_JobChare(), batch_(batch),
      summa_chare_proxy_(summa_chare_proxy), enable_logging_(enable_logging),
      job_chare_settings_(job_chare_settings),
      fa_chare_settings_(fa_chare_settings),
      hru_chare_settings_(hru_chare_settings)
{
  std::string err_msg;
  CkPrintf("JobChare: Started\n");

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
                                 job_chare_settings_.max_run_attempts_);
  if (gru_struc_->readDimension())
  {
    err_msg = "ERROR: Job_Chare - ReadDimension\n";
    CProxy_SummaChare(summa_chare_proxy_).reportError(-2, err_msg);
    return;
  }
  if (gru_struc_->readIcondNlayers())
  {
    err_msg = "ERROR: Job_Chare - ReadIcondNlayers\n";
    CProxy_SummaChare(summa_chare_proxy_).reportError(-2, err_msg);
    return;
  }
  gru_struc_->getNumHrusPerGru();

  // SummaInitStruc Initialization
  summa_init_struc_ = std::make_unique<SummaInitStruc>();
  if (summa_init_struc_->allocate(batch_.getNumHRU()) != 0)
  {
    err_msg = "ERROR -- Job_Chare: SummaInitStruc allocation failed\n";
    CProxy_SummaChare(summa_chare_proxy_).reportError(-2, err_msg);
    return;
  }
  if (summa_init_struc_->summa_paramSetup() != 0)
  {
    err_msg = "ERROR -- Job_Chare: SummaInitStruc paramSetup failed\n";
    CProxy_SummaChare(summa_chare_proxy_).reportError(-2, err_msg);
    return;
  }
  if (summa_init_struc_->summa_readRestart() != 0)
  {
    err_msg = "ERROR -- Job_Chare: SummaInitStruc readRestart failed\n";
    CProxy_SummaChare(summa_chare_proxy_).reportError(-2, err_msg);
    return;
  }
  summa_init_struc_->getInitTolerance(tolerance_settings_.rel_tol_temp_cas_, 
      tolerance_settings_.rel_tol_temp_veg_, tolerance_settings_.rel_tol_wat_veg_, 
      tolerance_settings_.rel_tol_temp_soil_snow_, tolerance_settings_.rel_tol_wat_snow_, 
      tolerance_settings_.rel_tol_matric_, tolerance_settings_.rel_tol_aquifr_, 
      tolerance_settings_.abs_tol_temp_cas_, tolerance_settings_.abs_tol_temp_veg_, 
      tolerance_settings_.abs_tol_wat_veg_, tolerance_settings_.abs_tol_temp_soil_snow_, 
      tolerance_settings_.abs_tol_wat_snow_, tolerance_settings_.abs_tol_matric_, 
      tolerance_settings_.abs_tol_aquifr_, default_tol_, tolerance_settings_.be_steps_);

  num_gru_info_ = NumGRUInfo(batch_.getStartHRU(), batch_.getStartHRU(), batch_.getNumHRU(),
                             batch_.getNumHRU(), gru_struc_->getFileGru(), false);



  // Start File Access Chare and Become User Selected Mode
  file_access_chare_ = CProxy_FileAccessChare::ckNew(num_gru_info_, fa_chare_settings_, thisProxy.ckGetChareID());

  int num_timesteps = file_access_chare_.initFileAccessChare(gru_struc_->getFileGru(), gru_struc_->getNumHru());
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

  // Start JobChare in User Selected Mode
  logger_->log("JobChare Initialized");
  CkPrintf("JobChare Initialized: Running %d Steps\n", num_timesteps);
  logger_->log("Async Mode: File Access Chare Ready");

  // TODO: Implement the data assimilation mode logic if needed
  num_steps_ = num_timesteps;
  spawnGruChares();
}

// ------------------------ Member Functions ------------------------
void JobChare::spawnGruChares()
{
  CkPrintf("JobChare: Spawning GRU Chares\n");
  // Tolerance values are already set in tolerance_settings_ - no need to copy them

  CkChareID fileAccessChareID = file_access_chare_.ckGetChareID();

  CkPrintf("JobChare: NumGRU = %d\n", gru_struc_->getNumGru());

  for (int i = 0; i < gru_struc_->getNumGru(); i++)
  {
    auto netcdf_index = gru_struc_->getStartGru() + i;
    auto job_index = i + 1;

    CProxy_GruChare gru_chare_proxy =
        CProxy_GruChare::ckNew(netcdf_index, job_index, num_steps_, hru_chare_settings_,
                               fa_chare_settings_.num_timesteps_in_output_buffer_, fileAccessChareID, thisProxy.ckGetChareID(), tolerance_settings_);
    std::unique_ptr<GRU> gru_obj = std::make_unique<GRU>(
        netcdf_index, job_index, gru_chare_proxy.ckGetChareID(), dt_init_factor_, tolerance_settings_,
        default_tol_, job_chare_settings_.max_run_attempts_);
    gru_struc_->addGRU(std::move(gru_obj));
    gru_chare_proxy.updateHRU();
  }
  gru_struc_->decrementRetryAttempts();
}

// Implementation method for finalization
void JobChare::finalizeJob()
{
  std::tuple<double, double> read_write_duration = file_access_chare_.finalize();
  CkPrintf("read_write_duration = (%f, %f)\n",
           std::get<0>(read_write_duration), std::get<1>(read_write_duration));
  int err = 0;
  int num_failed_grus = gru_struc_->getNumGruFailed();
  CkPrintf("JobChare: Finalizing job with %d failed GRUs\n", num_failed_grus);
  timing_info_.updateEndPoint("total_duration");
  CkPrintf(
      "\n_____________PRINTING JOB_CHARE TIMING INFO RESULTS____________\n"
  "Total Duration = %f Seconds\n"
  "Total Duration = %f Minutes\n"
  "Total Duration = %f Hours\n"
  "Job Init Duration = %f Seconds\n"
  "_________________________________________________________________\n\n",
  timing_info_.getDuration("total_duration").value_or(-1.0),
  timing_info_.getDuration("total_duration").value_or(-1.0) / 60,
  (timing_info_.getDuration("total_duration").value_or(-1.0) / 60) / 60,
  timing_info_.getDuration("init_duration").value_or(-1.0));


  // Deallocate GRU_Struc
  summa_init_struc_.reset();
  gru_struc_.reset(); 
  // Tell Parent we are done
  double total_duration = timing_info_.getDuration("total_duration").value_or(-1.0);
  double read_duration = std::get<0>(read_write_duration);
  double write_duration = std::get<1>(read_write_duration);
  CProxy_SummaChare(summa_chare_proxy_).doneJob(num_failed_grus, total_duration, read_duration, write_duration);

  CkPrintf("JobChare: Finalized successfully\n");
}

void JobChare::doneHRUJob(int job_index)
{
  handleFinishedGRU(job_index);
}

void JobChare::handleFinishedGRU(int job_index)
{
  gru_struc_->incrementNumGruDone();
  gru_struc_->getGRU(job_index)->setSuccess();
  success_logger_->logSuccess(gru_struc_->getGRU(job_index)->getIndexNetcdf(),
                              gru_struc_->getGRU(job_index)->getIndexJob(),
                              MISSING_DOUBLE, MISSING_DOUBLE,  // Using default rel_tol and abs_tol values
                              tolerance_settings_.rel_tol_temp_cas_, tolerance_settings_.rel_tol_temp_veg_,
                              tolerance_settings_.rel_tol_wat_veg_, tolerance_settings_.rel_tol_temp_soil_snow_,
                              tolerance_settings_.rel_tol_wat_snow_, tolerance_settings_.rel_tol_matric_,
                              tolerance_settings_.rel_tol_aquifr_, tolerance_settings_.abs_tol_temp_cas_,
                              tolerance_settings_.abs_tol_temp_veg_, tolerance_settings_.abs_tol_wat_veg_,
                              tolerance_settings_.abs_tol_temp_soil_snow_, tolerance_settings_.abs_tol_wat_snow_,
                              tolerance_settings_.abs_tol_matric_, tolerance_settings_.abs_tol_aquifr_, default_tol_);
  std::string update_str =
      "GRU Finished: " + std::to_string(gru_struc_->getNumGruDone()) + "/" +
      std::to_string(gru_struc_->getNumGru()) + " -- GlobalGRU=" +
      std::to_string(gru_struc_->getGRU(job_index)->getIndexNetcdf()) +
      " -- LocalGRU=" +
      std::to_string(gru_struc_->getGRU(job_index)->getIndexJob()) +
      " -- NumFailed=" + std::to_string(gru_struc_->getNumGruFailed()) + "\n";
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

  
      auto tighten_tol = [&](double& tol, const double& min_tol, const std::string& name){
        if (tol > min_tol) {
          tol /= 10;
          CkPrintf("Async Mode: Tightening tolerance\n");
          CkPrintf("Async Mode: %s = %f\n", name.c_str(), tol);
          return true;
        }
        return false;
      };

      // Update tolerances (general and specific)
      bool tol_updated = false;

      tolerance_settings_.be_steps_ = tolerance_settings_.be_steps_ * 2;
      CkPrintf("Async Mode: Tightening be steps: %d\n", tolerance_settings_.be_steps_);
      
      tol_updated |= tighten_tol(tolerance_settings_.rel_tol_temp_cas_, MIN_REL_TOL, "rel_tol_temp_cas_");

      tol_updated |= tighten_tol(tolerance_settings_.rel_tol_temp_veg_, MIN_REL_TOL, "rel_tol_temp_veg_");

      tol_updated |= tighten_tol(tolerance_settings_.rel_tol_wat_veg_, MIN_REL_TOL, "rel_tol_wat_veg_");

      tol_updated |= tighten_tol(tolerance_settings_.rel_tol_temp_soil_snow_, MIN_REL_TOL, "rel_tol_temp_soil_snow_");

      tol_updated |= tighten_tol(tolerance_settings_.rel_tol_wat_snow_, MIN_REL_TOL, "rel_tol_wat_snow_");

      tol_updated |= tighten_tol(tolerance_settings_.rel_tol_matric_, MIN_REL_TOL, "rel_tol_matric_");

      tol_updated |= tighten_tol(tolerance_settings_.rel_tol_aquifr_, MIN_REL_TOL, "rel_tol_aquifr_");

      tol_updated |= tighten_tol(tolerance_settings_.abs_tol_temp_cas_, MIN_ABS_TOL, "abs_tol_temp_cas_");

      tol_updated |= tighten_tol(tolerance_settings_.abs_tol_temp_veg_, MIN_ABS_TOL, "abs_tol_temp_veg_");

      tol_updated |= tighten_tol(tolerance_settings_.abs_tol_wat_veg_, MIN_ABS_TOL, "abs_tol_wat_veg_");

      tol_updated |= tighten_tol(tolerance_settings_.abs_tol_temp_soil_snow_, MIN_ABS_TOL, "abs_tol_temp_soil_snow_");

      tol_updated |= tighten_tol(tolerance_settings_.abs_tol_wat_snow_, MIN_ABS_TOL, "abs_tol_wat_snow_");

      tol_updated |= tighten_tol(tolerance_settings_.abs_tol_matric_, MIN_ABS_TOL, "abs_tol_matric_");

      tol_updated |= tighten_tol(tolerance_settings_.abs_tol_aquifr_, MIN_ABS_TOL, "abs_tol_aquifr_");

  // notify file_access_chare
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
    CkChareID fileAccessChareID = file_access_chare_.ckGetChareID();
    CProxy_GruChare gru_chare_proxy =
        CProxy_GruChare::ckNew(netcdf_index, job_index, num_steps_, hru_chare_settings_,
                               fa_chare_settings_.num_timesteps_in_output_buffer_, fileAccessChareID, thisProxy.ckGetChareID(), tolerance_settings_);
    gru_struc_->decrementNumGruFailed();
    std::unique_ptr<GRU> gru_obj = std::make_unique<GRU>(
        netcdf_index, job_index, gru_chare_proxy.ckGetChareID(), dt_init_factor_, tolerance_settings_,
        default_tol_, job_chare_settings_.max_run_attempts_);
    gru_struc_->addGRU(std::move(gru_obj));
    gru_chare_proxy.updateHRU();
  }
  gru_struc_->decrementRetryAttempts();
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
  logger_->log("JobChare: File_Access_Chare Error:" + err_msg);
  CkPrintf("JobChare: File_Access_Chare Error: %s\n", err_msg.c_str());
  if (err_code != -1)
  {
    logger_->log("JobChare: Have to Quit");
    CkPrintf("JobChare: Have to Quit");
    return;
  }
}

#include "JobChare.def.h"