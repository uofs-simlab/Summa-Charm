#pragma once
#include <string>
#include <vector>
#include <optional>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <thread>
#include "json.hpp"

#define SUCCESS 0
#define FAILURE -1
#define MISSING_INT -9999
#define MISSING_DOUBLE -9999.0
#define OUTPUT_TIMESTEPS 500
#define NUM_PARTITIONS 8
#define OUTPUT_FREQUENCY 1000
#define GRU_PER_JOB 1000

using json = nlohmann::json;

class SummaChareSettings
{
public:
  int max_gru_per_job_;
  bool enable_logging_;
  std::string log_dir_;

  SummaChareSettings(int max_gru_per_job = 0, bool enable_logging = false,
                     std::string log_dir = "")
      : max_gru_per_job_(max_gru_per_job), enable_logging_(enable_logging),
        log_dir_(log_dir) {};
  ~SummaChareSettings() {};

  std::string toString()
  {
    std::string str = "Summa Chare Settings:\n";
    str += "Max GRU Per Job: " + std::to_string(max_gru_per_job_) + "\n";
    str += "Enable Logging: " + std::to_string(enable_logging_) + "\n";
    str += "Log Directory: " + log_dir_ + "\n";
    return str;
  }
};