# FindCharm.cmake - Module to find Charm++ installation
#
# This module defines:
#  CHARM_FOUND - True if Charm++ is found
#  CHARM_INCLUDE_DIRS - Include directories for Charm++
#  CHARM_LIBRARIES - Link libraries for Charm++
#  CHARM_CHARMC - Path to charmc compiler
#  CHARM_CHARMRUN - Path to charmrun launcher
#  CHARM_VERSION - Version of Charm++ found

find_path(CHARM_INCLUDE_DIR
    NAMES charm++.h charm.h
    HINTS
        ${CHARM_ROOT}/include
        ${CMAKE_SOURCE_DIR}/../utils/dependencies/install/charmppNEW/include
        ${CMAKE_SOURCE_DIR}/../utils/dependencies/install/charmpp/include
        ${CMAKE_SOURCE_DIR}/utils/dependencies/install/charmppNEW/include
        ${CMAKE_SOURCE_DIR}/utils/dependencies/install/charmpp/include
        ENV CHARM_HOME
    PATH_SUFFIXES include
    DOC "Charm++ include directory"
)

find_program(CHARM_CHARMC
    NAMES charmc
    HINTSNEW/bin
        ${CMAKE_SOURCE_DIR}/../utils/dependencies/install/charmpp/bin
        ${CMAKE_SOURCE_DIR}/utils/dependencies/install/charmppNEW
        ${CHARM_ROOT}/bin
        ${CMAKE_SOURCE_DIR}/../utils/dependencies/install/charmpp/bin
        ${CMAKE_SOURCE_DIR}/utils/dependencies/install/charmpp/bin
        ENV CHARM_HOME
    PATH_SUFFIXES bin
    DOC "Charm++ compiler wrapper"
)

find_program(CHARM_CHARMRUN
    NAMES charmrun
    HINTS
        ${CHARM_ROOT}/bin
        ${CMAKE_SOURCE_DIR}/../utils/dependencies/install/charmppNEW/bin
        ${CMAKE_SOURCE_DIR}/../utils/dependencies/install/charmpp/bin
        ${CMAKE_SOURCE_DIR}/utils/dependencies/install/charmppNEW/bin
        ${CMAKE_SOURCE_DIR}/utils/dependencies/install/charmpp/bin
        ENV CHARM_HOME
    PATH_SUFFIXES bin
    DOC "Charm++ runtime launcher"
)

find_library(CHARM_CK_LIBRARY
    NAMES ck libck.a
    HINTS
        ${CHARM_ROOT}/lib
        ${CMAKE_SOURCE_DIR}/../utils/dependencies/install/charmppNEW/lib
        ${CMAKE_SOURCE_DIR}/../utils/dependencies/install/charmpp/lib
        ${CMAKE_SOURCE_DIR}/utils/dependencies/install/charmppNEW/lib
        ${CMAKE_SOURCE_DIR}/utils/dependencies/install/charmpp/lib
        ENV CHARM_HOME
    PATH_SUFFIXES lib
    DOC "Charm++ ck library"
)

find_library(CHARM_CONVERSE_LIBRARY
    NAMES converse libconverse.a
    HINTS
        ${CHARM_ROOT}/lib
        ${CMAKE_SOURCE_DIR}/../utils/dependencies/install/charmppNEW/lib
        ${CMAKE_SOURCE_DIR}/../utils/dependencies/install/charmpp/lib
        ${CMAKE_SOURCE_DIR}/utils/dependencies/install/charmppNEW/lib
        ${CMAKE_SOURCE_DIR}/utils/dependencies/install/charmpp/lib
        ENV CHARM_HOME
    PATH_SUFFIXES lib
    DOC "Charm++ converse library"
)

find_library(CHARM_TRACE_PROJECTIONS_LIBRARY
    NAMES trace-projections libtrace-projections.a
    HINTS
        ${CMAKE_SOURCE_DIR}/utils/dependencies/install/charmppNEW/lib
        ${CMAKE_SOURCE_DIR}/../utils/dependencies/install/charmpp/lib
        ${CMAKE_SOURCE_DIR}/utils/dependencies/install/charmppNEW/lib
        ${CHARM_ROOT}/lib
        ${CMAKE_SOURCE_DIR}/../utils/dependencies/install/charmpp/lib
        ${CMAKE_SOURCE_DIR}/utils/dependencies/install/charmpp/lib
        ENV CHARM_HOME
    PATH_SUFFIXES lib
    DOC "Charm++ trace-projections library for profiling"
)

find_library(CHARM_TREELB_LIBRARY
    NAMES moduleTreeLB libmoduleTreeLB.a
    HINTS
        ${CMAKE_SOURCE_DIR}/utils/dependencies/install/charmppNEW/lib
        ${CMAKE_SOURCE_DIR}/../utils/dependencies/install/charmppNEW/lib
        ${CHARM_ROOT}/lib
        ${CMAKE_SOURCE_DIR}/../utils/dependencies/install/charmpp/lib
        ${CMAKE_SOURCE_DIR}/utils/dependencies/install/charmpp/lib
        ENV CHARM_HOME
    PATH_SUFFIXES lib
    DOC "Charm++ TreeLB module library (required for GreedyLB, RefineLB, etc.)"
)

# Extract Charm++ version if possible
if(CHARM_CHARMC)
    execute_process(
        COMMAND ${CHARM_CHARMC} -V
        OUTPUT_VARIABLE CHARM_VERSION_OUTPUT
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    
    if(CHARM_VERSION_OUTPUT MATCHES "Version ([0-9]+\\.[0-9]+)")
        set(CHARM_VERSION ${CMAKE_MATCH_1})
    endif()
endif()

# Set include directories and libraries
set(CHARM_INCLUDE_DIRS ${CHARM_INCLUDE_DIR})
set(CHARM_LIBRARIES ${CHARM_CK_LIBRARY} ${CHARM_CONVERSE_LIBRARY})

# Add trace-projections library if found (optional for profiling)
if(CHARM_TRACE_PROJECTIONS_LIBRARY)
    list(APPEND CHARM_LIBRARIES ${CHARM_TRACE_PROJECTIONS_LIBRARY})
    message(STATUS "Charm++ trace-projections library found: ${CHARM_TRACE_PROJECTIONS_LIBRARY}")
endif()

# Add TreeLB module library if found (required for GreedyLB, RefineLB, etc.)
if(CHARM_TREELB_LIBRARY)
    list(APPEND CHARM_LIBRARIES ${CHARM_TREELB_LIBRARY})
    message(STATUS "Charm++ TreeLB module library found: ${CHARM_TREELB_LIBRARY}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Charm
    FOUND_VAR CHARM_FOUND
    REQUIRED_VARS
        CHARM_INCLUDE_DIR
        CHARM_CHARMC
        CHARM_CHARMRUN
        CHARM_CK_LIBRARY
        CHARM_CONVERSE_LIBRARY
    VERSION_VAR CHARM_VERSION
)

mark_as_advanced(
    CHARM_INCLUDE_DIR
    CHARM_CK_LIBRARY
    CHARM_CONVERSE_LIBRARY
    CHARM_TRACE_PROJECTIONS_LIBRARY
    CHARM_CHARMC
    CHARM_CHARMRUN
)

# Create imported target for Charm++
if(CHARM_FOUND AND NOT TARGET Charm::Charm)
    add_library(Charm::Charm INTERFACE IMPORTED)
    set_target_properties(Charm::Charm PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${CHARM_INCLUDE_DIRS}"
        INTERFACE_LINK_LIBRARIES "${CHARM_LIBRARIES}"
    )
endif()

# Function to compile Charm++ interface files
function(add_charm_module target)
    cmake_parse_arguments(CHARM "" "CI_FILE;OUTPUT_DIR" "" ${ARGN})
    
    if(NOT CHARM_CI_FILE)
        message(FATAL_ERROR "add_charm_module: CI_FILE is required")
    endif()
    
    get_filename_component(CI_BASE ${CHARM_CI_FILE} NAME_WE)
    
    if(CHARM_OUTPUT_DIR)
        set(OUTPUT_BASE ${CHARM_OUTPUT_DIR}/${CI_BASE})
    else()
        set(OUTPUT_BASE ${CMAKE_CURRENT_BINARY_DIR}/${CI_BASE})
    endif()
    
    set(DECL_FILE ${OUTPUT_BASE}.decl.h)
    set(DEF_FILE ${OUTPUT_BASE}.def.h)
    
    add_custom_command(
        OUTPUT ${DECL_FILE} ${DEF_FILE}
        COMMAND ${CHARM_CHARMC} ${CHARM_CI_FILE}
        DEPENDS ${CHARM_CI_FILE}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating Charm++ interface for ${CHARM_CI_FILE}"
    )
    
    add_custom_target(${target}_ci DEPENDS ${DECL_FILE} ${DEF_FILE})
    add_dependencies(${target} ${target}_ci)
endfunction()
