#pragma once

#include "GruChare.decl.h"
#include "FileAccessChare.decl.h"
#include "JobChare.decl.h"
#include "hru_chare_settings.hpp"
#include "tolarance_settings.hpp"
#include "fortran_data_types.hpp"
// #include "hru_chare.hpp"
#include <vector>
#include <memory>

extern "C"
{
    void f_getNumHruInGru(int &index_gru, int &num_hru);
    void f_initGru(int &index_gru, void *gru_data, int &output_buffer_steps,
                   int &err, void *message);
    void setupGRU_fortran(int &index_gru, void *gru_data, int &err,
                          void *message);
    void readGRURestart_fortran(int &index_gru, void *gru_data, int &err,
                                void *message);
    void setTimeZoneOffsetGRU_fortran(int &iFile, void *gru_data, int &err,
                                      void *message);
    void readGRUForcing_fortran(int &index_gru, int &iStep, int &iRead,
                                int &iFile, void *gru_data, int &err, void *message);
    void runGRU_fortran(int &index_gru, int &timestep, void *gru_data,
                        int &dt_init_factor, int &err, void *message);
    void writeGRUOutput_fortran(int &index_gru, int &timestep, int &output_step,
                                void *gru_data, int &err, void *message, 
                                int& year, int& month, int& day, int& hour);
    void f_setGruTolerances(void* gru_data, int& be_steps,
      // Relative Tolerances 
      double& rel_tol_temp_cas, double& rel_tol_temp_veg, 
      double& rel_tol_wat_veg, double& rel_tol_temp_soil_snow, 
      double& rel_tol_wat_snow, double& rel_tol_matric, double& rel_tol_aquifr, 
      // Absolute Tolerances
    //   double& abs_tol, double& abs_tolWat, double& abs_tolNrg,
      double& abs_tol_temp_cas, double& abs_tol_temp_veg, 
      double& abs_tol_wat_veg, double& abs_tol_temp_soil_snow, 
      double& abs_tol_wat_snow, double& abs_tol_matric, 
      double& abs_tol_aquifr);
}

struct GruDeleter
{
       void operator()(void *ptr) const
    {
        delete_handle_gru_type(ptr);
    }
};

struct Date {
  int y;
  int m;
  int d;
  int h;
};

class GruChare : public CBase_GruChare
{
    int netcdf_index_;
    int job_index_;
    HRUChareSettings hru_chare_settings_;
    ToleranceSettings tolerance_settings_;
    int num_steps_output_buffer_;
    CkChareID file_access_chare_;
    CkChareID parent_;

    int num_hrus_;
    std::unique_ptr<void, GruDeleter> gru_data_;

    double dt_init_ = 0.0;
    int dt_init_factor_ = 1;
    int num_steps_until_write_ = 0;
    int num_steps_ = 0; // number of time steps
    int timestep_ = 1;  // Current Timestep of HRU simulation
    int iFile_ = 1;
    int stepsInCurrentFFile_ = 0; // number of time steps in current forcing file
    int forcingStep_ = 1;     // index of current time step in current forcing file
    int output_step_ = 1;     // index of current time step in output structure

    bool data_assimilation_mode_ = false;
    bool logged_tols_ = false;
    Date current_time = {0,0,0,0};
    Date start_time = {-1,-1,-1,-1};

public:
    GruChare(int netcdf_index, int job_index,
             int num_steps, HRUChareSettings hru_chare_settings,
             int num_output_steps, CkChareID file_access_chare, CkChareID parent,
             ToleranceSettings tolerance_settings);
    GruChare(CkMigrateMessage *msg);
    ~GruChare() {}

    void newForcingFile(int num_forc_steps, int iFile);
    void setNumStepsBeforeWrite(int num_steps);
    void runHRU();
    void handleErr(int err, std::unique_ptr<char[]> &message);
    void doneHRU();
    void updateHRU();
};
