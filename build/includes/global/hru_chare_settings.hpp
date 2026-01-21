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

class HRUChareSettings
{
public:
  bool print_output_;
  int output_frequency_;
  int restart_frequency_;

  HRUChareSettings(
      bool print_output = false,
      int output_frequency = 100,
      int restart_frequency = 0)
      : print_output_(print_output),
        output_frequency_(output_frequency),
        restart_frequency_(restart_frequency) {};
  
  ~HRUChareSettings() {};

  std::string toString()
  {
    std::string str = "HRU Chare Settings:\n";
    str += "Print Output: " + std::to_string(print_output_) + "\n";
    str += "Output Frequency: " + std::to_string(output_frequency_) + "\n";
    return str;
  }

  // PUP method for Charm++ serialization
  template <typename PUPER>
  void pup(PUPER &p) {
    p | print_output_;
    p | output_frequency_;
  }
};