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

class HRUActorSettings
{
public:
  bool print_output_;
  int output_frequency_;
  int be_steps_;

  double rel_tol_;
  double rel_tol_temp_cas_;
  double rel_tol_temp_veg_;
  double rel_tol_wat_veg_;
  double rel_tol_temp_soil_snow_;
  double rel_tol_wat_snow_;
  double rel_tol_matric_;
  double rel_tol_aquifr_;

  double abs_tol_;
  double abs_tolWat_;
  double abs_tolNrg_;
  double abs_tol_temp_cas_;
  double abs_tol_temp_veg_;
  double abs_tol_wat_veg_;
  double abs_tol_temp_soil_snow_;
  double abs_tol_wat_snow_;
  double abs_tol_matric_;
  double abs_tol_aquifr_;

  bool default_tol_;

  HRUActorSettings(
      bool print_output = false,
      int output_frequency = 100,
      int be_steps = MISSING_INT,
      double rel_tol = 0.0,
      double rel_tol_temp_cas = 0.0,
      double rel_tol_temp_veg = 0.0,
      double rel_tol_wat_veg = 0.0,
      double rel_tol_temp_soil_snow = 0.0,
      double rel_tol_wat_snow = 0.0,
      double rel_tol_matric = 0.0,
      double rel_tol_aquifr = 0.0,
      double abs_tol = 0.0,
      double abs_tolWat_ = MISSING_DOUBLE,
      double abs_tolNrg_ = MISSING_DOUBLE,
      double abs_tol_temp_cas = 0.0,
      double abs_tol_temp_veg = 0.0,
      double abs_tol_wat_veg = 0.0,
      double abs_tol_temp_soil_snow = 0.0,
      double abs_tol_wat_snow = 0.0,
      double abs_tol_matric = 0.0,
      double abs_tol_aquifr = 0.0,
      bool default_tol = false)
      : print_output_(print_output),
        output_frequency_(output_frequency),
        be_steps_(be_steps),
        rel_tol_(rel_tol),
        rel_tol_temp_cas_(rel_tol_temp_cas),
        rel_tol_temp_veg_(rel_tol_temp_veg),
        rel_tol_wat_veg_(rel_tol_wat_veg),
        rel_tol_temp_soil_snow_(rel_tol_temp_soil_snow),
        rel_tol_wat_snow_(rel_tol_wat_snow),
        rel_tol_matric_(rel_tol_matric),
        rel_tol_aquifr_(rel_tol_aquifr),
        abs_tol_(abs_tol),
        abs_tolWat_(abs_tolWat_),
        abs_tolNrg_(abs_tolNrg_),
        abs_tol_temp_cas_(abs_tol_temp_cas),
        abs_tol_temp_veg_(abs_tol_temp_veg),
        abs_tol_wat_veg_(abs_tol_wat_veg),
        abs_tol_temp_soil_snow_(abs_tol_temp_soil_snow),
        abs_tol_wat_snow_(abs_tol_wat_snow),
        abs_tol_matric_(abs_tol_matric),
        abs_tol_aquifr_(abs_tol_aquifr),
        default_tol_(default_tol) {};

  ~HRUActorSettings() {};

  std::string toString()
  {
    std::string str = "HRU Actor Settings:\n";
    str += "Print Output: " + std::to_string(print_output_) + "\n";
    str += "Output Frequency: " + std::to_string(output_frequency_) + "\n";
    str += "BE Steps: " + std::to_string(be_steps_) + "\n";
    str += "Abs Tol Water: " + std::to_string(abs_tolWat_) + "\n";
    str += "Abs Tol Energy: " + std::to_string(abs_tolNrg_) + "\n";
    str += "Rel Tol: " + std::to_string(rel_tol_) + "\n";
    str += "Specific Tolerances:\n";
    str += "Rel Tol Temp Veg: " + std::to_string(rel_tol_temp_veg_) + "\n";
    str += "Rel Tol Temp Cas: " + std::to_string(rel_tol_temp_cas_) + "\n";
    str += "Rel Tol Wat Veg: " + std::to_string(rel_tol_wat_veg_) + "\n";
    str += "Rel Tol Temp Soil Snow: " + std::to_string(rel_tol_temp_soil_snow_) + "\n";
    str += "Rel Tol Wat Snow: " + std::to_string(rel_tol_wat_snow_) + "\n";
    str += "Rel Tol Matric: " + std::to_string(rel_tol_matric_) + "\n";
    str += "Rel Tol Aquifr: " + std::to_string(rel_tol_aquifr_) + "\n";
    str += "Abs Tol Temp Cas: " + std::to_string(abs_tol_temp_cas_) + "\n";
    str += "Abs Tol Temp Veg: " + std::to_string(abs_tol_temp_veg_) + "\n";
    str += "Abs Tol Wat Veg: " + std::to_string(abs_tol_wat_veg_) + "\n";
    str += "Abs Tol Temp Soil Snow: " + std::to_string(abs_tol_temp_soil_snow_) + "\n";
    str += "Abs Tol Wat Snow: " + std::to_string(abs_tol_wat_snow_) + "\n";
    str += "Abs Tol Matric: " + std::to_string(abs_tol_matric_) + "\n";
    str += "Abs Tol Aquifr: " + std::to_string(abs_tol_aquifr_) + "\n";
    str += "Default Tolerances: " + std::to_string(default_tol_) + "\n";
    return str;
  }

  // PUP method for Charm++ serialization
  template <typename PUPER>
  void pup(PUPER &p) {
    p | print_output_;
    p | output_frequency_;
    p | be_steps_;
    p | rel_tol_;
    p | rel_tol_temp_cas_;
    p | rel_tol_temp_veg_;
    p | rel_tol_wat_veg_;
    p | rel_tol_temp_soil_snow_;
    p | rel_tol_wat_snow_;
    p | rel_tol_matric_;
    p | rel_tol_aquifr_;
    p | abs_tol_;
    p | abs_tolWat_;
    p | abs_tolNrg_;
    p | abs_tol_temp_cas_;
    p | abs_tol_temp_veg_;
    p | abs_tol_wat_veg_;
    p | abs_tol_temp_soil_snow_;
    p | abs_tol_wat_snow_;
    p | abs_tol_matric_;
    p | abs_tol_aquifr_;
    p | default_tol_;
  }
};