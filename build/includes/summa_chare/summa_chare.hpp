#pragma once

#include <chrono>
#include <string>
#include <vector>
#include <memory>
#include "SummaChare.decl.h"
#include "timing_info.hpp"
#include "settings_functions.hpp"
#include "file_manager.hpp"
#include "batch_container.hpp"
#include "summa_global_data.hpp"

class SummaChare : public CBase_SummaChare
{
public:
  SummaChare(int start_gru, int num_gru, std::string config_file,
             std::string master_file, std::string output_file_suffix);
  
  // Entry methods
  void doneJob(int num_gru_failed, double job_duration, double read_duration, double write_duration);
  void reportError(int err_code, std::string err_msg);

private:
  Settings settings_;
  TimingInfo timing_info_;
  int start_gru_;
  int num_gru_;
  int file_gru_;
  int num_gru_failed_;
  std::string master_file_;
  std::string config_file_;
  std::string output_file_suffix_;
  std::string log_folder_;

  std::unique_ptr<FileManager> file_manager_;
  std::unique_ptr<BatchContainer> batch_container_;
  std::shared_ptr<Batch> current_batch_;
  std::unique_ptr<SummaGlobalData> global_fortran_state_;
  CkChareID current_job_;
  
  int spawnJob();
  int createLogDirectory();
  void finalize();
};