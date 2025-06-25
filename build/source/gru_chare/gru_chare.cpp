#include "gru_chare.hpp"
#include "JobChare.decl.h"
#include "FileAccessChare.decl.h"

GruChare::GruChare(int netcdf_index, int job_index,
                   int num_steps, HRUActorSettings hru_actor_settings, int num_output_steps,
                   CkChareID file_access_actor, CkChareID parent)
    : netcdf_index_(netcdf_index), job_index_(job_index),
      num_steps_(num_steps), hru_actor_settings_(hru_actor_settings),
      num_steps_output_buffer_(num_output_steps), file_access_actor_(file_access_actor),
      parent_(parent)
{
    int err = 0;
    f_getNumHruInGru(job_index_, num_hrus_);
    gru_data_ = std::unique_ptr<void, GruDeleter>(new_handle_gru_type(num_hrus_));

    std::unique_ptr<char[]> message(new char[256]);
    f_initGru(job_index_, gru_data_.get(), num_steps_output_buffer_, err,
              &message);
    if (err != 0)
    {
        CkPrintf("GRU Actor: Error initializing GRU -- %s\n", message.get());
        CkExit();
        return;
    }
    std::fill(message.get(), message.get() + 256, '\0');

    setupGRU_fortran(job_index_, gru_data_.get(), err, &message);
    if (err != 0)
    {
        CkPrintf("GRU Actor: Error setting up GRU -- %s\n", message.get());
        CkExit();
        return;
    }
    std::fill(message.get(), message.get() + 256, '\0');

    readGRURestart_fortran(job_index_, gru_data_.get(), err, &message);
    if (err != 0)
    {
        CkPrintf("GRU Actor: Error reading GRU restart -- %s\n", message.get());
        CkExit();
        return;
    }

    
    f_setGruTolerances(gru_data_.get(), hru_actor_settings_.rel_tol_,
                       hru_actor_settings_.abs_tol_);

    // TODO: Implement data assimilation mode if needed
    CProxy_FileAccessChare file_access_actor_proxy(file_access_actor_);
    int output_steps = file_access_actor_proxy.getNumOutputSteps(job_index_);
    num_steps_until_write_ = output_steps;


    // call the job chare
    CProxy_JobChare job_chare(parent_);
    job_chare.processGRU(1);


    // accessForcing(iFile_, thishandle);

    //         self_->mail(access_forcing_v, iFile_, self_).
    //             send(file_access_actor_);
}

// void GruChare::run()
// {
//     is_running_ = true;
//     CkPrintf("GruChare[%d]: Starting simulation run\n", gru_index_);
//     processStep();
// }

// void GruChare::updateTimeZoneOffset(int iFile)
// {
//     int err = 0;
//     char message[256] = "";
//     char *msg_ptr = message;

//     setTimeZoneOffsetGRU_fortran(iFile, gru_data_, err, &msg_ptr);
//     if (err != 0)
//     {
//         CkPrintf("GruChare[%d]: Error in updateTimeZoneOffset: %s\n", gru_index_, message);
//         // Report error to parent
//         ErrorInfo error_info;
//         error_info.gru_index = gru_index_;
//         error_info.timestep = timestep_;
//         error_info.error_code = err;
//         error_info.error_message = std::string(message);
//         parent_.reportError(error_info);
//         return;
//     }
// }

// void GruChare::updateHRU(int timestep, int forcing_step, int output_step)
// {
//     timestep_ = timestep;
//     forcing_step_ = forcing_step;
//     output_step_ = output_step;

//     if (is_running_)
//     {
//         processStep();
//     }
// }

// void GruChare::setNumStepsBeforeWrite(int num_steps)
// {
//     num_steps_before_write_ = num_steps;
// }

// void GruChare::processStep()
// {
//     int err = 0;
//     char message[256] = "";
//     char *msg_ptr = message;

//     // Read forcing data for this timestep
//     int iRead = forcing_step_;
//     int iFile = 1; // Assuming single forcing file for now
//     readGRUForcing_fortran(gru_index_, timestep_, iRead, iFile, gru_data_, err, &msg_ptr);
//     if (err != 0)
//     {
//         CkPrintf("GruChare[%d]: Error reading forcing at timestep %d: %s\n",
//                  gru_index_, timestep_, message);

//         ErrorInfo error_info;
//         error_info.gru_index = gru_index_;
//         error_info.timestep = timestep_;
//         error_info.error_code = err;
//         error_info.error_message = std::string(message);
//         parent_.reportError(error_info);
//         return;
//     }

//     // Run GRU physics
//     int dt_init_factor = 1; // Default timestep factor
//     runGRU_fortran(gru_index_, timestep_, gru_data_, dt_init_factor, err, &msg_ptr);
//     if (err != 0)
//     {
//         CkPrintf("GruChare[%d]: Error in physics simulation at timestep %d: %s\n",
//                  gru_index_, timestep_, message);

//         ErrorInfo error_info;
//         error_info.gru_index = gru_index_;
//         error_info.timestep = timestep_;
//         error_info.error_code = err;
//         error_info.error_message = std::string(message);
//         parent_.reportError(error_info);
//         return;
//     }

//     // Write output if needed
//     if (num_steps_before_write_ > 0)
//     {
//         writeGRUOutput_fortran(gru_index_, timestep_, output_step_, gru_data_, err, &msg_ptr);
//         if (err != 0)
//         {
//             CkPrintf("GruChare[%d]: Error writing output at timestep %d: %s\n",
//                      gru_index_, timestep_, message);

//             ErrorInfo error_info;
//             error_info.gru_index = gru_index_;
//             error_info.timestep = timestep_;
//             error_info.error_code = err;
//             error_info.error_message = std::string(message);
//             parent_.reportError(error_info);
//             return;
//         }
//         num_steps_before_write_--;
//     }

//     // Notify completion
//     handleCompletion();
// }

// void GruChare::handleCompletion()
// {
//     GruCompletionInfo completion_info;
//     completion_info.gru_index = gru_index_;
//     completion_info.timestep = timestep_;
//     completion_info.success = true;

//     parent_.reportGruCompletion(completion_info);
// }

// void GruChare::exit()
// {
//     is_running_ = false;

//     // Clean up Fortran data
//     if (gru_data_)
//     {
//         delete_handle_gru_type(gru_data_);
//         gru_data_ = nullptr;
//     }

//     CkPrintf("GruChare[%d]: Exiting\n", gru_index_);
// }

#include "GruChare.def.h"