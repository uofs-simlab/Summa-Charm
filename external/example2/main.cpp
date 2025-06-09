#include "example2.decl.h"
#include "helper.h"

class Main : public CBase_Main {
 public:
  Main(CkArgMsg* msg) {
    CkPrintf("=== Complex Helper Example Started ===\n");

    // Create a DataContainer with some sample data
    std::vector<int> numbers = {10, 20, 30, 40, 50};
    DataContainer data("SampleData", numbers, 2.5);
    
    // Create a Configuration
    Configuration config(1000, true, "advanced_algorithm");
    
    // Create the Helper with nested objects
    Helper h(data, config, 1);
    
    CkPrintf("Main chare: Creating Helper with complex nested objects\n");
    h.processData();
    
    // Create multiple workers with different modifications to the helper
    CkPrintf("\nCreating multiple workers with modified helpers...\n");
    
    // Worker 1: Original helper
    CProxy_Worker::ckNew(h);
    
    // Worker 2: Scaled helper
    Helper h2 = h;
    h2.updateProcessingId(2);
    h2.scaleData(0.5);
    CProxy_Worker worker2 = CProxy_Worker::ckNew(h2);
    
    // Worker 3: Different data
    std::vector<int> numbers3 = {5, 15, 25};
    DataContainer data3("ModifiedData", numbers3, 3.0);
    Configuration config3(500, false, "simple_algorithm");
    Helper h3(data3, config3, 3);
    CProxy_Worker worker3 = CProxy_Worker::ckNew(h3);
    
    // Create a coordinator to manage the workers
    CProxy_Coordinator::ckNew(h, 3);

    delete msg;
  }
};

class Worker : public CBase_Worker {
 private:
  Helper myHelper;
  
 public:
  Worker(Helper h) : myHelper(h) {
    CkPrintf("\n--- Worker %d Created ---\n", myHelper.processingId);
    myHelper.processData();
    
    // Simulate some processing time and modification
    myHelper.scaleData(1.1);  // Increase multiplier by 10%
    
    CkPrintf("Worker %d: After processing, new sum = %.2f\n", 
             myHelper.processingId, myHelper.data.calculateSum());
    
    // Send a message to demonstrate inter-chare communication
    processHelper(myHelper);
  }
  
  void processHelper(Helper h) {
    CkPrintf("Worker %d: Received helper for additional processing\n", h.processingId);
    h.processData();
    
    // Notify coordinator about completion
    double result = h.data.calculateSum();
    CProxy_Coordinator coord = CProxy_Coordinator::ckNew(h, 1);  // This would normally be stored
    // coord.receiveResult(h.processingId, result);  // Simplified for this example
    
    // Exit after processing all workers
    static int workerCount = 0;
    workerCount++;
    if (workerCount >= 3) {
      CkPrintf("\n=== All workers completed ===\n");
      CkExit();
    }
  }
};

class Coordinator : public CBase_Coordinator {
 private:
  Helper baseHelper;
  int totalWorkers;
  int completedWorkers;
  
 public:
  Coordinator(Helper h, int numWorkers) 
    : baseHelper(h), totalWorkers(numWorkers), completedWorkers(0) {
    CkPrintf("\n=== Coordinator Created ===\n");
    CkPrintf("Managing %d workers with base helper:\n", totalWorkers);
    baseHelper.processData();
  }
  
  void receiveResult(int workerId, double result) {
    completedWorkers++;
    CkPrintf("Coordinator: Received result %.2f from worker %d (%d/%d completed)\n", 
             result, workerId, completedWorkers, totalWorkers);
    
    if (completedWorkers >= totalWorkers) {
      CkPrintf("Coordinator: All workers completed!\n");
      CkExit();
    }
  }
};

#include "example2.def.h"
