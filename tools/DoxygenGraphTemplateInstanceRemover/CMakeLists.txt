# #################################################################################################
#  CMakeLists.txt - ALox Tools: DoxygenGraphTemplateInstanceRemover
#
#  Copyright 2015-2018 A-Worx GmbH, Germany
#  Published under 'Boost Software License' (a free software license, see LICENSE.txt)
#
#  How to use:
#   1. Enter subdirectory 'debug'   and invoke:  `cmake ..`
#   2. Enter subdirectory 'release' and invoke:  `cmake -DCMAKE_BUILD_TYPE=Release ..`
#   3. Invoke `make` within any of these directories.
#   More information is found in ALox documentation.
# ##################################################################################################

# -------------------------------------------------------------------------------------------------
# Prior to setting CMake project
# -------------------------------------------------------------------------------------------------

    # uncomment to force cotire on clean cmake built
    #set( ALIB_CMAKE_COTIRE   "On" )

# -------------------------------------------------------------------------------------------------
# General
# -------------------------------------------------------------------------------------------------

    project( "DoxygenGraphTemplateInstanceRemover" )
    cmake_minimum_required(VERSION 2.8)

    # this defaults to a release version!
    if ( "${CMAKE_BUILD_TYPE}" STREQUAL ""    )
        MESSAGE( STATUS "Default build type 'Release' chosen" )
        set( CMAKE_BUILD_TYPE "Release" )
    endif()

## -------------------------------------------------------------------------------------------------
## find libraries
## -------------------------------------------------------------------------------------------------

    # Multithreading
    find_package(Threads REQUIRED)

## -------------------------------------------------------------------------------------------------
## include ALib/ALox base cmake file
## -------------------------------------------------------------------------------------------------

    list( APPEND  ALIB_MODULES  "ALOX"        )
    include( ${CMAKE_CURRENT_LIST_DIR}/../../build/cmake/ALib.cmake )


# -------------------------------------------------------------------------------------------------
# sources
# -------------------------------------------------------------------------------------------------
aux_source_directory(. SRC_LIST)


# -------------------------------------------------------------------------------------------------
# The target
# -------------------------------------------------------------------------------------------------
    ALibAddStaticLibrary()

    add_executable( dot ${SRC_LIST})
    ALibSetCompilerAndLinker( dot )

    target_compile_definitions( dot   PUBLIC   ${ALIB_COMPILATION_SYMBOLS} )


#! [DOXYGEN_CMAKE_COTIRE_TARGET]
# link against unity-built version of library in case of cotire
if( ${ALIB_CMAKE_COTIRE} )
    target_link_libraries     ( dot   ALib_StaticLib_unity  )
else()
    target_link_libraries     ( dot   ALib_StaticLib        )
endif()
#! [DOXYGEN_CMAKE_COTIRE_TARGET]

    set( EXECUTABLE_OUTPUT_PATH       ${ALIB_BASE_DIR}/docs/doxygen)


# -------------------------------------------------------------------------------------------------
# Precompiled headers with 'cotire' CMake script.
# For more Info see 'ALox.cmake' which downloads the script and the cotire website at
# https://github.com/sakra/cotire/
# -------------------------------------------------------------------------------------------------
    if( ${ALIB_CMAKE_COTIRE} )
        set_target_properties( dot   PROPERTIES COTIRE_UNITY_SOURCE_MAXIMUM_NUMBER_OF_INCLUDES "-j" )
        set_target_properties( dot   PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT  "${ALIB_SOURCE_DIR}/alib/alib.hpp" )
        cotire( dot )
    endif()
