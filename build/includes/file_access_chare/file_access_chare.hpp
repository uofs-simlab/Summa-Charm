#pragma once

#include "charm++.h"
#include "pup_stl.h"

// Include Charm++ generated headers (first, to resolve forward references)
#include "FileAccessChare.decl.h"
#include "GruChare.decl.h"
#include "GruWorker.decl.h"

// Regular includes
#include <memory>
#include <vector>
#include "num_gru_info.hpp" // Include the global NumGRUInfo definition
#include "output_buffer.hpp"
#include "file_access_chare_settings.hpp"
#include "fortran_data_types.hpp"
#include "auxilary.hpp"
#include "forcing_file_info.hpp"
#include "json.hpp"

// Forward declarations
class OutputBuffer;
class TimingInfo;
class FileAccessChareSettings;
// Fortran interface functions
extern "C"
{
  void f_getNumTimeSteps(int &num_timesteps);
  void writeRestart_fortran(void *handle_ncid, int &start_gru, int &max_gru,
                            int &timestep, int &year, int &month, int &day,
                            int &hour, int &err);
}

// The actual class definition
class FileAccessChare : public CBase_FileAccessChare
{
private:
  TimingInfo timing_info_;
  NumGRUInfo num_gru_info_;
  FileAccessChareSettings fa_settings_;
  CkChareID job_chare_proxy_;

  // Direct routing to GruWorkers — avoids JobChare bottleneck on the hot I/O path
  CkArrayID gru_worker_array_id_;
  bool gru_worker_initialized_ = false;
  int num_workers_in_pool_ = 0;
  std::vector<int> job_to_worker_; // job_index → worker_id

  int start_gru_;
  int num_gru_;
  int num_hru_;

  int num_steps_;
  bool write_params_flag_ = true;
  std::unique_ptr<forcingFileContainer> forcing_files_;
  std::unique_ptr<OutputBuffer> output_buffer_;
  int write_output_calls_ = 0;
  int write_flushes_ = 0;
  int write_resume_batches_ = 0;
  int write_resume_jobs_ = 0;

  // Checkpointing variables
  int completed_checkpoints_ = 1;
  std::vector<int> hru_checkpoints_;
  std::vector<int> hru_timesteps_;

public:
  FileAccessChare(NumGRUInfo num_gru_info, FileAccessChareSettings fa_settings, CkChareID parent_proxy);
  FileAccessChare(CkMigrateMessage *msg) : num_gru_info_(), fa_settings_() {};

  int initFileAccessChare(const int file_gru, int num_hru);
  int getNumOutputSteps(int job_index);
  void accessForcing(int i_file, int gru_job_index);
  void runFailure(int index_gru_job);
  std::tuple<double, double> finalize();
  void error(int err_code, std::string err_msg);
  int restartFailures();
  void accessForcingInternal(int i_file);
  void writeOutput(int index_gru, int gru_job_index);
  void writeRestartOutput(int gru, int gru_timestep, int gru_checkpoint,
                          int output_structure_index, int year, int month, int day, int hour);
  void setGruWorkerProxy(CkArrayID gru_worker_id, int num_jobs, int num_workers);
  void updateJobWorkerMapping(int job_index, int worker_id);
};

/*********************************************
 * File Access Chare state variables
 *********************************************/
struct file_access_state
{
  TimingInfo file_access_timing;
  CkChareID parent;
  int start_gru;
  int num_gru;
  int num_hru;

  NumGRUInfo num_gru_info;

  void *handle_ncid = new_handle_var_i(); // output file ids
  int num_steps;
  int num_output_steps;
  int err = 0; // this is to make compiler happy

  std::unique_ptr<forcingFileContainer> forcing_files;

  bool write_params_flag = true;
};
