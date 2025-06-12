#include "summa_chare.hpp"
#include "job_array.hpp"
#include "json.hpp"
#include <iostream>
#include <fstream>
// #include <filesystem>
#include <chrono>
#include <sstream>
#include <iomanip>

// Create directories
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <cstring>
#include <iostream>

// Forward declaration
using json = nlohmann::json;

SummaChare::SummaChare(int start_gru, int num_gru,
                       std::string config_file, std::string master_file,
                       std::string output_file_suffix)
    : start_gru_(start_gru), num_gru_(num_gru), file_gru_(-1),
      num_gru_failed_(0), master_file_(master_file),
      config_file_(config_file), output_file_suffix_(output_file_suffix)
{

  CkPrintf("Starting SUMMA Chare: start_gru=%d, num_gru=%d\n", start_gru, num_gru);

  settings_ = Settings(config_file_);
  settings_.readSettings();
  settings_.printSettings();
  if (master_file_ != "")
  {
    settings_.job_actor_settings_.file_manager_path_ = master_file_;
  }
  if (output_file_suffix_ != "")
  {
    settings_.fa_actor_settings_.output_file_suffix_ = output_file_suffix_;
  }

  timing_info_ = TimingInfo();
  timing_info_.addTimePoint("total_duration");
  timing_info_.updateStartPoint("total_duration");

  file_manager_ = std::make_unique<FileManager>(
      settings_.job_actor_settings_.file_manager_path_);
  auto err_msg = file_manager_->setTimesDirsAndFiles();

  if (!err_msg.empty())
  {
    CkPrintf("ERROR--File Manager: %s\n", err_msg.c_str());
    CkExit();
    return;
  }

  file_gru_ = file_manager_->getFileGru();
  if (file_gru_ < 0)
  {
    CkPrintf("ERROR--File Manager: Unable To Verify Number Of GRUs\n");
    CkExit();
    return;
  }
  else if (file_gru_ < num_gru_ + start_gru_ - 1)
  {
    CkPrintf("ERROR--File Manager: Number Of GRUs Exceeds File GRUs\n");
    CkExit();
    return;
  }

  global_fortran_state_ = std::make_unique<SummaGlobalData>();
  auto err = global_fortran_state_->defineGlobalData();
  if (err != 0)
  {
    CkPrintf("ERROR--Global State: Unable To Define Global Data");
    CkExit();
    return;
  }

  err = createLogDirectory();
  if (err != 0)
  {
    CkPrintf("ERROR--Unable To Create Log Directory\n");
    CkExit();
    return;
  }

  batch_container_ = std::make_unique<BatchContainer>(start_gru_, num_gru_,
                                                      settings_.summa_actor_settings_.max_gru_per_job_, log_folder_);
  CkPrintf("\n\nStarting SUMMA Chare with %d Batches\n\n",
           batch_container_->getBatchesRemaining());

  if (spawnJob() != 0)
  {
    CkPrintf("ERROR--Summa_Actor: Unable To Spawn Job\n");
    CkExit();
    return;
  }
}

void SummaChare::finalize()
{
  timing_info_.updateEndPoint("total_duration");
  CkPrintf("SummaChare completed. Total failed GRUs: %d\n", num_gru_failed_);
  CkExit();
}

bool create_directories(const std::string &path)
{
  struct stat info;

  if (stat(path.c_str(), &info) != 0)
  {
    // Directory does not exist
    if (errno == ENOENT)
    {
      if (mkdir(path.c_str(), 0755) != 0)
      {
        std::cerr << "Error creating directory: " << strerror(errno) << std::endl;
        return false;
      }
    }
    else
    {
      std::cerr << "Error checking directory: " << strerror(errno) << std::endl;
      return false;
    }
  }
  else if (!(info.st_mode & S_IFDIR))
  {
    std::cerr << "Path exists but is not a directory" << std::endl;
    return false;
  }
  return true;
}
int SummaChare::createLogDirectory()
{
  if (settings_.summa_actor_settings_.enable_logging_)
  {
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    std::tm *now_tm = std::localtime(&now_c);
    std::stringstream ss;
    ss << std::put_time(now_tm, "%m_%d_%H:%M");
    log_folder_ = "startgru-" + std::to_string(start_gru_) + "_endgru-" +
                  std::to_string(start_gru_ + num_gru_ - 1) + "_" + ss.str();
    if (!settings_.summa_actor_settings_.log_dir_.empty())
      log_folder_ = settings_.summa_actor_settings_.log_dir_ + "/" + log_folder_;

    return (create_directories(log_folder_)) ? 0 : -1;
  }
  else
  {
    log_folder_ = ""; // Empty log to signal no logging
    return 0;
  }
}



int SummaChare::spawnJob()
{
    // Get the next unsolved batch
    auto batch_optional = batch_container_->getUnsolvedBatch();
    if (!batch_optional.has_value()) {
        CkPrintf("No more batches to process. Finalizing...\n");
        finalize();
        return 0;
    }

    current_batch_ = std::make_shared<Batch>(batch_optional.value());

    CkPrintf("Processing Batch ID: %d, Start HRU: %d, Num HRUs: %d\n", 
             current_batch_->getBatchID(), 
             current_batch_->getStartHRU(), 
             current_batch_->getNumHRU());

    // Create an array of JobArray chares
    CProxy_JobArray job_array_proxy = CProxy_JobArray::ckNew(*current_batch_, num_gru_);

    return 0;
}

void SummaChare::doneJob(int num_gru_failed, double job_duration, double read_duration, double write_duration)
{
  int num_success = current_batch_->getNumHRU() - num_gru_failed;
  batch_container_->updateBatchStats(current_batch_->getBatchID(),
                                     job_duration, read_duration, write_duration, num_success, num_gru_failed);

  num_gru_failed_ += num_gru_failed;

  CkPrintf("Batch %d completed. Success: %d, Failed: %d\n", 
           current_batch_->getBatchID(), num_success, num_gru_failed);

  if (!batch_container_->hasUnsolvedBatches())
  {
    CkPrintf("All batches completed!\n");
    finalize();
    return;
  }

  if (spawnJob() != 0)
  {
    CkPrintf("ERROR--Unable to spawn next job\n");
    CkExit();
  }
}

void SummaChare::reportError(int err_code, std::string err_msg)
{
  if (err_code == -2)
  {
    CkPrintf("Unrecoverable error from JobChare: %s\n", err_msg.c_str());
    CkExit(); // Fatal error
  }
  else
  {
    CkPrintf("Recoverable error (not yet handled): %s\n", err_msg.c_str());
    CkExit(); // For now, treat all errors as fatal
  }
}


#include "SummaChare.def.h"

