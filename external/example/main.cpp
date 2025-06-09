#include "example.decl.h"
#include "helper.h"

class Main : public CBase_Main {
 public:
  Main(CkArgMsg* msg) {
    CkPrintf("Main chare initialized.\n");

    Helper h(42); // Create the Helper instance locally

    CkPrintf("Creating Worker with Helper.value = %d\n", h.value);

    CProxy_Worker::ckNew(h);  // Send the actual Helper object

    delete msg;
  }
};

class Worker : public CBase_Worker {
 public:
  Worker(Helper h) {  // Receive the actual Helper object
    CkPrintf("Worker chare created. Received Helper with value = %d\n", h.value);
    
    // You can now use the helper object directly
    h.value += 10;  // Modify it
    CkPrintf("Modified Helper value to: %d\n", h.value);
    
    CkExit();  // Exit the program
  }
};

#include "example.def.h"
