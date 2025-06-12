#include "file_access_chare.hpp"
#include "settings_functions.hpp"  // For FileAccessActorSettings
#include "pup_stl.h"  // For STL serialization
#include "FileAccessChare.decl.h"

// Now define the inheritance after the base class is available
class FileAccessChare : public CBase_FileAccessChare {
private:
    NumGRUInfo num_gru_info_;
    FileAccessActorSettings fa_settings_;
    
    int start_gru_;
    int num_gru_;
    int num_hru_;
    int num_steps_;
    
    bool write_params_flag_ = true;
    
    // Output handling
    // std::unique_ptr<OutputBuffer> output_buffer_;
    // std::unique_ptr<forcingFileContainer> forcing_files_;
    
    // Checkpointing variables
    int completed_checkpoints_ = 1;
    std::vector<int> hru_checkpoints_;
    std::vector<int> hru_timesteps_;

public:
    FileAccessChare(NumGRUInfo num_gru_info, FileAccessActorSettings fa_settings);
    FileAccessChare(CkMigrateMessage* msg) : num_gru_info_(), fa_settings_() {}

    void initFileAccessChare(int file_gru, int num_hru, CkCallback cb);
    void accessForcing(int iFile);
    void restartFailures();
    void finalize();
    void error(int err_code, std::string err_msg);

    // Migration support
    void pup(PUP::er &p) override {
        CBase_FileAccessChare::pup(p);
        p | num_gru_info_;
        p | fa_settings_;
        p | start_gru_;
        p | num_gru_;
        p | num_hru_;
        p | num_steps_;
        p | write_params_flag_;
        p | completed_checkpoints_;
        p | hru_checkpoints_;
        p | hru_timesteps_;
    }
};

FileAccessChare::FileAccessChare(NumGRUInfo num_gru_info, FileAccessActorSettings fa_settings)
    : num_gru_info_(num_gru_info), fa_settings_(fa_settings) {
    
    CkPrintf("FileAccessChare: Started\n");
    
    // Set GRU info based on settings
    if (num_gru_info_.use_global_for_data_structures) {
        start_gru_ = num_gru_info_.start_gru_global;
        num_gru_ = num_gru_info_.num_gru_global;
    } else {
        start_gru_ = num_gru_info_.start_gru_local;
        num_gru_ = num_gru_info_.num_gru_local;
    }
}

void FileAccessChare::initFileAccessChare(int file_gru, int num_hru, CkCallback cb) {
    CkPrintf("FileAccessChare: Initializing with file_gru=%d, num_hru=%d\n", file_gru, num_hru);
    
    num_hru_ = num_hru;
    
    // Get number of time steps from Fortran
    f_getNumTimeSteps(num_steps_);
    CkPrintf("FileAccessChare: Number of timesteps = %d\n", num_steps_);
    
    // TODO: Initialize forcing files
    // forcing_files_ = std::make_unique<forcingFileContainer>();
    // if (forcing_files_->initForcingFiles() != 0) {
    //     CkPrintf("FileAccessChare: Error initializing forcing files\n");
    //     cb.send(CkReductionMsg::buildNew(sizeof(int), &num_steps_));
    //     return;
    // }
    
    // TODO: Initialize output buffer
    // output_buffer_ = std::make_unique<OutputBuffer>(fa_settings_, num_gru_info_, num_hru_, num_steps_);
    // int chunk_return = output_buffer_->setChunkSize();
    // CkPrintf("FileAccessChare: Chunk Size = %d\n", chunk_return);
    
    // int err = output_buffer_->defOutput(std::to_string(thisIndex));
    // if (err != 0) {
    //     CkPrintf("FileAccessChare: Error defOutput - Can't define output file\n");
    //     num_steps_ = -1;
    //     cb.send(CkReductionMsg::buildNew(sizeof(int), &num_steps_));
    //     return;
    // }
    
    // err = output_buffer_->allocateOutputBuffer(num_steps_);
    // if (err != 0) {
    //     CkPrintf("FileAccessChare: Error allocating output buffer\n");
    //     num_steps_ = -1;
    // }
    
    CkPrintf("FileAccessChare: Initialization complete\n");
    
    // Return number of steps via callback
    cb.send(CkReductionMsg::buildNew(sizeof(int), &num_steps_));
}

void FileAccessChare::accessForcing(int iFile) {
    CkPrintf("FileAccessChare: Accessing forcing file %d\n", iFile);
    // TODO: Implement forcing file access
}

void FileAccessChare::restartFailures() {
    CkPrintf("FileAccessChare: Handling restart failures\n");
    // TODO: Implement restart failure handling
}

void FileAccessChare::finalize() {
    CkPrintf("FileAccessChare: Finalizing\n");
    // TODO: Implement finalization
}

void FileAccessChare::error(int err_code, std::string err_msg) {
    CkPrintf("FileAccessChare: Error %d: %s\n", err_code, err_msg.c_str());
    // TODO: Implement error handling
}

#include "FileAccessChare.def.h"