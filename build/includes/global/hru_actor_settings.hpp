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

class HRUActorSettings
{
public:
  bool print_output_;
  int output_frequency_;

  double abs_tol_;
  double rel_tol_;

  HRUActorSettings(bool print_output = false, int output_frequency = 100,
                   double abs_tol = 0.0, double rel_tol = 0.0)
      : print_output_(print_output), output_frequency_(output_frequency),
        abs_tol_(abs_tol), rel_tol_(rel_tol) {};
  ~HRUActorSettings() {};

  std::string toString()
  {
    std::string str = "HRU Actor Settings:\n";
    str += "Print Output: " + std::to_string(print_output_) + "\n";
    str += "Output Frequency: " + std::to_string(output_frequency_) + "\n";
    str += "Abs Tol: " + std::to_string(abs_tol_) + "\n";
    str += "Rel Tol: " + std::to_string(rel_tol_) + "\n";
    return str;
  }

  // Charm++ PUP serialization method
  void pup(PUP::er &p) {
    p | print_output_;
    p | output_frequency_;
    p | abs_tol_;
    p | rel_tol_;
  }
};