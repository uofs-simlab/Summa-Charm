cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
project(summa-charm++ VERSION 1.0.0 LANGUAGES Fortran C CXX)

# Add options for build type
set(CMAKE_CONFIGURATION_TYPES Release Debug)
message("\nSelected Bulid Type: ${CMAKE_BUILD_TYPE}\n")

# Optional Dependencies
option(USE_SUNDIALS "Use IDA solver from sundilas suite" OFF)
option(USE_V4 "Use V4 interface" OFF)
set(EXT_TARGETS) # list of external targets to link to

if (USE_SUNDIALS)
    message("ENABLING SUNDIALS")
    set(USE_V4 ON CACHE BOOL "Use V4 interface" FORCE)
    find_package(SUNDIALS REQUIRED)
    list(APPEND EXT_TARGETS SUNDIALS::fida_mod_shared 
                            SUNDIALS::fkinsol_mod_shared)
    add_compile_definitions(SUNDIALS_ACTIVE)
endif()

if (USE_V4)
    add_compile_definitions(CHARES_ACTIVE V4_ACTIVE)
endif()

# Required Dependencies
include(FortranCInterface)
FortranCInterface_VERIFY(CXX)

# Attempt to find TBB
option(USE_TBB "Use TBB" OFF)
if (USE_TBB)
    add_compile_definitions(ACTORS_ACTIVE TBB_ACTIVE)
    message("Attempting to enable TBB")
    find_package(TBB QUIET)
    if(TBB_FOUND)
        list(APPEND EXT_TARGETS TBB::tbb)
        message(STATUS "TBB found and will be used.")
    else()
        message(WARNING "TBB not found. Proceeding without TBB.")
    endif()
endif()

set(F_MASTER "${CMAKE_CURRENT_LIST_DIR}/..")
get_filename_component(F_MASTER "${F_MASTER}" REALPATH)
set(EXEC_DIR "${F_MASTER}/bin")
get_filename_component(EXEC_DIR "${EXEC_DIR}" REALPATH)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${EXEC_DIR})
set(EXEC_NAME summa_chare.exe)

# NetCDF is found with a custom FindNetCDF.cmake file
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")
LIST(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/cmake/")
find_package(NetCDF REQUIRED)
list(APPEND EXT_TARGETS NetCDF::NetCDF)

# Find Charm++
find_package(Charm REQUIRED)
list(APPEND EXT_TARGETS ${CHARM_LIBRARIES})

message(STATUS "Charm++ compiler wrapper: ${CHARM_CHARMC}")


# LAPACK
option(USE_CUSTOM_LAPACK "Use custom LAPACK" OFF)
if (USE_CUSTOM_LAPACK)
    message("Using custom LAPACK installation")
    # Set the LAPACK library and include directories explicitly
    set(LAPACK_LIBRARIES $ENV{LAPACK_PATH})
    message("LAPACK_LIBRARIES: ${LAPACK_LIBRARIES}")
    list(APPEND EXT_TARGETS ${LAPACK_LIBRARIES})
else()
    find_package(LAPACK REQUIRED)
    list(APPEND EXT_TARGETS ${LAPACK_LIBRARIES})
endif()


# Set compiler flags
set(FLAGS_OPT $ENV{FLAGS_OPT}) # get optional user-specified flags from environment variables
if(CMAKE_BUILD_TYPE MATCHES Debug)
    message("\nSetting Debug Options")
    add_compile_definitions(DEBUG) 
    set(FLAGS_NOAH -g -O0 -fbacktrace -fbounds-check -ffree-form -ffree-line-length-none -fmax-errors=0 -fPIC -Wfatal-errors ${FLAGS_OPT})
    set(FLAGS_ALL -g -O0 -fbacktrace -fbounds-check -ffree-line-length-none -fmax-errors=0 -fPIC -Wfatal-errors -cpp ${FLAGS_OPT})
    set(FLAGS_CXX -g -O0 -fbounds-check -Wfatal-errors -std=c++17 ${FLAGS_OPT})
else()
    message("\nSetting Release Options")
    set(FLAGS_NOAH -O3 -ffree-form -ffree-line-length-none -fmax-errors=0 -fPIC -Wfatal-errors ${FLAGS_OPT})
    set(FLAGS_ALL -O3 -ffree-line-length-none -fmax-errors=0 -fPIC -Wfatal-errors -cpp ${FLAGS_OPT})
    set(FLAGS_CXX -O3 -Wfatal-errors -std=c++17 ${FLAGS_OPT})
endif()


#=========================================================================================
# COMPILE PART 1: Define directory paths
#=========================================================================================

# Define directories that contains source code
set(DRIVER_DIR ${F_MASTER}/build/summa/build/source/driver)
set(DSHARE_DIR ${F_MASTER}/build/summa/build/source/dshare)
set(ENGINE_DIR ${F_MASTER}/build/summa/build/source/engine)
set(HOOKUP_DIR ${F_MASTER}/build/summa/build/source/hookup)
set(NETCDF_DIR ${F_MASTER}/build/summa/build/source/netcdf)
set(NOAHMP_DIR ${F_MASTER}/build/summa/build/source/noah-mp)

# Define Chares specific directories
set(CHARES_DIR      ${F_MASTER}/build/source)
set(SYS_INIT_DIR    ${CHARES_DIR}/system_initialization)
set(FILE_ACCESS_DIR ${CHARES_DIR}/file_access_chare)
set(JOB_CHARE_DIR   ${CHARES_DIR}/job_chare)
set(GRU_CHARE_DIR   ${CHARES_DIR}/gru_chare)

#=========================================================================================
# COMPILE PART 2: Assemble all of the SUMMA sub-routines
#=========================================================================================

# SUMMA Chares Source Files are defined in the CMakeLists.txt file in the subdirectory ./source
add_subdirectory(${F_MASTER}/build/source)
# SUMMA Source Files are defined in the CMakeLists.txt file in the subdirectory ./summa/build/source
add_subdirectory(${F_MASTER}/build/summa/build/source)

#=========================================================================================
# COMPILE PART 1.5: Process Charm++ Interface Files
#=========================================================================================

# Process Charm++ interface files to generate .decl.h and .def.h files
foreach(CI_FILE ${CI_FILES})
    get_filename_component(CI_NAME ${CI_FILE} NAME_WE)
    set(DECL_FILE ${CMAKE_CURRENT_BINARY_DIR}/${CI_NAME}.decl.h)
    set(DEF_FILE ${CMAKE_CURRENT_BINARY_DIR}/${CI_NAME}.def.h)
    
    add_custom_command(
        OUTPUT ${DECL_FILE} ${DEF_FILE}
        COMMAND ${CHARM_CHARMC} -c ${CI_FILE}
        DEPENDS ${CI_FILE}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Processing Charm++ interface file ${CI_FILE}"
    )
    
    list(APPEND CHARM_GENERATED_FILES ${DECL_FILE} ${DEF_FILE})
endforeach()

# Create a custom target for the generated files
add_custom_target(charm_interfaces ALL DEPENDS ${CHARM_GENERATED_FILES})

# Adjust Summa Source Files
list(REMOVE_ITEM MODRUN
        ${ENGINE_DIR}/read_force.f90)

list(REMOVE_ITEM SOLVER
        ${ENGINE_DIR}/run_oneGRU.f90
        ${ENGINE_DIR}/run_oneHRU.f90)

list(REMOVE_ITEM DRIVER
        ${DRIVER_DIR}/summa_util.f90
        ${DRIVER_DIR}/summa_defineOutput.f90
        ${DRIVER_DIR}/summa_init.f90
        ${DRIVER_DIR}/summa_forcing.f90
        ${DRIVER_DIR}/summa_modelRun.f90
        ${DRIVER_DIR}/summa_setup.f90
        ${DRIVER_DIR}/summa_writeOutput.f90)

#=========================================================================================
# COMPILE PART 3: Collect the subroutines into build groups depending on build type
#=========================================================================================
set(COMM_ALL ${NRPROC} ${HOOKUP} ${DATAMS} ${UTILMS})
set(SUMMA_ALL ${NETCDF} ${PRELIM} ${MODRUN} ${SOLVER} ${DRIVER})
set(COMM_ALL ${COMM_ALL} ${DATAMS_CHARES} ${INTERFACE})
set(SUMMA_ALL ${SUMMA_ALL} ${SYS_INIT_INTERFACE} ${FILE_ACCESS_INTERFACE}
              ${JOB_INTERFACE} ${GRU_INTERFACE})
set(MAIN_SUMMA ${CHARES_DIR}/main.cpp)

if (USE_SUNDIALS)
    set(COMM_ALL ${COMM_ALL} ${DATAMS_SUNDIALS} ${UTILMS_SUNDIALS})
    set(SUMMA_ALL ${SUMMA_ALL} ${MODRUN_SUNDIALS} ${SOLVER_SUNDIALS})
endif()


#=========================================================================================
# COMPILE PART 4: Do the compilation
#=========================================================================================

# Build SUMMA_NOAHMP Object
add_library(SUMMA_NOAHMP OBJECT ${NOAHMP} ${NRUTIL})
target_compile_options(SUMMA_NOAHMP PRIVATE ${FLAGS_NOAH})

# Build SUMMA_COMM Object
add_library(SUMMA_COMM OBJECT ${COMM_ALL})
target_compile_options(SUMMA_COMM PRIVATE ${FLAGS_ALL})
target_link_libraries(SUMMA_COMM PUBLIC SUMMA_NOAHMP ${EXT_TARGETS}) # added flags to the link step

# Build SUMMA Object
add_library(summa SHARED ${SUMMA_ALL})
target_compile_options(summa PRIVATE ${FLAGS_ALL})
target_link_libraries(summa PUBLIC ${EXT_TARGETS} SUMMA_NOAHMP SUMMA_COMM)

# HAVE CMAKE INVOKE CHARM++ FOR THE LINK STEP
# Add -module TreeLB to enable GreedyLB, RefineLB, and other TreeLB-based load balancers
# 
# TOGGLE TRACING: Uncomment one of the following lines:
# 
# For PROFILING with Projections (tracing enabled - use +traceroot at runtime):
set(CMAKE_CXX_LINK_EXECUTABLE
    "${CHARM_CHARMC} -tracemode projections -module TreeLB <FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>"
)
#
# For PRODUCTION/LOAD BALANCER TESTING (no tracing overhead):
#set(CMAKE_CXX_LINK_EXECUTABLE
#    "${CHARM_CHARMC} -module TreeLB <FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>"
#)

add_executable(${EXEC_NAME} ${MAIN_SUMMA} ${CHARES_GLOBAL} 
               ${FILE_ACCESS_CHARE} ${JOB_CHARE} ${GRU_CHARE} ${SYS_INIT})

# Add dependency on charm interface generation
add_dependencies(${EXEC_NAME} charm_interfaces)

set_property(TARGET ${EXEC_NAME} PROPERTY LINKER_LANGUAGE CXX)
target_compile_options(${EXEC_NAME} PUBLIC ${FLAGS_CXX})
target_include_directories(${EXEC_NAME} PUBLIC 
                           ${F_MASTER}/build/includes/global 
                           ${F_MASTER}/build/includes/summa_chare 
                           ${F_MASTER}/build/includes/job_chare 
                           ${F_MASTER}/build/includes/file_access_chare 
                           ${F_MASTER}/build/includes/gru_chare
                           ${CMAKE_CURRENT_BINARY_DIR}
                           ${CHARM_INCLUDE_DIRS})
target_link_libraries(${EXEC_NAME} ${EXT_TARGETS} summa)

