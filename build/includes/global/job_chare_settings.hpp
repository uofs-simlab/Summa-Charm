#pragma once
#include <string>
#include <vector>
#include <optional>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <thread>
#include "json.hpp"
#include "pup.h"

#define SUCCESS 0
#define FAILURE -1
#define MISSING_INT -9999
#define MISSING_DOUBLE -9999.0
#define OUTPUT_TIMESTEPS 500
#define NUM_PARTITIONS 8
#define OUTPUT_FREQUENCY 1000
#define GRU_PER_JOB 1000

using json = nlohmann::json;

class JobChareSettings
{
public:
  std::string file_manager_path_;
  int max_run_attempts_;
  bool data_assimilation_mode_;
  int batch_size_;
  int worker_pool_size_;
  int worker_prefetch_depth_;
  bool allow_worker_oversubscription_;
  bool reserve_pe0_for_control_;

  JobChareSettings(std::string file_manager_path = "",
                   int max_run_attempts = 5,
                   bool data_assimilation_mode = false,
                   int batch_size = 10,
                   int worker_pool_size = 0,
                   int worker_prefetch_depth = 1,
                   bool allow_worker_oversubscription = false,
                   bool reserve_pe0_for_control = false)
      : file_manager_path_(file_manager_path),
        max_run_attempts_(max_run_attempts),
        data_assimilation_mode_(data_assimilation_mode),
        batch_size_(batch_size),
        worker_pool_size_(worker_pool_size),
        worker_prefetch_depth_(worker_prefetch_depth),
        allow_worker_oversubscription_(allow_worker_oversubscription),
        reserve_pe0_for_control_(reserve_pe0_for_control) {};

  ~JobChareSettings() {};

  std::string toString()
  {
    std::string str = "Job Chare Settings:\n";
    str += "File Manager Path: " + file_manager_path_ + "\n";
    str += "Max Run Attempts: " + std::to_string(max_run_attempts_) + "\n";
    str += "Data Assimilation Mode: " + std::to_string(data_assimilation_mode_) + "\n";
    str += "Batch Size: " + std::to_string(batch_size_) + "\n";
    str += "Worker Pool Size: " + std::to_string(worker_pool_size_) + "\n";
    str += "Worker Prefetch Depth: " + std::to_string(worker_prefetch_depth_) + "\n";
    str += "Allow Worker Oversubscription: " +
           std::to_string(allow_worker_oversubscription_) + "\n";
    str += "Reserve PE0 For Control: " +
           std::to_string(reserve_pe0_for_control_) + "\n";
    return str;
  }

  // PUP method for Charm++ serialization
  template <typename PUPER>
  void pup(PUPER &p) {
    p | file_manager_path_;
    p | max_run_attempts_;
    p | data_assimilation_mode_;
    p | batch_size_;
    p | worker_pool_size_;
    p | worker_prefetch_depth_;
    p | allow_worker_oversubscription_;
    p | reserve_pe0_for_control_;
  }
};
