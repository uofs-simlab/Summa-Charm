#pragma once
#include "pup.h"

struct NumGRUInfo {
  int start_gru_local;
  int start_gru_global; 
  int num_gru_local;
  int num_gru_global;
  int file_gru; 
  bool use_global_for_data_structures;

  // Constructor
  NumGRUInfo(int start_gru_local = 0, int start_gru_global= 0, 
             int num_gru_local = 0, int num_gru_global = 0, int file_gru = 0, 
             bool use_global_for_data_structures = false) 
      : start_gru_local(start_gru_local), start_gru_global(start_gru_global), 
        num_gru_local(num_gru_local), num_gru_global(num_gru_global), 
        file_gru(file_gru), 
        use_global_for_data_structures(use_global_for_data_structures) {}

  // PUP method for Charm++ serialization
  void pup(PUP::er &p) {
    p | start_gru_local;
    p | start_gru_global;
    p | num_gru_local;
    p | num_gru_global;
    p | file_gru;
    p | use_global_for_data_structures;
  }
};
