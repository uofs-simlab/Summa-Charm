#include "summa_chare.hpp"
#include "SummaChare.decl.h" // Include Charm++ generated declarations first
#include "json.hpp"
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

// Create directories
#include <cstring>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

// Forward declaration
using json = nlohmann::json;

SummaChare::SummaChare(int start_gru, int num_gru, std::string config_file,
                       std::string master_file, std::string output_file_suffix)
    : start_gru_(start_gru), num_gru_(num_gru), file_gru_(-1),
      num_gru_failed_(0), master_file_(master_file), config_file_(config_file),
      output_file_suffix_(output_file_suffix) {

  CkPrintf("Starting SUMMA Chare: start_gru=%d, num_gru=%d\n", start_gru,
           num_gru);

  readSettings(config_file_);
  printSettings();
  if (master_file_ != "") {
    job_actor_settings_.file_manager_path_ = master_file_;
  }
  if (output_file_suffix_ != "") {
    fa_actor_settings_.output_file_suffix_ = output_file_suffix_;
  }

  timing_info_ = TimingInfo();
  timing_info_.addTimePoint("total_duration");
  timing_info_.updateStartPoint("total_duration");

  file_manager_ =
      std::make_unique<FileManager>(job_actor_settings_.file_manager_path_);
  auto err_msg = file_manager_->setTimesDirsAndFiles();
  if (!err_msg.empty()) {
    CkPrintf("ERROR--File Manager: %s\n", err_msg.c_str());
    CkExit();
    return;
  }

  file_gru_ = file_manager_->getFileGru();
  if (file_gru_ < 0) {
    CkPrintf("ERROR--File Manager: Unable To Verify Number Of GRUs\n");
    CkExit();
    return;
  } else if (file_gru_ < num_gru_ + start_gru_ - 1) {
    CkPrintf("ERROR--File Manager: Number Of GRUs Exceeds File GRUs\n");
    CkExit();
    return;
  }

  global_fortran_state_ = std::make_unique<SummaGlobalData>();
  auto err = global_fortran_state_->defineGlobalData();
  if (err != 0) {
    CkPrintf("ERROR--Global State: Unable To Define Global Data");
    CkExit();
    return;
  }

  err = createLogDirectory();
  if (err != 0) {
    CkPrintf("ERROR--Unable To Create Log Directory\n");
    CkExit();
    return;
  }

  CkPrintf("Log directory created: %s\n", log_folder_.c_str());

  batch_container_ = std::make_unique<BatchContainer>(
      start_gru_, num_gru_, summa_actor_settings_.max_gru_per_job_,
      log_folder_);
  CkPrintf("\n\nStarting SUMMA Chare with %d Batches\n\n",
           batch_container_->getBatchesRemaining());

  if (spawnJob() != 0) {
    CkPrintf("ERROR--Summa_Actor: Unable To Spawn Job\n");
    CkExit();
    return;
  }
}

void SummaChare::doneJob(int num_gru_failed, double job_duration,
                         double read_duration, double write_duration) {

  int num_success = current_batch_->getNumHRU() - num_gru_failed;
  batch_container_->updateBatchStats(current_batch_->getBatchID(), job_duration,
                                     read_duration, write_duration, num_success,
                                     num_gru_failed);

  num_gru_failed_ += num_gru_failed;

  if (!batch_container_->hasUnsolvedBatches()) {
    finalize();
    return;
  }
  // TODO: Implement a way to reuse the current jobChare instead of spawning new
  if (spawnJob() != 0) {
    CkPrintf("ERROR--Unable to spawn next job\n");
    CkExit();
  }
}

void SummaChare::reportError(int err_code, const std::string err_msg) {
  if (err_code == -2) {
    CkPrintf("Unrecoverable error from JobChare: %s\n", err_msg.c_str());
    CkExit(); // good for fatal errors
  } else {
    CkPrintf("Recoverable Error from jobChare\n\t Error Message = %s\n\t Error "
             "Code = %d\nIMPLEMENTATION NEEDED\n",
             err_msg.c_str(), err_code);
    CkExit();
  }
}

// TODO: Implement this in a way that is usable with Charm++
/*
[this](const down_msg& dm) {
      self_->println("Lost Connection With A Connected Actor\nReason: {}",
                   to_string(dm.reason));
    }
*/


int SummaChare::spawnJob() {
  std::optional<Batch> batch = batch_container_->getUnsolvedBatch();
  if (!batch.has_value()) {
    CkPrintf("No more batches to process. Finalizing...\n");
    finalize();
    return -1;
  }
  current_batch_ = std::make_shared<Batch>(batch.value());
  current_job_ = CProxy_JobChare::ckNew(
      batch.value(), summa_actor_settings_.enable_logging_, job_actor_settings_,
      fa_actor_settings_, hru_actor_settings_, thishandle, file_gru_);
  return 0;
}

bool create_directories(const std::string &path) {
  struct stat info;

  if (stat(path.c_str(), &info) != 0) {
    // Directory does not exist
    if (errno == ENOENT) {
      if (mkdir(path.c_str(), 0755) != 0) {
        std::cerr << "Error creating directory: " << strerror(errno)
                  << std::endl;
        return false;
      }
    } else {
      std::cerr << "Error checking directory: " << strerror(errno) << std::endl;
      return false;
    }
  } else if (!(info.st_mode & S_IFDIR)) {
    std::cerr << "Path exists but is not a directory" << std::endl;
    return false;
  }
  return true;
}
int SummaChare::createLogDirectory() {
  if (summa_actor_settings_.enable_logging_) {
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    std::tm *now_tm = std::localtime(&now_c);
    std::stringstream ss;
    ss << std::put_time(now_tm, "%m_%d_%H:%M");
    log_folder_ = "startgru-" + std::to_string(start_gru_) + "_endgru-" +
                  std::to_string(start_gru_ + num_gru_ - 1) + "_" + ss.str();
    if (!summa_actor_settings_.log_dir_.empty())
      log_folder_ = summa_actor_settings_.log_dir_ + "/" + log_folder_;

    return (create_directories(log_folder_)) ? 0 : -1;
  } else {
    log_folder_ = ""; // Empty log to signal no logging
    return 0;
  }
}

void SummaChare::finalize() {
  CkPrintf("All Batches Finished\n{}",
           batch_container_->getAllBatchInfoString());

  timing_info_.updateEndPoint("total_duration");

  double total_dur_sec =
      timing_info_.getDuration("total_duration").value_or(-1.0);
  double total_dur_min = total_dur_sec / 60;
  double total_dur_hr = total_dur_min / 60;
  double read_dur_sec = batch_container_->getTotalReadTime();
  double write_dur_sec = batch_container_->getTotalWriteTime();

  CkPrintf("\n________________SUMMA INFO________________\n"
                 "Total Duration = {} Seconds\n"
                 "Total Duration = {} Minutes\n"
                 "Total Duration = {} Hours\n"
                 "Total Read Duration = {} Seconds\n"
                 "Total Write Duration = {} Seconds\n"
                 "Num Failed = {}\n"
                 "___________________Program Finished__________________\n",
                 total_dur_sec, total_dur_min, total_dur_hr, read_dur_sec,
                 write_dur_sec, num_gru_failed_);
  CkPrintf("Test completed successfully. Exiting.\n");
  CkExit();
}

template <typename T>
std::optional<T> getSettings(json settings, std::string key_1,
                             std::string key_2) {
  try {
    if (settings.find(key_1) != settings.end()) {
      json key_1_settings = settings[key_1];

      // find value behind second key
      if (key_1_settings.find(key_2) != key_1_settings.end()) {
        return key_1_settings[key_2];
      } else
        return {};
    } else {
      return {}; // return none in the optional (error value)
    }
  } catch (json::exception &e) {
    std::cout << e.what() << "\n" << key_1 << "\n" << key_2 << "\n";
    return {};
  }
}

int SummaChare::readSettings(std::string config_file) {
  std::ifstream settings_file(config_file);
  json json_settings;
  if (!settings_file.good()) {
    std::cout << "Could not open settings file: " << config_file
              << "\n\tContinuing with default settings\n";
  } else {
    settings_file >> json_settings;
  }
  settings_file.close();

  summa_actor_settings_ = SummaActorSettings(
      getSettings<int>(json_settings, "Summa_Actor", "max_gru_per_job")
          .value_or(GRU_PER_JOB),
      getSettings<bool>(json_settings, "Summa_Actor", "enable_logging")
          .value_or(false),
      getSettings<std::string>(json_settings, "Summa_Actor", "log_dir")
          .value_or(""));

  fa_actor_settings_ = FileAccessActorSettings(
      getSettings<int>(json_settings, "File_Access_Actor",
                       "num_partitions_in_output_buffer")
          .value_or(NUM_PARTITIONS),
      getSettings<int>(json_settings, "File_Access_Actor",
                       "num_timesteps_in_output_buffer")
          .value_or(OUTPUT_TIMESTEPS),
      getSettings<std::string>(json_settings, "File_Access_Actor",
                               "output_file_suffix")
          .value_or(""));

  job_actor_settings_ = JobActorSettings(
      getSettings<std::string>(json_settings, "Job_Actor", "file_manager_path")
          .value_or(""),
      getSettings<int>(json_settings, "Job_Actor", "max_run_attempts")
          .value_or(1),
      getSettings<bool>(json_settings, "Job_Actor", "data_assimilation_mode")
          .value_or(false),
      getSettings<int>(json_settings, "Job_Actor", "batch_size").value_or(10));

  hru_actor_settings_ = HRUActorSettings(
      getSettings<bool>(json_settings, "HRU_Actor", "print_output")
          .value_or(true),
      getSettings<int>(json_settings, "HRU_Actor", "output_frequency")
          .value_or(OUTPUT_FREQUENCY),
      getSettings<double>(json_settings, "HRU_Actor", "abs_tol").value_or(1e-3),
      getSettings<double>(json_settings, "HRU_Actor", "rel_tol")
          .value_or(1e-3));

  return SUCCESS;
}

void SummaChare::printSettings() {
  std::cout << "************ DISTRIBUTED_SETTINGS ************\n"
            << distributed_settings_.toString() << "\n"
            << "************ SUMMA_ACTORS SETTINGS ************\n"
            << summa_actor_settings_.toString() << "\n"
            << "************ FILE_ACCESS_ACTOR SETTINGS ************\n"
            << fa_actor_settings_.toString() << "\n"
            << "************ JOB_ACTOR SETTINGS ************\n"
            << job_actor_settings_.toString() << "\n"
            << "************ HRU_ACTOR SETTINGS ************\n"
            << hru_actor_settings_.toString() << "\n"
            << "********************************************\n\n";
}
