#pragma once
#include <string>
#include <vector>
#include <optional>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <thread>
#include "json.hpp"
#include "pup.h"
#include "pup_stl.h"

#define SUCCESS 0
#define FAILURE -1
#define MISSING_INT -9999
#define MISSING_DOUBLE -9999.0
#define OUTPUT_TIMESTEPS 500
#define NUM_PARTITIONS 8
#define OUTPUT_FREQUENCY 1000
#define GRU_PER_JOB 1000

using json = nlohmann::json;

class DistributedSettings
{
public:
  bool distributed_mode_;
  std::vector<std::string> servers_list_;
  int port_;
  int total_hru_count_;
  int num_hru_per_batch_;
  int num_nodes_;
  bool load_balancing_;

  DistributedSettings(bool distributed_mode = false,
                      std::vector<std::string> servers_list = {},
                      int port = 0,
                      int total_hru_count = 0,
                      int num_hru_per_batch = 0,
                      int num_nodes = 0,
                      bool load_balancing = false)
      : distributed_mode_(distributed_mode),
        servers_list_(std::move(servers_list)),
        port_(port),
        total_hru_count_(total_hru_count),
        num_hru_per_batch_(num_hru_per_batch),
        num_nodes_(num_nodes),
        load_balancing_(load_balancing) {};
  ~DistributedSettings() {};

  std::string toString()
  {
    std::string str = "Distributed Settings:\n";
    str += "Distributed Mode: " + std::to_string(distributed_mode_) + "\n";
    str += "Servers List: ";
    for (auto &server : servers_list_)
    {
      str += server + " ";
    }
    str += "\n";
    str += "Port: " + std::to_string(port_) + "\n";
    str += "Total HRU Count: " + std::to_string(total_hru_count_) + "\n";
    str += "Num HRU Per Batch: " + std::to_string(num_hru_per_batch_) + "\n";
    str += "Num Nodes: " + std::to_string(num_nodes_) + "\n";
    str += "Load Balancing: " + std::to_string(load_balancing_) + "\n";
    return str;
  }
};