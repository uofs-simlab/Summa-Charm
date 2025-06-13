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
#include "pup_stl.h"

#define SUCCESS 0
#define FAILURE -1
#define MISSING_INT -9999
#define MISSING_DOUBLE -9999.0
#define OUTPUT_TIMESTEPS 500
#define NUM_PARTITIONS 8
#define OUTPUT_FREQUENCY 1000
#define GRU_PER_JOB 1000

using json = nlohmann::json;

class JobActorSettings
{
public:
  std::string file_manager_path_;
  int max_run_attempts_;
  bool data_assimilation_mode_;
  int batch_size_;

  JobActorSettings(std::string file_manager_path = "",
                   int max_run_attempts = 1,
                   bool data_assimilation_mode = false,
                   int batch_size = 10)
      : file_manager_path_(file_manager_path),
        max_run_attempts_(max_run_attempts),
        data_assimilation_mode_(data_assimilation_mode),
        batch_size_(batch_size) {};

  ~JobActorSettings() {};

  std::string toString()
  {
    std::string str = "Job Actor Settings:\n";
    str += "File Manager Path: " + file_manager_path_ + "\n";
    str += "Max Run Attempts: " + std::to_string(max_run_attempts_) + "\n";
    str += "Data Assimilation Mode: " + std::to_string(data_assimilation_mode_) + "\n";
    str += "Batch Size: " + std::to_string(batch_size_) + "\n";
    return str;
  }
};