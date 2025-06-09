#ifndef HELPER_H
#define HELPER_H

#include "pup.h"

class Helper {
 public:
  int value;

  Helper(int v = 0) : value(v) {}
  
  // PUP routine for serialization
  void pup(PUP::er &p) {
    p | value;
  }
};

#endif
