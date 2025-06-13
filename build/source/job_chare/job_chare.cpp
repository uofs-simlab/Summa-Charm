#include "job_chare.hpp"
#include "gru_struc.hpp"
#include "FileAccessChare.decl.h"
#include "SummaChare.decl.h"  

// #include "summa_init_struc.hpp"  // Comment out for now since it's missing
#include <unistd.h>
#include <limits.h>

JobChare::JobChare(Batch batch, CkChareID summa_chare_proxy, int file_gru)
    : batch_(batch), file_gru_(file_gru), enable_logging_(false), default_tol_(true),
      num_steps_(0), timestep_(1), rel_tol_(0.0), abs_tol_(0.0),
      summa_chare_proxy_(summa_chare_proxy)
{
    CkPrintf("JobChare: Started initialization for batch with %d GRUs starting at %d\n", 
             batch_.getNumHRU(), batch_.getStartHRU());
    
    // Get hostname for logging
    gethostname(hostname_, HOST_NAME_MAX);
    
    // Initialize based on CAF JobActor::make_behavior() pattern
    initializeBatch(batch);
}

void JobChare::initializeBatch(Batch batch)
{
    std::string err_msg;
    CkPrintf("JobChare: Starting initialization process...\n");

    // Timing Information (following CAF JobActor pattern)
    timing_info_ = TimingInfo();
    timing_info_.addTimePoint("total_duration");
    timing_info_.updateStartPoint("total_duration");
    timing_info_.addTimePoint("init_duration");
    timing_info_.updateStartPoint("init_duration");
    
    CkPrintf("JobChare: Timing info initialized\n");

    // Create Loggers (simplified)
    if (enable_logging_) {
        CkPrintf("JobChare: Logging enabled (placeholder)\n");
    } else {
        CkPrintf("JobChare: Logging disabled\n");
    }

    // For now, set default tolerances
    rel_tol_ = 1e-6;
    abs_tol_ = 1e-6;
    CkPrintf("JobChare: Using default tolerances: rel_tol=%e, abs_tol=%e\n", 
             rel_tol_, abs_tol_);

    // Create NumGRUInfo (from CAF JobActor::make_behavior())
    // Use the actual file_gru parameter passed from SummaChare
    num_gru_info_ = NumGRUInfo(batch_.getStartHRU(), batch_.getStartHRU(), 
                               batch_.getNumHRU(), batch_.getNumHRU(), 
                               file_gru_, false);  // Use actual file_gru

    // Set FileAccessChare settings (from CAF JobActor)
    fa_settings_ = FileAccessActorSettings(1, 2);  // Default values from CAF

    // Create FileAccessChare (following CAF JobActor pattern)
    CkPrintf("JobChare: Creating FileAccessChare...\n");
    file_access_chare_ = CProxy_FileAccessChare::ckNew(num_gru_info_, fa_settings_);
    
    // Initialize FileAccessChare with our proxy so it can call us back
    file_access_chare_.initFileAccessChare(file_gru_, batch_.getNumHRU(), thisProxy);  // Use actual file_gru
    
    CkPrintf("JobChare: FileAccessChare spawned, waiting for initialization...\n");
}

// New entry method for FileAccessChare callback
void JobChare::fileAccessReady(int num_steps)
{
    CkPrintf("JobChare: FileAccessChare ready with %d timesteps\n", num_steps);
    
    if (num_steps < 0) {
        std::string err_msg = "ERROR: JobChare: FileAccessChare initialization failed\n";
        CkPrintf("JobChare: %s", err_msg.c_str());
        handleError(-2, err_msg);
        return;
    }
    
    num_steps_ = num_steps;
    
    timing_info_.updateEndPoint("init_duration");
    CkPrintf("JobChare: Full initialization completed successfully!\n");
    
    // Report completion
    double init_duration = timing_info_.getDuration("init_duration").value_or(-1.0);
    CkPrintf("JobChare: Initialization took %f seconds\n",  init_duration);
    
    // Now ready for simulation - in full implementation would spawn GRU actors
    CkPrintf("JobChare: Ready for simulation with %d steps\n",  num_steps_);
    
    // For now, just proceed to finalize to test the workflow
    finalize();
}

// Entry method for error handling
void JobChare::handleError(int err_code, std::string err_msg)
{
    CkPrintf("JobChare: Error %d: %s\n",  err_code, err_msg.c_str());
    // TODO: Implement proper error handling and cleanup
    finalize();
}

// Entry method implementation for processGRU
void JobChare::processGRU(int gru_id)
{
    CkPrintf("JobChare: Processing GRU %d (placeholder)\n", gru_id);
    
    // For now, this is a placeholder implementation
    // In a full implementation, this would trigger GRU simulation
    // For testing purposes, we'll just report completion
    finalize();
}

// Entry method implementation for finalize
void JobChare::finalize()
{
    timing_info_.updateEndPoint("total_duration");
    
    CkPrintf("JobChare: Finalizing...\n");
    
    double total_duration = timing_info_.getDuration("total_duration").value_or(-1.0);
    double init_duration = timing_info_.getDuration("init_duration").value_or(-1.0);
    
    CkPrintf("JobChare: Timing results:\n");
    CkPrintf("  Total Duration: %f seconds\n", total_duration);
    CkPrintf("  Init Duration: %f seconds\n", init_duration);
    
    CkPrintf("JobChare: Finalization completed\n");

    // Notify the SummaChare that we're done
    CkPrintf("JobChare: Notifying SummaChare of completion...\n");
   
    CProxy_SummaChare summa_chare(summa_chare_proxy_);
    summa_chare.doneJob(0, total_duration, init_duration, 0.0);  // Placeholder values
   
}

void JobChare::pup(PUP::er &p) {
    CBase_JobChare::pup(p);

    // Serialize basic types only for now
    p | batch_;
    p | file_gru_;
    p | enable_logging_;
    p | default_tol_;
    p | num_steps_;
    p | timestep_;
    p | rel_tol_;
    p | abs_tol_;
    p | num_gru_info_;
    p | fa_settings_;
    
    // Note: timing_info_, gru_struc_, and other complex objects
    // need special handling for migration - commented out for now
    // p | timing_info_;
}

#include "JobChare.def.h"