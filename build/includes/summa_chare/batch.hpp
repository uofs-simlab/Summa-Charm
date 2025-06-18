#pragma once
#include <string>
#include <iostream>
#include <fstream>
#include "charm++.h"
#include "pup.h"        // For Charm++ serialization
#include "pup_stl.h"    // For STL serialization

class Batch {
  private:
    int batch_id_;
    int start_hru_;
    int num_hru_;
    double run_time_;
    double read_time_;
    double write_time_;
    bool assigned_to_actor_;
    bool solved_;

    std::string log_dir_; // Directory to write log files

  public:
    Batch(int batch_id = -1, int start_hru = -1, int num_hru = -1);
        
    // Getters
    inline int getBatchID() const { return batch_id_; }
    inline int getStartHRU() const { return start_hru_;}
    inline int getNumHRU() const { return num_hru_;}
    inline std::string getLogDir() const { return log_dir_; }
    double getRunTime();
    double getReadTime();
    double getWriteTime();
    bool isAssigned();
    bool isSolved();
    std::string getBatchInfoString();
    // Setters
    inline void setLogDir(std::string log_dir) { log_dir_ = log_dir; }
    void updateRunTime(double run_time);
    void updateReadTime(double read_time);
    void updateWriteTime(double write_time);
    void updateAssigned(bool boolean);
    void updateSolved(bool boolean);
    void printBatchInfo();
    void writeBatchToFile(std::string csv_output, std::string hostname);

    std::string toString();

    // specific method - commented out for Charm++ version
    void assignToActor(std::string hostname, CkChareID assigned_actor);


    // Charm++ PUP serialization method
    void pup(PUP::er &p) {
        p | batch_id_;
        p | start_hru_;
        p | num_hru_;
        p | run_time_;
        p | read_time_;
        p | write_time_;
        p | assigned_to_actor_;
        p | solved_;
        p | log_dir_;
    }
};