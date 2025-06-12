#include "job_array.hpp"
#include "gru_struc_charm.hpp"
#include "FileAccessChare.decl.h"
// #include "summa_init_struc.hpp"  // Comment out for now since it's missing
#include <unistd.h>
#include <limits.h>

JobArray::JobArray(Batch batch)
    : batch_(batch), enable_logging_(false), default_tol_(true),
      num_steps_(0), timestep_(1), rel_tol_(0.0), abs_tol_(0.0)
{
    CkPrintf("JobArray[%d]: Started initialization for batch with %d GRUs starting at %d\n", 
             thisIndex, batch_.getNumHRU(), batch_.getStartHRU());
    
    // Get hostname for logging
    gethostname(hostname_, HOST_NAME_MAX);
    
    // Initialize based on CAF JobActor::make_behavior() pattern
    initializeBatch(batch);
}

void JobArray::initializeBatch(Batch batch)
{
    std::string err_msg;
    CkPrintf("JobArray[%d]: Starting initialization process...\n", thisIndex);

    // Timing Information (following CAF JobActor pattern)
    timing_info_ = TimingInfo();
    timing_info_.addTimePoint("total_duration");
    timing_info_.updateStartPoint("total_duration");
    timing_info_.addTimePoint("init_duration");
    timing_info_.updateStartPoint("init_duration");
    
    CkPrintf("JobArray[%d]: Timing info initialized\n", thisIndex);

    // Create Loggers (simplified)
    if (enable_logging_) {
        CkPrintf("JobArray[%d]: Logging enabled (placeholder)\n", thisIndex);
    } else {
        CkPrintf("JobArray[%d]: Logging disabled\n", thisIndex);
    }

    // GruStruc Initialization (from CAF JobActor::make_behavior())
    CkPrintf("JobArray[%d]: About to initialize GruStruc...\n", thisIndex);
    
    // TODO: Temporarily disable GruStruc initialization to isolate the crash
    /*
    gru_struc_ = std::make_unique<GruStruc>(batch_.getStartHRU(), 
        batch_.getNumHRU(), 1); // max_run_attempts = 1 for simplification

    if (gru_struc_->readDimension()) {
        err_msg = "ERROR: JobArray - ReadDimension\n";
        CkPrintf("JobArray[%d]: %s", thisIndex, err_msg.c_str());
        return;
    }

    if (gru_struc_->readIcondNlayers()) {
        err_msg = "ERROR: JobArray - ReadIcondNlayers\n";
        CkPrintf("JobArray[%d]: %s", thisIndex, err_msg.c_str());
        return;
    }

    gru_struc_->getNumHrusPerGru();
    CkPrintf("JobArray[%d]: GruStruc initialized successfully\n", thisIndex);
    */
    
    CkPrintf("JobArray[%d]: GruStruc initialization skipped for debugging\n", thisIndex);

    // SummaInitStruc Initialization (from CAF JobActor::make_behavior())
    // NOTE: Commenting out for now since we don't have SummaInitStruc in Charm version
    /*
    CkPrintf("JobArray[%d]: Initializing SummaInitStruc...\n", thisIndex);
    summa_init_struc_ = std::make_unique<SummaInitStruc>();
    
    if (summa_init_struc_->allocate(batch_.getNumHRU()) != 0) {
        err_msg = "ERROR -- JobArray: SummaInitStruc allocation failed\n";
        CkPrintf("JobArray[%d]: %s", thisIndex, err_msg.c_str());
        return;
    }
    
    if (summa_init_struc_->summa_paramSetup() != 0) {
        err_msg = "ERROR -- JobArray: SummaInitStruc paramSetup failed\n";
        CkPrintf("JobArray[%d]: %s", thisIndex, err_msg.c_str());
        return;
    }
    
    if (summa_init_struc_->summa_readRestart() != 0) {
        err_msg = "ERROR -- JobArray: SummaInitStruc readRestart failed\n";
        CkPrintf("JobArray[%d]: %s", thisIndex, err_msg.c_str());
        return;
    }
    
    // Get initial tolerances from SummaInitStruc
    summa_init_struc_->getInitTolerance(rel_tol_, abs_tol_);
    CkPrintf("JobArray[%d]: SummaInitStruc initialized successfully\n", thisIndex);
    */

    // For now, set default tolerances
    rel_tol_ = 1e-6;
    abs_tol_ = 1e-6;
    CkPrintf("JobArray[%d]: Using default tolerances: rel_tol=%e, abs_tol=%e\n", 
             thisIndex, rel_tol_, abs_tol_);

    // Create NumGRUInfo (from CAF JobActor::make_behavior())
    // For now, use placeholder values since we don't have GruStruc working
    num_gru_info_ = NumGRUInfo(batch_.getStartHRU(), batch_.getStartHRU(), 
                               batch_.getNumHRU(), batch_.getNumHRU(), 
                               1, false);  // placeholder file_gru = 1

    // Set FileAccessChare settings (from CAF JobActor)
    fa_settings_ = FileAccessActorSettings(1, 2);  // Default values from CAF

    // Create FileAccessChare (following CAF JobActor pattern)
    CkPrintf("JobArray[%d]: Creating FileAccessChare...\n", thisIndex);
    file_access_chare_ = CProxy_FileAccessChare::ckNew(num_gru_info_, fa_settings_);
    
    // Initialize FileAccessChare with our proxy so it can call us back
    file_access_chare_.initFileAccessChare(1, batch_.getNumHRU(), thisProxy);  // placeholder file_gru = 1
    
    CkPrintf("JobArray[%d]: FileAccessChare spawned, waiting for initialization...\n", thisIndex);
}

// New entry method for FileAccessChare callback
void JobArray::fileAccessReady(int num_steps)
{
    CkPrintf("JobArray[%d]: FileAccessChare ready with %d timesteps\n", thisIndex, num_steps);
    
    if (num_steps < 0) {
        std::string err_msg = "ERROR: JobArray: FileAccessChare initialization failed\n";
        CkPrintf("JobArray[%d]: %s", thisIndex, err_msg.c_str());
        handleError(-2, err_msg);
        return;
    }
    
    num_steps_ = num_steps;
    
    timing_info_.updateEndPoint("init_duration");
    CkPrintf("JobArray[%d]: Full initialization completed successfully!\n", thisIndex);
    
    // Report completion
    double init_duration = timing_info_.getDuration("init_duration").value_or(-1.0);
    CkPrintf("JobArray[%d]: Initialization took %f seconds\n", thisIndex, init_duration);
    
    // Now ready for simulation - in full implementation would spawn GRU actors
    CkPrintf("JobArray[%d]: Ready for simulation with %d steps\n", thisIndex, num_steps_);
    
    // For now, just proceed to finalize to test the workflow
    finalize();
}

// Entry method for error handling
void JobArray::handleError(int err_code, std::string err_msg)
{
    CkPrintf("JobArray[%d]: Error %d: %s\n", thisIndex, err_code, err_msg.c_str());
    // TODO: Implement proper error handling and cleanup
    finalize();
}

// Entry method implementation for processGRU
void JobArray::processGRU(int gru_id)
{
    CkPrintf("JobArray[%d]: Processing GRU %d (placeholder)\n", thisIndex, gru_id);
    
    // For now, this is a placeholder implementation
    // In a full implementation, this would trigger GRU simulation
    // For testing purposes, we'll just report completion
    finalize();
}

// Entry method implementation for finalize
void JobArray::finalize()
{
    timing_info_.updateEndPoint("total_duration");
    
    CkPrintf("JobArray[%d]: Finalizing...\n", thisIndex);
    
    double total_duration = timing_info_.getDuration("total_duration").value_or(-1.0);
    double init_duration = timing_info_.getDuration("init_duration").value_or(-1.0);
    
    CkPrintf("JobArray[%d]: Timing results:\n", thisIndex);
    CkPrintf("  Total Duration: %f seconds\n", total_duration);
    CkPrintf("  Init Duration: %f seconds\n", init_duration);
    
    CkPrintf("JobArray[%d]: Finalization completed\n", thisIndex);
}

void JobArray::pup(PUP::er &p) {
    CBase_JobArray::pup(p);

    // Serialize basic types only for now
    p | batch_;
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

#include "JobArray.def.h"