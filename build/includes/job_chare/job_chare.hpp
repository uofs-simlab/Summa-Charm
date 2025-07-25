#pragma once

#include "JobChare.decl.h"
#include "FileAccessChare.decl.h"
#include "file_access_actor_settings.hpp" // For FileAccessActorSettings
// #include "file_access_chare.hpp"
// #include "gru_batch_actor.hpp"
#include "gru_struc.hpp"
#include "hru_actor_settings.hpp" // For HruActorSettings
#include "job_actor_settings.hpp" // For JobActorSettings
#include "json.hpp"
#include "num_gru_info.hpp" // For NumGRUInfo
#include "timing_info.hpp"
#include "logger.hpp"
#include "summa_init_struc.hpp" // For SummaInitStruc
#include "gru_struc.hpp" // For GruStruc


#include <chrono>
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
  CkChareID summa_chare_proxy_;
  CProxy_FileAccessChare file_access_chare_;

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
  JobActorSettings job_actor_settings_;
  FileAccessActorSettings fa_actor_settings_;
  HRUActorSettings hru_actor_settings_;

   //Min rel and abs tol values
  const double MIN_REL_TOL = 1e-6;
  const double MIN_ABS_TOL = 1e-6;

  // HRU Attributes
  int be_steps_ = -9999;
  double rel_tol_ = -9999;
  double abs_tol_ = -9999;
  double rel_tol_temp_cas_ = -9999;
  double rel_tol_temp_veg_ = -9999;
  double rel_tol_wat_veg_ = -9999;
  double rel_tol_temp_soil_snow_ = -9999;
  double rel_tol_wat_snow_ = -9999;
  double rel_tol_matric_ = -9999;
  double rel_tol_aquifr_ = -9999;
  double abs_tol_temp_cas_ = -9999;
  double abs_tol_temp_veg_ = -9999;
  double abs_tol_wat_veg_ = -9999;
  double abs_tol_temp_soil_snow_ = -9999;
  double abs_tol_wat_snow_ = -9999;
  double abs_tol_matric_ = -9999;
  double abs_tol_aquifr_ = -9999;
  // TODO: Ashley's New Variables
  double abs_tolWat_ = -9999;
  double abs_tolNrg_ = -9999;
  
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

public:
  // Simplified constructor - takes batch and the chare ID
  JobChare(Batch batch, bool enable_logging,
           JobActorSettings job_actor_settings,
           FileAccessActorSettings fa_actor_settings,
           HRUActorSettings hru_actor_settings,
           CkChareID summa_chare_proxy);

  void spawnGruActors();
  void doneHRUJob(int job_index);
  void handleFinishedGRU(int job_index); 
  void finalizeJob();
  void restartFailures();
  void handleGruChareError(int job_index, int timestep, int err_code,
                              std::string err_msg);
  void handleGRUError(int err_code, int job_index, int timestep, 
                              std::string err_msg);
  void handleFileAccessError(int err_code, std::string err_msg);
};



/*********************************************
 * Job Actor Data Structures
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
 * Job Actor state variables
 *********************************************/
struct job_state {
  TimingInfo job_timing;
  std::unique_ptr<Logger> logger;
  std::unique_ptr<ErrorLogger> err_logger;
  std::unique_ptr<SuccessLogger> success_logger;
  // Actor References
  CkChareID file_access_actor; // actor reference for the file_access_actor
  CkChareID summa_chare_proxy;            // actor reference to the top-level SummaActor

  Batch batch; // Information about the number of HRUs and starting point 

  // TODO: gru_struc can contain the num_gru_info and be the gru_container
  std::unique_ptr<GruStruc> gru_struc; 
  NumGRUInfo num_gru_info;
  GRU_Container gru_container;

  std::unique_ptr<SummaInitStruc> summa_init_struc;

  // Variables for GRU monitoring
  int dt_init_start_factor = 1; // Initial Factor for dt_init (coupled_em)
  int num_gru_done = 0;         // The number of GRUs that have completed
  int num_gru_failed = 0;       // Number of GRUs that have failed

  
  std::string hostname;

  
  FileAccessActorSettings file_access_actor_settings;
  JobActorSettings job_actor_settings; 
  HRUActorSettings hru_actor_settings;

  // Forcing information
  int iFile = 1; // index of current forcing file from forcing file list
  int stepsInCurrentFFile;
  int forcingStep = 1;
  int timestep = 1;
  int num_gru_done_timestep = 0;
  int num_steps = 0;
};