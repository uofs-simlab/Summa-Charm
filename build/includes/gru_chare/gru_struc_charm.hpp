#pragma once

#include <vector>
#include <memory>
#include <string>

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