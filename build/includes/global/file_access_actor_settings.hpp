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

class FileAccessActorSettings
{
public:
  int num_partitions_in_output_buffer_;
  int num_timesteps_in_output_buffer_;
  std::string output_file_suffix_;

  FileAccessActorSettings(int num_partitions_in_output_buffer = 0,
                          int num_timesteps_in_output_buffer = 0,
                          std::string output_file_suffix = "")
      : num_partitions_in_output_buffer_(num_partitions_in_output_buffer),
        num_timesteps_in_output_buffer_(num_timesteps_in_output_buffer),
        output_file_suffix_(output_file_suffix) {};
  ~FileAccessActorSettings() {};

  std::string toString()
  {
    std::string str = "File Access Actor Settings:\n";
    str += "Num Partitions in Output Buffer: " +
           std::to_string(num_partitions_in_output_buffer_) + "\n";
    str += "Num Timesteps in Output Buffer: " +
           std::to_string(num_timesteps_in_output_buffer_) + "\n";
    str += "Output File Suffix: " + output_file_suffix_ + "\n";
    return str;
  }
};