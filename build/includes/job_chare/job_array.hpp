#pragma once

#include <chrono>
#include <string>
#include <vector>
#include <memory>
#include "JobArray.decl.h"
#include "FileAccessChare.decl.h"  // Need this for CProxy_FileAccessChare
#include "timing_info.hpp"
#include "gru_struc.hpp"
#include "file_access_chare.hpp"
#include "settings_functions.hpp"  // For FileAccessActorSettings
#include "num_gru_info.hpp"  // For NumGRUInfo

// For HOST_NAME_MAX
#include <limits.h>
#include <unistd.h>
#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 255
#endif

// Forward declaration for Batch - we'll check if this exists later
class Batch;

class JobArray : public CBase_JobArray
{
public:
    // Simplified constructor - takes batch and the chare ID
    JobArray(Batch batch, CkChareID summa_chare_proxy, int file_gru);

    // Entry methods from JobArray.ci
    void initializeBatch(Batch batch);
    void processGRU(int gru_id);
    void finalize();
    void fileAccessReady(int num_steps);  // New callback from FileAccessChare
    void handleError(int err_code, std::string err_msg);  // Error handler

    // PUP serialization method
    void pup(PUP::er &p);

private:
    // Basic member variables for simplified implementation
    Batch batch_;
    char hostname_[HOST_NAME_MAX];
    int file_gru_;  // File GRU parameter from SummaChare
    
    // Timing information 
    TimingInfo timing_info_;
    
    // Basic settings
    bool enable_logging_;
    bool default_tol_;
    
    // Core SUMMA structures (from CAF JobActor)
    std::unique_ptr<GruStruc> gru_struc_;
    // std::unique_ptr<SummaInitStruc> summa_init_struc_;  // Comment out for now
    
    // FileAccessChare proxy and settings
    CProxy_FileAccessChare file_access_chare_;
    NumGRUInfo num_gru_info_;
    FileAccessActorSettings fa_settings_;
    
    // Simulation state
    int num_steps_;
    int timestep_;
    
    // Tolerance values (from CAF JobActor)
    double rel_tol_;
    double abs_tol_;

    // SummaChare proxy as a CkChareID
    CkChareID summa_chare_proxy_;
};
