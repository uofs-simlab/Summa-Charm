#include "settings_functions.hpp"

void Settings::generateConfigFile() {
    using json = nlohmann::ordered_json;
    json config_file; 
    config_file["Summa_Chare"] = {
        {"max_gru_per_job", GRU_PER_JOB},
        {"enable_logging", false},
        {"log_dir", ""}
    };
    config_file["File_Access_Chare"] = {
        {"num_partitions_in_output_buffer", NUM_PARTITIONS},
        {"num_timesteps_in_output_buffer", OUTPUT_TIMESTEPS}
    };
    config_file["Job_Chare"] = {
        {"file_manager_path", "/home/username/summa_file_manager"},
        {"max_run_attempts", 1},
        {"data_assimilation_mode", false},
        {"batch_size", MISSING_INT}
    };
    config_file["HRU_Chare"] = {
        {"print_output", true},
        {"output_frequency", OUTPUT_FREQUENCY},
        {"abs_tol", 1e1},
        {"rel_tol", 1e1},
        {"rel_tol_temp_cas", 1e1},
        {"rel_tol_temp_veg", 1e1},
        {"rel_tol_wat_veg", 1e1},
        {"rel_tol_temp_soil_snow", 1e1},
        {"rel_tol_wat_snow", 1e1},
        {"rel_tol_matric", 1e1},
        {"rel_tol_aquifr", 1e1},
        {"abs_tol_temp_cas", 1e1},
        {"abs_tol_temp_veg", 1e1},
        {"abs_tol_wat_veg", 1e1},
        {"abs_tol_temp_soil_snow", 1e1},
        {"abs_tol_wat_snow", 1e1},
        {"abs_tol_matric", 1e1},
        {"abs_tol_aquifr", 1e1},
        {"default_tol", true}
    };

    std::ofstream config_file_stream("config.json");
    config_file_stream << std::setw(4) << config_file.dump(2) << std::endl;
    config_file_stream.close();
}