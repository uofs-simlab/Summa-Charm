#pragma once

#include "charm++.h"
#include "pup_stl.h"

// Include Charm++ generated headers (first, to resolve forward references)
#include "FileAccessChare.decl.h"

// Regular includes
#include <memory>
#include <vector>
#include "num_gru_info.hpp"  // Include the global NumGRUInfo definition
#include "output_buffer.hpp"
#include "file_access_actor_settings.hpp"
#include "fortran_data_types.hpp"
#include "auxilary.hpp"
#include "forcing_file_info.hpp"
#include "json.hpp"

// Forward declarations
class OutputBuffer;
class TimingInfo;
class FileAccessActorSettings;  // Defined in settings_functions.hpp

// Fortran interface functions
extern "C" {
    void f_getNumTimeSteps(int& num_timesteps);
    void writeRestart_fortran(void* handle_ncid, int& start_gru, int& max_gru, 
                              int& timestep, int& year, int& month, int& day, 
                              int& hour, int& err);
}

// The actual class definition
class FileAccessChare : public CBase_FileAccessChare {
private:
  TimingInfo timing_info_;
  NumGRUInfo num_gru_info_;
  FileAccessActorSettings fa_settings_;
  CkChareID job_chare_proxy_;

  int start_gru_;
  int num_gru_;
  int num_hru_;

  int num_steps_;
  bool write_params_flag_ = true;
  std::unique_ptr<forcingFileContainer> forcing_files_;
  std::unique_ptr<OutputBuffer> output_buffer_;

  // Checkpointing variables
  int completed_checkpoints_ = 1;
  std::vector<int> hru_checkpoints_;
  std::vector<int> hru_timesteps_;

public:
  FileAccessChare(NumGRUInfo num_gru_info, FileAccessActorSettings fa_settings, CkChareID parent_proxy);
  FileAccessChare(CkMigrateMessage *msg) : num_gru_info_(), fa_settings_() {};

  int initFileAccessChare(const int file_gru, int num_hru);
  int getNumOutputSteps(int job_index);
  void accessForcing(int i_file, CkChareID gru_chare);
  void runFailure(int index_gru_job);
  void finalize();
  void error(int err_code, std::string err_msg);
  int restartFailures();
  void accessForcingInternal(int i_file);
  void writeOutput(int index_gru, CkChareID gru_chare);

  // Migration support
  void pup(PUP::er &p) override {
    CBase_FileAccessChare::pup(p);
    p | num_gru_info_;
    p | fa_settings_;
    p | start_gru_;
    p | num_gru_;
    p | num_hru_;
    p | num_steps_;
    p | write_params_flag_;
    p | completed_checkpoints_;
    p | hru_checkpoints_;
    p | hru_timesteps_;
  }
};

/*********************************************
 * File Access Actor state variables
 *********************************************/
struct file_access_state {
  TimingInfo file_access_timing;
  CkChareID parent; 
  int start_gru;
  int num_gru;
  int num_hru;

  NumGRUInfo num_gru_info;

  void *handle_ncid = new_handle_var_i();  // output file ids
  int num_steps;
  int num_output_steps;
  int err = 0; // this is to make compiler happy



  std::unique_ptr<forcingFileContainer> forcing_files;

  bool write_params_flag = true;
};
