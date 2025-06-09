#ifndef DATA_H
#define DATA_H

#include "pup.h"
#include <string>
#include <vector>

class DataContainer {
 public:
  std::string name;
  std::vector<int> numbers;
  double multiplier;

  DataContainer() : name(""), multiplier(1.0) {}
  
  DataContainer(const std::string& n, const std::vector<int>& nums, double mult = 1.0) 
    : name(n), numbers(nums), multiplier(mult) {}
  
  // PUP routine for serialization
  void pup(PUP::er &p) {
    p | name;
    p | numbers;
    p | multiplier;
  }
  
  // Helper method to display the data
  void printData() const {
    CkPrintf("DataContainer: name='%s', multiplier=%.2f, numbers=[", name.c_str(), multiplier);
    for (size_t i = 0; i < numbers.size(); ++i) {
      CkPrintf("%d", numbers[i]);
      if (i < numbers.size() - 1) CkPrintf(", ");
    }
    CkPrintf("]\n");
  }
  
  // Method to calculate sum with multiplier
  double calculateSum() const {
    double sum = 0.0;
    for (int num : numbers) {
      sum += num;
    }
    return sum * multiplier;
  }
};

class Configuration {
 public:
  int maxIterations;
  bool verbose;
  std::string algorithm;
  
  Configuration() : maxIterations(100), verbose(false), algorithm("default") {}
  
  Configuration(int iter, bool v, const std::string& algo) 
    : maxIterations(iter), verbose(v), algorithm(algo) {}
  
  // PUP routine for serialization
  void pup(PUP::er &p) {
    p | maxIterations;
    p | verbose;
    p | algorithm;
  }
  
  void printConfig() const {
    CkPrintf("Configuration: algorithm='%s', maxIterations=%d, verbose=%s\n", 
             algorithm.c_str(), maxIterations, verbose ? "true" : "false");
  }
};

#endif
