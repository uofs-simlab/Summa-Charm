#pragma once

#include "pup_stl.h"
#include "JobChare.decl.h"
#include "FileAccessChare.decl.h"
#include "GruChare.decl.h"
#include "GruWorker.decl.h"
#include "file_access_chare_settings.hpp" // For FileAccessChareSettings
// #include "file_access_chare.hpp"
// #include "gru_batch_chare.hpp"
#include "gru_struc.hpp"
#include "hru_chare_settings.hpp" // For HruChareSettings
#include "job_chare_settings.hpp" // For JobChareSettings
#include "tolarance_settings.hpp" // For ToleranceSettings
#include "json.hpp"
#include "num_gru_info.hpp" // For NumGRUInfo
#include "timing_info.hpp"
#include "logger.hpp"
#include "summa_init_struc.hpp" // For SummaInitStruc
#include "gru_struc.hpp" // For GruStruc


#include <chrono>
#include <deque>
#include <memory>
#include <string>
#include <vector>

// For HOST_NAME_MAX
#include <limits.h>
#include <unistd.h>
#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 255
#endif

// Forward declaration for Batch - we'll check if this exists later
class Batch;

class JobChare : public CBase_JobChare {

private:
  int counter_ = 0; // Counter for the number of GRUs processed
  int num_gru_constructed_ = 0;
  int total_gru_to_construct_ = 0;
  CkChareID summa_chare_proxy_;
  CProxy_FileAccessChare file_access_chare_;
  CProxy_GruWorker gru_worker_array_;

  char hostname_[HOST_NAME_MAX];

  TimingInfo timing_info_;
  bool enable_logging_ = false;
  std::unique_ptr<Logger> logger_;
  std::unique_ptr<ErrorLogger> err_logger_;
  std::unique_ptr<SuccessLogger> success_logger_;

  Batch batch_;
  std::unique_ptr<GruStruc> gru_struc_;
  std::unique_ptr<SummaInitStruc> summa_init_struc_;
  NumGRUInfo num_gru_info_;

  // Settings
  JobChareSettings job_chare_settings_;
  FileAccessChareSettings fa_chare_settings_;
  HRUChareSettings hru_chare_settings_;
  ToleranceSettings tolerance_settings_;

   //Min rel and abs tol values
  const double MIN_REL_TOL = 1e-6;
  const double MIN_ABS_TOL = 1e-6;
  
  int dt_init_factor_ = 1;
  // Default tolerances flag
  bool default_tol_ = true;

  // Misc
  int num_steps_ = 0;
  int iFile_ = 1;
  int steps_in_ffile_ = 0;
  int forcing_step_ = 1;
  int timestep_ = 1;
  int num_gru_done_timestep_ = 0;
  int output_step_ = 1; // Index in the output structure
  int num_write_msgs_ = 0;
  bool da_paused_ = false;
  std::vector<std::chrono::time_point<std::chrono::steady_clock>>
      gru_start_times_;

  int num_workers_ = 0;
  int worker_prefetch_depth_effective_ = 1;
  std::vector<int> job_to_worker_;
  std::vector<int> worker_to_pe_;
  std::vector<int> inflight_tasks_per_worker_;
  std::deque<int> pending_jobs_global_;
  std::vector<int> assigned_grus_per_pe_;
  std::vector<int> completed_grus_per_pe_;
  std::string pe_distribution_csv_path_;
  bool pe_distribution_csv_ready_ = false;

  void logPeGruDistribution(const char *label,
                            const std::vector<int> &counts);
  void appendPeGruDistributionCsv(const char *label,
                                  const std::vector<int> &counts);
  void enqueueJob(int job_index);
  bool dequeueJobForWorker(int worker_id, int &job_index);
  bool assignNextTask(int worker_id, bool ignore_prefetch_cap = false);

public:
  // Simplified constructor - takes batch and the chare ID
  JobChare(Batch batch, bool enable_logging,
           JobChareSettings job_chare_settings,
           FileAccessChareSettings fa_chare_settings,
           HRUChareSettings hru_chare_settings,
           CkChareID summa_chare_proxy);

  void spawnGruChares();
  void notifyGruConstructed(int job_index);
  void doneHRUJob(int job_index, int worker_id);
  void requestMoreWork(int worker_id);
  void handleFinishedGRU(int job_index, int worker_id = -1);
  void finalizeJob();
  void restartFailures();
  void handleGruChareError(int job_index, int timestep, int err_code,
                              std::string err_msg);
  void handleGRUError(int err_code, int job_index, int timestep, 
                              std::string err_msg);
  void handleFileAccessError(int err_code, std::string err_msg);
  
  // Array communication helper methods
  void forwardNewForcingFile(int job_index, int num_forc_steps, int iFile);
  void forwardSetNumStepsBeforeWrite(int job_index, int num_steps);
  void forwardSetNumStepsBeforeWriteBatch(std::vector<int> job_indices,
                                          int num_steps);
};



/*********************************************
 * Job Chare Data Structures
 *********************************************/
// Holds information about the GRUs
struct GRU_Container {
  std::vector<GRU*> gru_list;
  std::chrono::time_point<std::chrono::system_clock> gru_start_time; // Vector of start times for each GRU
  int num_gru_done = 0; 
  int num_gru_failed = 0; // number of grus that are waiting to be restarted
  int num_gru_in_run_domain = 0; // number of grus we are currently solving for
  int run_attempts_left = 1; // current run attempt for all grus
};


/*********************************************
 * Job Chare state variables
 *********************************************/
struct job_state {
  TimingInfo job_timing;
  std::unique_ptr<Logger> logger;
  std::unique_ptr<ErrorLogger> err_logger;
  std::unique_ptr<SuccessLogger> success_logger;
  // Chare References
  CkChareID file_access_chare; // chare reference for the file_access_chare
  CkChareID summa_chare_proxy;            // chare reference to the top-level SummaChare

  Batch batch; // Information about the number of HRUs and starting point 

  // TODO: gru_struc can contain the num_gru_info and be the gru_container
  std::unique_ptr<GruStruc> gru_struc; 
  NumGRUInfo num_gru_info;
  GRU_Container gru_container;

  std::unique_ptr<SummaInitStruc> summa_init_struc;

  // Variables for GRU monitoring
  int dt_init_start_factor = 1; // Initial Fchare for dt_init (coupled_em)
  int num_gru_done = 0;         // The number of GRUs that have completed
  int num_gru_failed = 0;       // Number of GRUs that have failed

  
  std::string hostname;

  
  FileAccessChareSettings file_access_chare_settings;
  JobChareSettings job_chare_settings; 
  HRUChareSettings hru_chare_settings;

  // Forcing information
  int iFile = 1; // index of current forcing file from forcing file list
  int stepsInCurrentFFile;
  int forcingStep = 1;
  int timestep = 1;
  int num_gru_done_timestep = 0;
  int num_steps = 0;
};
