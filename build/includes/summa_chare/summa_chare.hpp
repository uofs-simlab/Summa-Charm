#pragma once

#include <chrono>
#include <string>
#include <vector>
#include <memory>
#include "timing_info.hpp"
#include "distributed_settings.hpp"
#include "summa_actor_settings.hpp"
#include "file_access_actor_settings.hpp"
#include "job_actor_settings.hpp"
#include "hru_actor_settings.hpp"
#include "file_manager.hpp"
#include "batch_container.hpp"
#include "summa_global_data.hpp"
#include "SummaChare.decl.h"

class SummaChare : public CBase_SummaChare
{
public:
  SummaChare(int start_gru, int num_gru, std::string config_file,
             std::string master_file, std::string output_file_suffix);

  // Entry methods
  void doneJob(int num_gru_failed, double job_duration, double read_duration, double write_duration);
  void reportError(int err_code, std::string err_msg);
  int spawnJob();
  int createLogDirectory();
  void finalize();

  template <typename T>
  std::optional<T> getSettings(json settings, std::string key_1,
                               std::string key_2)
  {
    try
    {
      if (settings.find(key_1) != settings.end())
      {
        json key_1_settings = settings[key_1];

        // find value behind second key
        if (key_1_settings.find(key_2) != key_1_settings.end())
        {
          return key_1_settings[key_2];
        }
        else
          return {};
      }
      else
      {
        return {}; // return none in the optional (error value)
      }
    }
    catch (json::exception &e)
    {
      std::cout << e.what() << "\n"
                << key_1 << "\n"
                << key_2 << "\n";
      return {};
    }
  }

  std::optional<std::vector<std::string>> getSettingsArray(
      json settings, std::string key_1, std::string key_2);

private:
  DistributedSettings distributed_settings_;
  SummaActorSettings summa_actor_settings_;
  FileAccessActorSettings fa_actor_settings_;
  JobActorSettings job_actor_settings_;
  HRUActorSettings hru_actor_settings_;
  TimingInfo timing_info_;
  int start_gru_;
  int num_gru_;
  int file_gru_;
  int num_gru_failed_;
  std::string master_file_;
  std::string config_file_;
  std::string output_file_suffix_;
  std::string log_folder_;
  CkChareID current_job_;

  std::unique_ptr<FileManager> file_manager_;
  std::unique_ptr<BatchContainer> batch_container_;
  std::shared_ptr<Batch> current_batch_;
  std::unique_ptr<SummaGlobalData> global_fortran_state_;

  int readSettings(std::string config_file);
  void printSettings();
};