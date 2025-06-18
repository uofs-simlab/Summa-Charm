#pragma once

#include <vector>
#include <memory>
#include <string>
#include "charm++.h"

// Forward declarations
extern "C" {
    void f_readDimension(int start_gru, int num_gru, int& file_gru, int& file_hru, 
                        int& err, char* err_msg);
    void f_setHruCount(int i, int start_gru);
    void f_setIndexMap();
    void f_getNumHru(int& num_hru);
    void f_readIcondNlayers(int num_gru, int& err, char* err_msg);
    void f_getNumHruPerGru(int num_gru, int& num_hru_per_gru_first);
}

// Enum for GRU state
enum class gru_state {
    pending,
    running,
    succeeded,
    failed,
    retry
};


/** Gru Information (meant to mimic gru_struc)*/
class GRU {
  private:
    int index_netcdf_;       // The index of the GRU in the netcdf file
    int index_job_;          // The index of the GRU within this job
    CkChareID actor_ref_;   // The actor for the GRU

    int num_hrus_;           // The number of HRUs in the GRU

    // Modifyable Parameters
    int dt_init_factor_;     // The initial dt for the GRU
    double rel_tol_;         // The relative tolerance for the GRU
    double abs_tol_;         // The absolute tolerance for the GRU

    // Status Information
    int attempts_left_;      // The number of attempts left for the GRU to succeed
    gru_state state_;        // The state of the GRU

    // Timing Information
    double run_time_ = 0.0;  // The total time to run the GRU

    
  public:
    // Constructor
    GRU(int index_netcdf, int index_job,
        int dt_init_factor, double rel_tol, double abs_tol, int max_attempts) 
        : index_netcdf_(index_netcdf), index_job_(index_job), 
          dt_init_factor_(dt_init_factor),
          rel_tol_(rel_tol), abs_tol_(abs_tol), attempts_left_(max_attempts),
          state_(gru_state::running) {};

    // Deconstructor
    ~GRU() {};

    // Getters
    inline int getIndexNetcdf() const { return index_netcdf_; }
    inline int getIndexJob() const { return index_job_; }
    inline CkChareID getActorRef() const { return actor_ref_; }
    inline double getRunTime() const { return run_time_; }
    inline double getRelTol() const { return rel_tol_; }
    inline double getAbsTol() const { return abs_tol_; }
    inline int getAttemptsLeft() const { return attempts_left_; }
    inline gru_state getStatus() const { return state_; }

    // Setters
    inline void setRunTime(double run_time) { run_time_ = run_time; }
    inline void setRelTol(double rel_tol) { rel_tol_ = rel_tol; }
    inline void setAbsTol(double abs_tol) { abs_tol_ = abs_tol; }
    inline void setSuccess() { state_ = gru_state::succeeded; }
    inline void setFailed() { state_ = gru_state::failed; }
    inline void setRunning() { state_ = gru_state::running; }

    // Methods
    inline bool isFailed() const { return state_ == gru_state::failed; }
    inline void decrementAttemptsLeft() { attempts_left_--; }
    inline void setActorRef(CkChareID gru_actor) { actor_ref_ = gru_actor; }
};

// Structure for node GRU information
struct NodeGruInfo {
    int node_start_gru_;
    int start_gru_;
    int node_num_gru_;
    int num_gru_;
    int file_gru_;
    
    NodeGruInfo(int node_start_gru, int start_gru, int node_num_gru, 
                int num_gru, int file_gru)
        : node_start_gru_(node_start_gru), start_gru_(start_gru),
          node_num_gru_(node_num_gru), num_gru_(num_gru), file_gru_(file_gru) {}
};

// Forward declaration for GRU info class
class GRUInfo {
public:
    virtual ~GRUInfo() = default;
    virtual gru_state getStatus() const = 0;
    virtual int getIndexJob() const = 0;
};

// Main GruStruc class
class GruStruc {
public:
    // Constructor
    GruStruc(int start_gru, int num_gru, int num_retry_attempts);
    
    // Destructor
    virtual ~GruStruc() = default;
    
    // Public methods
    int readDimension();
    int readIcondNlayers();
    int getFailedIndex();
    void getNumHrusPerGru();
    int setNodeGruInfo(int num_nodes);
    std::string getNodeGruInfoString();
    
    // Getters
    int getStartGru() const { return start_gru_; }
    int getNumGru() const { return num_gru_; }
    int getFileGru() const { return file_gru_; }
    int getFileHru() const { return file_hru_; }
    int getNumHru() const { return num_hru_; }
    int getRetryAttemptsLeft() const { return num_retry_attempts_left_; }
    
    const std::vector<int>& getNumHruPerGru() const { return num_hru_per_gru_; }
    const std::vector<NodeGruInfo>& getNodeGruInfo() const { return node_gru_info_; }
    const std::vector<std::unique_ptr<GRUInfo>>& getGruInfo() const { return gru_info_; }
    
    // Setters/Modifiers
    void decrementRetryAttempts() { if (num_retry_attempts_left_ > 0) num_retry_attempts_left_--; }

private:
    // Member variables
    int start_gru_;
    int num_gru_;
    int file_gru_;
    int file_hru_;
    int num_hru_;
    int num_retry_attempts_left_;
    
    std::vector<int> num_hru_per_gru_;
    std::vector<NodeGruInfo> node_gru_info_;
    std::vector<std::unique_ptr<GRUInfo>> gru_info_;
};