#pragma once

#include "pup_stl.h"
#include "GruWorker.decl.h"
#include "FileAccessChare.decl.h"
#include "JobChare.decl.h"
#include "fortran_data_types.hpp"
#include "hru_chare_settings.hpp"
#include "tolarance_settings.hpp"

#include <deque>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

extern "C" {
void f_getNumHruInGru(int &index_gru, int &num_hru);
void f_initGru(int &index_gru, void *gru_data, int &output_buffer_steps,
               int &err, void *message);
void setupGRU_fortran(int &index_gru, void *gru_data, int &err, void *message);
void readGRURestart_fortran(int &index_gru, void *gru_data, int &err,
                            void *message);
void setTimeZoneOffsetGRU_fortran(int &iFile, void *gru_data, int &err,
                                  void *message);
void readGRUForcing_fortran(int &index_gru, int &iStep, int &iRead, int &iFile,
                            void *gru_data, int &err, void *message);
void runGRU_fortran(int &index_gru, int &timestep, void *gru_data,
                    int &dt_init_factor, int &err, void *message);
void writeGRUOutput_fortran(int &index_gru, int &timestep, int &output_step,
                            void *gru_data, int &err, void *message,
                            int &year, int &month, int &day, int &hour);
void f_setGruTolerances(
    void *gru_data, int &be_steps, double &rel_tol_temp_cas,
    double &rel_tol_temp_veg, double &rel_tol_wat_veg,
    double &rel_tol_temp_soil_snow, double &rel_tol_wat_snow,
    double &rel_tol_matric, double &rel_tol_aquifr, double &abs_tol_temp_cas,
    double &abs_tol_temp_veg, double &abs_tol_wat_veg,
    double &abs_tol_temp_soil_snow, double &abs_tol_wat_snow,
    double &abs_tol_matric, double &abs_tol_aquifr);
}

struct GruWorkerDeleter {
  void operator()(void *ptr) const { delete_handle_gru_type(ptr); }
};

struct GruTaskState {
  int netcdf_index = -1;
  int job_index = -1;
  int num_hrus = 0;
  int dt_init_factor = 1;
  bool default_tol = true;
  ToleranceSettings tolerance_settings;
  std::unique_ptr<void, GruWorkerDeleter> gru_data{nullptr};
  int num_steps_until_write = 0;
  int timestep = 1;
  int iFile = 1;
  int stepsInCurrentFFile = 0;
  int forcingStep = 1;
  int output_step = 1;
  bool waiting_for_forcing = true;
  bool waiting_for_output = false;
};

class GruWorker : public CBase_GruWorker {
private:
  int worker_id_ = -1;
  int num_steps_ = 0;
  int num_steps_output_buffer_ = 0;

  HRUChareSettings hru_chare_settings_;
  CkChareID file_access_chare_;
  CkChareID parent_;

  std::unordered_map<int, GruTaskState> tasks_;
  std::deque<int> runnable_jobs_;
  std::unordered_set<int> queued_jobs_;
  bool scheduler_active_ = false;
  bool work_request_pending_ = false;

  GruTaskState *getTask(int job_index);
  void cleanupTask(int job_index);
  void enqueueRunnable(int job_index);
  void maybeRequestMoreWork();
  void runTaskSlice(GruTaskState &task);
  void handleErr(int job_index, int timestep, int err, const std::string &message);

public:
  GruWorker(int worker_id, int num_steps, HRUChareSettings hru_chare_settings,
            int num_output_steps, CkChareID file_access_chare, CkChareID parent);
  GruWorker(CkMigrateMessage *msg);
  ~GruWorker() {}

  void assignTask(int netcdf_index, int job_index,
                  ToleranceSettings tolerance_settings, int dt_init_factor,
                  bool default_tol);
  void newForcingFile(int job_index, int num_forc_steps, int iFile);
  void setNumStepsBeforeWrite(int job_index, int num_steps);
  void setNumStepsBeforeWriteBatch(std::vector<int> job_indices, int num_steps);
  void runHRU();
};
