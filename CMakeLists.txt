# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

# Set the project name
set(PROJECT_NAME "sip_door_station")

# Set partition table file
set(PARTITION_TABLE_CUSTOM_FILENAME partitions.csv)

# Load environment variables from .env file if it exists
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.env")
    message(STATUS "Loading configuration from .env file")
    file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/.env" ENV_VARS)
    foreach(ENV_VAR ${ENV_VARS})
        # Skip comments and empty lines
        if(NOT ENV_VAR MATCHES "^#" AND NOT ENV_VAR STREQUAL "")
            # Parse KEY=VALUE format
            string(REGEX MATCH "^([^=]+)=(.*)$" MATCH_RESULT ${ENV_VAR})
            if(MATCH_RESULT)
                set(ENV_KEY ${CMAKE_MATCH_1})
                set(ENV_VALUE ${CMAKE_MATCH_2})
                # Set as environment variable for the build
                set(ENV{${ENV_KEY}} ${ENV_VALUE})
                message(STATUS "Set ${ENV_KEY}=${ENV_VALUE}")
            endif()
        endif()
    endforeach()
else()
    message(STATUS "No .env file found, using system environment variables only")
endif()

# Force include test component if test mode is enabled
if(DEFINED ENV{RUN_TESTS})
    message(STATUS "Test mode detected - forcing test component inclusion")
    set(EXTRA_COMPONENT_DIRS "test")
endif()

# Include ESP-IDF build system
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Define the project
project(${PROJECT_NAME})

# Add web_root directory to SPIFFS image
spiffs_create_partition_image(spiffs web_root FLASH_IN_PROJECT)