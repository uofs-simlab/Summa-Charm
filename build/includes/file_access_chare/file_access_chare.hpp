#pragma once

#include "charm++.h"
#include "pup_stl.h"
#include <memory>
#include <vector>
#include "num_gru_info.hpp"  // Include the global NumGRUInfo definition
#include "forcing_file_info.hpp"  // Include forcing file support

// Forward declarations
class OutputBuffer;
class TimingInfo;
class FileAccessActorSettings;  // Defined in settings_functions.hpp

// Fortran interface functions
extern "C" {
    void f_getNumTimeSteps(int& num_timesteps);
    void writeRestart_fortran(void* handle_ncid, int& start_gru, int& max_gru, 
                              int& timestep, int& year, int& month, int& day, 
                              int& hour, int& err);
}

// Forward declaration - actual class will be defined in the .cpp file after including .decl.h
class FileAccessChare;