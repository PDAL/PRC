#
# top-level CMake configuration file for PRC
#

cmake_minimum_required(VERSION 2.6.0)
project(PRC)
string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)

#------------------------------------------------------------------------------
# internal cmake settings
#------------------------------------------------------------------------------

set(CMAKE_COLOR_MAKEFILE ON)

# Allow advanced users to generate Makefiles printing detailed commands
mark_as_advanced(CMAKE_VERBOSE_MAKEFILE)

# Path to additional CMake modules
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/prc_utils.cmake)

set(CMAKE_INCLUDE_DIRECTORIES_PROJECT_BEFORE ON)

#------------------------------------------------------------------------------
# General build settings
#------------------------------------------------------------------------------

set(PRC_VERSION_STRING 0.1.0 CACHE STRING "PRC version")

DISSECT_VERSION()
GET_OS_INFO()
SET_INSTALL_DIRS()

set(PRC_LIB_SOVERSION ${PRC_VERSION_MAJOR})

set(PRC_LIB_NAME prc)

# note we default to debug mode
if(NOT MSVC_IDE)
  if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING 
    "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel" FORCE)
  endif()
  message(STATUS "Setting PRC build type - ${CMAKE_BUILD_TYPE}")
endif()

set(PRC_BUILD_TYPE ${CMAKE_BUILD_TYPE})

set(PRC_OUTPUT_LIB_DIR ${PRC_BINARY_DIR}/${LIB_INSTALL_DIR})
set(PRC_OUTPUT_BIN_DIR ${PRC_BINARY_DIR}/${BIN_INSTALL_DIR})
make_directory(${PRC_OUTPUT_LIB_DIR})
make_directory(${PRC_OUTPUT_BIN_DIR})
if(WIN32)
  foreach(config ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${config} CONFIG)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG} "${PRC_OUTPUT_LIB_DIR}")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG} "${PRC_OUTPUT_BIN_DIR}")
    # ---[ Windows requires DLLs (shared libraries) to be installed in the same directory as executables
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG} "${PRC_OUTPUT_BIN_DIR}")
  endforeach(config)
else(WIN32)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PRC_OUTPUT_LIB_DIR}")
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PRC_OUTPUT_BIN_DIR}")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PRC_OUTPUT_LIB_DIR}")
endif(WIN32)



#------------------------------------------------------------------------------
# Platform and compiler specific settings
#------------------------------------------------------------------------------

if(WIN32)
    add_definitions("-DPDAL_DLL_EXPORT=1")
endif()

if(WIN32)
  if (MSVC)
    if (MSVC80 OR MSVC90 OR MSVC10 OR MSVC11)
      add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
      add_definitions(-D_CRT_SECURE_NO_WARNINGS)
      add_definitions(-D_CRT_NONSTDC_NO_WARNING)
      add_definitions(-D_SCL_SECURE_NO_WARNINGS)

      set(PRC_COMPILER_MSVC 1)
      if (MSVC11)
        set(PRC_COMPILER_VC11 1)
      elseif (MSVC10)
        set(PRC_COMPILER_VC10 1)
      elseif (MSVC9)
        set(PRC_COMPILER_VC9 1)
      elseif (MSVC8)
        set(PRC_COMPILER_VC8 1)
      endif()
    endif()
    
    if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
      string(REGEX REPLACE "/W[0-4]" "/W4"
        CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else()
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    endif()
  endif()
  
  #  set(CMAKE_INCLUDE_PATH c:/osgeo4w/include;$ENV{CMAKE_INCLUDE_PATH})
  #  set(CMAKE_LIBRARY_PATH c:/osgeo4w/lib;$ENV{CMAKE_LIBRARY_PATH})

  set(PRC_PLATFORM_WIN32 1)

else()
  # not WIN32

  # Recommended C++ compilation flags
  # -Weffc++
  set(PRC_COMMON_CXX_FLAGS
	  #    "-pedantic -Wall -Wpointer-arith -Wcast-align -Wcast-qual -Wfloat-equal -Wredundant-decls -Wno-long-long -fPIC")
    "-Wextra -Wall -Wno-unused-parameter -Wno-unused-variable -Wpointer-arith -Wcast-align -Wcast-qual -Wfloat-equal -Wredundant-decls -Wno-long-long -fPIC")

  if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PRC_COMMON_CXX_FLAGS}")
    if (CMAKE_COMPILER_IS_GNUCXX)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
    endif()
    
    set(PRC_COMPILER_GCC 1)
    
  elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" OR "${CMAKE_CXX_COMPILER}" MATCHES "clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PRC_COMMON_CXX_FLAGS} -std=c++11")
    set(PRC_COMPILER_CLANG 1)
  endif()
endif(WIN32)

if (APPLE)
  set(SO_EXT dylib)
  set(CMAKE_FIND_FRAMEWORK "LAST")
elseif(WIN32)
  set(SO_EXT dll)
else()
  set(SO_EXT so)
endif(APPLE)

#------------------------------------------------------------------------------
# settings for 3rd party dependencies
#------------------------------------------------------------------------------

if(WIN32 AND MSVC)
  set(Boost_USE_STATIC_LIBS ON)
endif()
set(Boost_USE_MULTITHREADED ON)
set(Bosot_USE_STATIC_RUNTIME OFF)
add_definitions(-DBOOST_ALL_NO_LIB)

find_package(Boost REQUIRED)
mark_as_advanced(CLEAR Boost_INCLUDE_DIR)
mark_as_advanced(CLEAR Boost_LIBRARIES)
include_directories(${Boost_INCLUDE_DIR})

find_package(ZLIB REQUIRED)
mark_as_advanced(CLEAR ZLIB_INCLUDE_DIR)
mark_as_advanced(CLEAR ZLIP_LIBRARY)
include_directories(${ZLIB_INCLUDE_DIR})

find_package(PDAL REQUIRED)
mark_as_advanced(CLEAR PDAL_INCLUDE_DIR)
mark_as_advanced(CLEAR PDAL_LIBRARIES)
include_directories(${PDAL_INCLUDE_DIR})

find_library(HPDF_LIBRARY hpdf)
find_path(HPDF_INCLUDE_DIR hpdf_u3d.h)
include_directories(${HPDF_INCLUDE_DIR})

#------------------------------------------------------------------------------
# subdirectory controls
#------------------------------------------------------------------------------

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

if(WIN32)
  set(PRC_WRITER_NAME libpdal_plugin_writer_prc)
else(WIN32)
  set(PRC_WRITER_NAME pdal_plugin_writer_prc)
endif(WIN32)

set(PRC_HPP
  include/prc/PRC.hpp
  include/prc/PRCbitStream.hpp
  include/prc/PRCdouble.hpp
  include/prc/oPRCFile.hpp
  include/prc/writePRC.hpp
  include/prc/Writer.hpp)

set(PRC_CPP
  src/PRCbitStream.cpp
  src/PRCdouble.cpp
  src/oPRCFile.cpp
  src/writePRC.cpp
  src/Writer.cpp)

set(PRC_SOURCES
  ${PRC_HPP}
  ${PRC_CPP})

add_library(${PRC_WRITER_NAME} SHARED ${PRC_SOURCES})
target_link_libraries(${PRC_WRITER_NAME}
                      ${PDAL_LIBRARY}
		      ${ZLIB_LIBRARY}
		      ${HPDF_LIBRARY})
set_target_properties(${PRC_WRITER_NAME} PROPERTIES
  SOVERSION "0.1.0" )

###############################################################################
# Targets installation

install(TARGETS ${PRC_WRITER_NAME}
  RUNTIME DESTINATION ${PRC_BIN_DIR}
  LIBRARY DESTINATION ${PRC_LIB_DIR}
  ARCHIVE DESTINATION ${PRC_LIB_DIR})
