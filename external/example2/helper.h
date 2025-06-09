#ifndef HELPER_H
#define HELPER_H

#include "pup.h"
#include "data.h"

class Helper {
 public:
  DataContainer data;
  Configuration config;
  int processingId;
  
  Helper() : processingId(0) {}
  
  Helper(const DataContainer& d, const Configuration& c, int id = 0) 
    : data(d), config(c), processingId(id) {}
  
  // PUP routine for serialization
  void pup(PUP::er &p) {
    p | data;
    p | config;
    p | processingId;
  }
  
  // Method to process the data
  void processData() {
    CkPrintf("=== Processing Helper (ID: %d) ===\n", processingId);
    data.printData();
    config.printConfig();
    
    double result = data.calculateSum();
    CkPrintf("Calculated sum: %.2f\n", result);
    
    if (config.verbose) {
      CkPrintf("Verbose mode: Processing with algorithm '%s'\n", config.algorithm.c_str());
    }
    
    CkPrintf("================================\n");
  }
  
  // Method to modify the helper
  void updateProcessingId(int newId) {
    processingId = newId;
  }
  
  void scaleData(double factor) {
    data.multiplier *= factor;
  }
};

#endif
