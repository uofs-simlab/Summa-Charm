#===============================================================================
# List of Source Files for Summa-Chares
# All Variables are reachable from the parent scope
#===============================================================================

# Charm++ interface files
set(CI_FILES
    ${CHARES_DIR}/main.ci
    ${CHARES_DIR}/SummaChare.ci
    ${CHARES_DIR}/JobChare.ci
    ${CHARES_DIR}/FileAccessChare.ci
    ${CHARES_DIR}/GruChare.ci
    CACHE INTERNAL "CI_FILES")

# Chares interface modules
set(INTERFACE
    ${CHARES_DIR}/global/cppwrap_auxiliary.f90
    ${CHARES_DIR}/global/cppwrap_datatypes.f90
    ${CHARES_DIR}/global/cppwrap_metadata.f90
    ${CHARES_DIR}/global/c_interface_module.f90
    ${CHARES_DIR}/global/global_tol.f90
    CACHE INTERNAL "INTERFACE")
set(SYS_INIT_INTERFACE
    ${SYS_INIT_DIR}/batch_distributer_chare.f90
    CACHE INTERNAL "SYS_INIT_INTERFACE")
set(FILE_ACCESS_INTERFACE
    ${FILE_ACCESS_DIR}/summa_init_struc.f90
    ${FILE_ACCESS_DIR}/forcing_file_info.f90
    ${FILE_ACCESS_DIR}/file_access_chare.f90
    # ${FILE_ACCESS_DIR}/output_structure.f90
    ${FILE_ACCESS_DIR}/output_buffer.f90
    ${FILE_ACCESS_DIR}/output_buffer_write.f90
    ${FILE_ACCESS_DIR}/fileAccess_writeOutput.f90
    CACHE INTERNAL "FILE_ACCESS_INTERFACE")
if (USE_V4)
    set(JOB_INTERFACE
        ${JOB_CHARE_DIR}/summa_setup_sundials.f90
        ${JOB_CHARE_DIR}/gru_struc.f90
        CACHE INTERNAL "JOB_INTERFACE")
else()
    set(JOB_INTERFACE
        ${JOB_CHARE_DIR}/summa_setup.f90
        ${JOB_CHARE_DIR}/gru_struc.f90
        CACHE INTERNAL "JOB_INTERFACE")
endif()        
set(GRU_INTERFACE
    ${GRU_CHARE_DIR}/hru_init.f90
    ${GRU_CHARE_DIR}/hru_read.f90
    ${GRU_CHARE_DIR}/hru_modelRun.f90
    ${GRU_CHARE_DIR}/hru_writeOutput.f90
    ${GRU_CHARE_DIR}/gru_chare.f90
    CACHE INTERNAL "GRU_INTERFACE")

# Chares actual chare modules
set(DATAMS_CHARES
    ${CHARES_DIR}/global/chare_data_types.f90
    CACHE INTERNAL "DATAMS_CHARES")

set(CHARES_GLOBAL
    ${CHARES_DIR}/global/auxiliary.cpp
    ${CHARES_DIR}/global/file_manager.cpp
    ${CHARES_DIR}/global/settings_functions.cpp
    ${CHARES_DIR}/global/timing_info.cpp
    ${CHARES_DIR}/global/logger.cpp
    CACHE INTERNAL "CHARES_GLOBAL")
set(SYS_INIT
    ${SYS_INIT_DIR}/batch.cpp
    ${SYS_INIT_DIR}/batch_container.cpp
    # ${SYS_INIT_DIR}/client.cpp
    # ${SYS_INIT_DIR}/client_container.cpp
    ${SYS_INIT_DIR}/summa_global_data.cpp
    ${SYS_INIT_DIR}/summa_chare.cpp
    # ${SYS_INIT_DIR}/summa_backup_server.cpp
    # ${SYS_INIT_DIR}/summa_client.cpp
    # ${SYS_INIT_DIR}/summa_server.cpp
    CACHE INTERNAL "SYS_INIT")
set(FILE_ACCESS_CHARE
    ${FILE_ACCESS_DIR}/summa_init_struc.cpp  # C++ implementation for SummaInitStruc
    ${FILE_ACCESS_DIR}/file_access_chare.cpp  # C++ implementation
    ${FILE_ACCESS_DIR}/forcing_file_info.cpp
    ${FILE_ACCESS_DIR}/output_buffer.cpp
    CACHE INTERNAL "FILE_ACCESS_CHARE")
set(JOB_CHARE
    ${JOB_CHARE_DIR}/job_chare.cpp  # C++ implementation for JobArray
    ${JOB_CHARE_DIR}/gru_struc.cpp  # C++ implementation for GRU structures
    # ${JOB_CHARE_DIR}/da_server_chare.cpp
    # ${JOB_CHARE_DIR}/da_client_chare.cpp
    # ${JOB_CHARE_DIR}/node_chare.cpp
    CACHE INTERNAL "JOB_CHARE")
set(GRU_CHARE
    ${GRU_CHARE_DIR}/gru_chare.cpp
    # ${GRU_CHARE_DIR}/gru_batch_chare.cpp
    CACHE INTERNAL "GRU_CHARE")
set(HRU_CHARE
    # ${HRU_CHARE_DIR}/hru_utils.cpp
    # ${HRU_CHARE_DIR}/hru_chare.cpp
    CACHE INTERNAL "HRU_CHARE")
