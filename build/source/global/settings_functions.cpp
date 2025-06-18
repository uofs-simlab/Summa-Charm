#include "settings_functions.hpp"

void Settings::generateConfigFile() {
    using json = nlohmann::ordered_json;
    json config_file; 
    config_file["Distributed_Settings"] = {
        {"distributed_mode", false},
        {"port", MISSING_INT},
        {"total_hru_count", MISSING_INT},
        {"num_hru_per_batch", MISSING_INT},
        {"load_balancing", false},
        {"num_nodes", MISSING_INT},
        {"servers_list", {
            {{"hostname", "host_1"}},
            {{"hostname", "host_2"}},
            {{"hostname", "host_3"}}
        }}
    };

    config_file["Summa_Actor"] = {
        {"max_gru_per_job", GRU_PER_JOB},
        {"enable_logging", false},
        {"log_dir", ""}
    };
    config_file["File_Access_Actor"] = {
        {"num_partitions_in_output_buffer", NUM_PARTITIONS},
        {"num_timesteps_in_output_buffer", OUTPUT_TIMESTEPS}
    };
    config_file["Job_Actor"] = {
        {"file_manager_path", "/home/username/summa_file_manager"},
        {"max_run_attempts", 1},
        {"data_assimilation_mode", false},
        {"batch_size", 10}
    };
    config_file["HRU_Actor"] = {
        {"print_output", true},
        {"output_frequency", OUTPUT_FREQUENCY}
    };

    std::ofstream config_file_stream("config.json");
    config_file_stream << std::setw(4) << config_file.dump(2) << std::endl;
    config_file_stream.close();
}