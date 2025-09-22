#include "gru_chare.hpp"
#include "JobChare.decl.h"
#include "FileAccessChare.decl.h"

GruChare::GruChare(int netcdf_index, int job_index,
                   int num_steps, HRUChareSettings hru_chare_settings, int num_output_steps,
                   CkChareID file_access_chare, CkChareID parent)
    : netcdf_index_(netcdf_index), job_index_(job_index),
      num_steps_(num_steps), hru_chare_settings_(hru_chare_settings),
      num_steps_output_buffer_(num_output_steps), file_access_chare_(file_access_chare),
      parent_(parent)
{
    int err = 0;
    f_getNumHruInGru(job_index_, num_hrus_);
    gru_data_ = std::unique_ptr<void, GruDeleter>(new_handle_gru_type(num_hrus_));

    std::unique_ptr<char[]> message(new char[256]);
    f_initGru(job_index_, gru_data_.get(), num_steps_output_buffer_, err, &message);
    if (err != 0)
    {
        CkPrintf("GRU Chare: Error initializing GRU -- %s\n", message.get());
        return;
    }
    std::fill(message.get(), message.get() + 256, '\0');

    setupGRU_fortran(job_index_, gru_data_.get(), err, &message);
    if (err != 0)
    {
        CkPrintf("GRU Chare: Error setting up GRU -- %s\n", message.get());
        return;
    }
    std::fill(message.get(), message.get() + 256, '\0');

    readGRURestart_fortran(job_index_, gru_data_.get(), err, &message);
    if (err != 0)
    {
        CkPrintf("GRU Chare: Error reading GRU restart -- %s\n", message.get());
        return;
    }

    f_setGruTolerances(gru_data_.get(), hru_chare_settings_.be_steps_,
                       // Relative Tolerances
                       hru_chare_settings_.rel_tol_, hru_chare_settings_.rel_tol_temp_cas_,
                       hru_chare_settings_.rel_tol_temp_veg_,
                       hru_chare_settings_.rel_tol_wat_veg_,
                       hru_chare_settings_.rel_tol_temp_soil_snow_,
                       hru_chare_settings_.rel_tol_wat_snow_,
                       hru_chare_settings_.rel_tol_matric_, hru_chare_settings_.rel_tol_aquifr_,
                       // Absolute Tolerances
                       hru_chare_settings_.abs_tol_,
                       hru_chare_settings_.abs_tolWat_, hru_chare_settings_.abs_tolNrg_,
                       hru_chare_settings_.abs_tol_temp_cas_,
                       hru_chare_settings_.abs_tol_temp_veg_,
                       hru_chare_settings_.abs_tol_wat_veg_,
                       hru_chare_settings_.abs_tol_temp_soil_snow_,
                       hru_chare_settings_.abs_tol_wat_snow_,
                       hru_chare_settings_.abs_tol_matric_,
                       hru_chare_settings_.abs_tol_aquifr_);

    // // TODO: Implement data assimilation mode if needed
}

void GruChare::newForcingFile(int num_forc_steps, int iFile)
{
    int err;
    std::unique_ptr<char[]> message(new char[256]);
    iFile_ = iFile;
    stepsInCurrentFFile_ = num_forc_steps;
    setTimeZoneOffsetGRU_fortran(iFile_, gru_data_.get(), err, &message);
    if (err != 0)
    {
        CkPrintf("GRU Chare: Error setting time zone offset");
        return;
    }
    forcingStep_ = 1;
    thisProxy.runHRU();
}

void GruChare::setNumStepsBeforeWrite(int num_steps)
{
    num_steps_until_write_ = num_steps;
    output_step_ = 1;
}

void GruChare::runHRU()
{
    int err = 0;
    std::unique_ptr<char[]> message(new char[256]);
    if (timestep_ > num_steps_)
    {
        doneHRU();
        return;
    }
    while (num_steps_until_write_ > 0)
    {
        if (forcingStep_ > stepsInCurrentFFile_)
        {
            CProxy_FileAccessChare(file_access_chare_).accessForcing(iFile_ + 1, thisProxy.ckGetChareID());
            break;
        }
        num_steps_until_write_--;
        if (hru_chare_settings_.print_output_ &&
            timestep_ % hru_chare_settings_.output_frequency_ == 0)
        {
            CkPrintf("GRU Chare %d: timestep=%d, forcingStep=%d, iFile=%d\n",
                     job_index_, timestep_, forcingStep_, iFile_);
        }
        readGRUForcing_fortran(job_index_, timestep_, forcingStep_, iFile_,
                               gru_data_.get(), err, &message);
        if (err != 0)
        {
            handleErr(err, message);
            return;
        }
        std::fill(message.get(), message.get() + 256, '\0'); // Clear message
        runGRU_fortran(job_index_, timestep_, gru_data_.get(), dt_init_factor_,
                       err, &message);
        if (err != 0)
        {
            handleErr(err, message);
            return;
        }
        std::fill(message.get(), message.get() + 256, '\0'); // Clear message
        int year, month, day, hour;
        writeGRUOutput_fortran(job_index_, timestep_, output_step_,
                               gru_data_.get(), err, &message, year, month, day, hour);
        if (err != 0)
        {
            handleErr(err, message);
            return;
        }

        timestep_++;
        forcingStep_++;
        output_step_++;

        if (timestep_ > num_steps_)
            break;
    }

    // Our output structure is full
    if (num_steps_until_write_ <= 0)
    {
        CProxy_FileAccessChare(file_access_chare_).writeOutput(job_index_, thisProxy.ckGetChareID());
    }
}

void GruChare::doneHRU()
{
    CkPrintf("GRU Chare caling doneHRU\n");
    CProxy_JobChare(parent_).doneHRUJob(job_index_);
}

void GruChare::handleErr(int err, std::unique_ptr<char[]> &message)
{
    CkPrintf("GRU Chare %d-%d: Error running GRU at timestep %d",
             job_index_, netcdf_index_, timestep_);
    // int local_err = 0;
    // std::unique_ptr<char[]> local_message(new char[256]);
    // f_fillOutputWithErrs(job_index_, timestep_, output_step_, gru_data_.get(),
    //                      local_err, &local_message);
    CProxy_JobChare(parent_).handleGruChareError(job_index_, timestep_, err, message.get());
}

void GruChare::updateHRU()
{
    int output_steps = CProxy_FileAccessChare(file_access_chare_).getNumOutputSteps(job_index_);
    num_steps_until_write_ = output_steps;
    CProxy_FileAccessChare(file_access_chare_).accessForcing(iFile_, thisProxy.ckGetChareID());
}

#include "GruChare.def.h"