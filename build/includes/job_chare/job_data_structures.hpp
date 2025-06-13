#pragma once

#include <chrono>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "JobChare.decl.h"
#include "timing_info.hpp"
#include "settings_functions.hpp"
#include "file_manager.hpp"
#include "batch_container.hpp"
#include "summa_global_data.hpp"
#include "gru_struc.hpp"
#include "summa_init_struc.hpp"
#include "num_gru_info.hpp"
#include "logger.hpp"

// For HOST_NAME_MAX
#include <limits.h>
#include <unistd.h>
#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 255
#endif

// Forward declarations
class GRU;
class FileAccessActor;

/*********************************************
 * Job Chare Data Structures
 *********************************************/
// Holds information about the GRUs (adapted from CAF version)
struct GRU_Container {
    std::vector<GRU*> gru_list;
    std::chrono::time_point<std::chrono::system_clock> gru_start_time;
    int num_gru_done = 0; 
    int num_gru_failed = 0; // number of grus that are waiting to be restarted
    int num_gru_in_run_domain = 0; // number of grus we are currently solving for
    int run_attempts_left = 1; // current run attempt for all grus
};

// Job state structure (adapted from CAF version)
struct job_chare_state {
    TimingInfo job_timing;
    std::unique_ptr<Logger> logger;
    std::unique_ptr<ErrorLogger> err_logger;
    std::unique_ptr<SuccessLogger> success_logger;
    
    // Chare References (adapted from actor references)
    CkChareID file_access_chare; // chare reference for the file_access_chare
    CkChareID parent;            // chare reference to the top-level SummaChare

    Batch batch; // Information about the number of HRUs and starting point 

    std::unique_ptr<GruStruc> gru_struc; 
    NumGRUInfo num_gru_info;
    GRU_Container gru_container;

    std::unique_ptr<SummaInitStruc> summa_init_struc;

    // Variables for GRU monitoring
    int dt_init_start_factor = 1; // Initial Factor for dt_init (coupled_em)
    int num_gru_done = 0;         // The number of GRUs that have completed
    int num_gru_failed = 0;       // Number of GRUs that have failed

    std::string hostname;

    FileAccessActorSettings file_access_actor_settings;
    JobActorSettings job_actor_settings; 
    HRUActorSettings hru_actor_settings;

    // Forcing information
    int iFile = 1; // index of current forcing file from forcing file list
    int stepsInCurrentFFile;
    int forcingStep = 1;
    int timestep = 1;
    int num_gru_done_timestep = 0;
    int num_steps = 0;
};

// Distributed job state structure (adapted from CAF version)
struct distributed_job_chare_state {
    TimingInfo job_timing;

    int file_gru;
    int start_gru;
    int num_gru;

    Batch batch;
    
    NumGRUInfo num_gru_info;
    std::vector<NumGRUInfo> node_num_gru_info;
    
    DistributedSettings distributed_settings;
    JobActorSettings job_actor_settings; 
    HRUActorSettings hru_actor_settings;
    FileAccessActorSettings file_access_actor_settings;

    std::vector<CkChareID> connected_nodes;

    std::vector<std::vector<double>> gru_times_per_node;
    std::vector<double> node_walltimes;

    std::chrono::time_point<std::chrono::system_clock> load_balance_start_time;
    std::chrono::time_point<std::chrono::system_clock> load_balance_end_time;
    double load_balance_time = 0.0;

    // Simplified mappings using vectors instead of unordered_maps
    // (to avoid CkChareID hash function requirements)
    std::vector<std::pair<CkChareID, CkChareID>> hru_to_node_pairs;
    std::vector<std::pair<CkChareID, double>> hru_walltimes_pairs;
    std::vector<std::pair<CkChareID, double>> node_walltimes_pairs;

    std::vector<std::pair<CkChareID, int>> hrus_to_balance;  // Using int instead of GRU for simplicity

    // Alternative: use maps instead of unordered_maps if ordering is acceptable
    std::map<int, std::vector<std::pair<int, int>>> node_to_hru_to_balance_map;  // Using int keys
    std::map<int, int> node_to_hru_to_balance_map_size; 

    int num_hrus_to_swap = 0; // We want to swap 25% of the HRUs

    // Forcing information
    int iFile = 1; // index of current forcing file from forcing file list
    int stepsInCurrentFFile;
    int forcingStep = 1;
    int timestep = 1;
    int num_gru_done_timestep = 0;
    int num_steps = 0;
};
