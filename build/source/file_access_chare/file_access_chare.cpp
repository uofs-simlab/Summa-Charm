#include "file_access_chare.hpp"
#include "FileAccessChare.decl.h"
#include "JobChare.decl.h"        // For CProxy_JobChare
#include "pup_stl.h"              // For STL serialization
#include "settings_functions.hpp" // For FileAccessActorSettings

FileAccessChare::FileAccessChare(NumGRUInfo num_gru_info,
                                 FileAccessActorSettings fa_settings,
                                 CkChareID job_chare_proxy)
    : CBase_FileAccessChare(), num_gru_info_(num_gru_info),
      fa_settings_(fa_settings), job_chare_proxy_(job_chare_proxy)
{

  CkPrintf("\n----------File_Access_Chare Started----------\n");

  // Timing Info
  timing_info_ = TimingInfo();
  timing_info_.addTimePoint("write_duration");

  // Set GRU info based on settings
  if (num_gru_info_.use_global_for_data_structures)
  {
    start_gru_ = num_gru_info_.start_gru_global;
    num_gru_ = num_gru_info_.num_gru_global;
  }
  else
  {
    start_gru_ = num_gru_info_.start_gru_local;
    num_gru_ = num_gru_info_.num_gru_local;
  }
}

int FileAccessChare::initFileAccessChare(int file_gru, int num_hru)
{
  int err = 0;
  CkPrintf("File Access Actor: Initializing\n");
  num_hru_ = num_hru;

  f_getNumTimeSteps(num_steps_);

  forcing_files_ = std::make_unique<forcingFileContainer>();
  if (forcing_files_->initForcingFiles() != 0)
    return -1;

  // Initialize output buffer
  output_buffer_ = std::make_unique<OutputBuffer>(
      fa_settings_, num_gru_info_, num_hru_, num_steps_);
  int chunk_return = output_buffer_->setChunkSize();
  CkPrintf("Chunk Size = %d\n", chunk_return);
  err = output_buffer_->defOutput(std::to_string(thishandle.onPE));

  // err = output_buffer_->defOutput("FileAccessChare");
  if (err != 0)
  {
    CkPrintf("File Access Actor: Error defOutput\n"
             "\tMessage = Can't define output file\n");
    CkExit();
    return err;
  }
  err = output_buffer_->allocateOutputBuffer(num_steps_);

  timing_info_.updateEndPoint("init_duration");
  return num_steps_;
}

int FileAccessChare::getNumOutputSteps(int job_index)
{
  return output_buffer_->getNumStepsBuffer(job_index);
}

void FileAccessChare::accessForcing(int i_file, CkChareID gru_chare)
{
  if (forcing_files_->allFilesLoaded())
  {
    CProxy_GruChare(gru_chare).newForcingFile(forcing_files_->getNumSteps(i_file), i_file);
    return;
  }
  auto err = forcing_files_->loadForcingFile(i_file, start_gru_, num_gru_);
  if (err != 0)
  {
    CkPrintf("File Access Actor: Error loadForcingFile\n"
             "\tMessage = Can't load forcing file\n");
    CProxy_JobChare(job_chare_proxy_).handleGruChareError(0, 0, err, "Can't load forcing file\n");
    return;
  }

  // Load files behind the scenes
  accessForcingInternal(i_file + 1);
  CProxy_GruChare(gru_chare).newForcingFile(forcing_files_->getNumSteps(i_file), i_file);
}

void FileAccessChare::runFailure(int index_gru_job)
{
  timing_info_.updateStartPoint("write_duration");
  auto update_status = output_buffer_->addFailedGRU(index_gru_job);

  if (!update_status.has_value())
  {
    timing_info_.updateEndPoint("write_duration");
    return;
  }

  if (update_status.value()->err != 0)
  {
    CkPrintf("File Access Actor: Error writeOutput\n"
             "\tMessage = %s\n",
             update_status.value()->message.c_str());
    CProxy_JobChare(job_chare_proxy_).handleGruChareError(0, 0, update_status.value()->err, update_status.value()->message);
    return;
  }

  for (auto gru : update_status.value()->actor_to_update)
  {
    CProxy_GruChare(gru).setNumStepsBeforeWrite(update_status.value()->num_steps_update);
    CProxy_GruChare(gru).runHRU();
  }

  timing_info_.updateEndPoint("write_duration");
}

int FileAccessChare::restartFailures()
{
  CkPrintf("File Access Actor: Restarting Failed GRUs\n");
  int sleep = output_buffer_->reconstruct();
  return sleep;
}

void FileAccessChare::accessForcingInternal(int i_file)
{
  if (forcing_files_->allFilesLoaded())
    return;
  auto err = forcing_files_->loadForcingFile(i_file, start_gru_, num_gru_);
  if (err != 0)
  {
    CkPrintf("File Access Actor: Error loadForcingFile\n"
             "\tMessage = Can't load forcing file\n");
    CProxy_JobChare(job_chare_proxy_).handleGruChareError(0, 0, err, "Can't load forcing file\n");
    return;
  }
  accessForcingInternal(i_file + 1);
}

void FileAccessChare::writeOutput(int index_gru, CkChareID gru_chare)
{
  timing_info_.updateStartPoint("write_duration");

  auto update_status = output_buffer_->writeOutput(index_gru, gru_chare);

  // Do nothing if optional is emtpy
  if (!update_status.has_value())
  {
    timing_info_.updateEndPoint("write_duration");
    return;
  }

  // If error, send error message to parent
  if (update_status.value()->err != 0)
  {
    CkPrintf("File Access Actor: Error writeOutput\n"
             "\tMessage = %s\n",
             update_status.value()->message.c_str());
    CProxy_JobChare(job_chare_proxy_).handleGruChareError(0, 0, update_status.value()->err, update_status.value()->message);
    return;
  } 

  for (auto gru : update_status.value()->actor_to_update)
  {
    CProxy_GruChare(gru).setNumStepsBeforeWrite(update_status.value()->num_steps_update);
    CProxy_GruChare(gru).runHRU();
  }

  timing_info_.updateEndPoint("write_duration");
}

void FileAccessChare::finalize()
{
  CkPrintf("FileAccessChare: Finalizing\n");
  // TODO: Implement finalization
}

void FileAccessChare::error(int err_code, std::string err_msg)
{
  CkPrintf("FileAccessChare: Error %d: %s\n", err_code, err_msg.c_str());
  // TODO: Implement error handling
}

#include "FileAccessChare.def.h"