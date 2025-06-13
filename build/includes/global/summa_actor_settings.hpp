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

class SummaActorSettings
{
public:
  int max_gru_per_job_;
  bool enable_logging_;
  std::string log_dir_;

  SummaActorSettings(int max_gru_per_job = 0, bool enable_logging = false,
                     std::string log_dir = "")
      : max_gru_per_job_(max_gru_per_job), enable_logging_(enable_logging),
        log_dir_(log_dir) {};
  ~SummaActorSettings() {};

  std::string toString()
  {
    std::string str = "Summa Actor Settings:\n";
    str += "Max GRU Per Job: " + std::to_string(max_gru_per_job_) + "\n";
    str += "Enable Logging: " + std::to_string(enable_logging_) + "\n";
    str += "Log Directory: " + log_dir_ + "\n";
    return str;
  }
};