# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

EXTRA_COMPONENT_DIRS := $(realpath ../..)
EXCLUDE_COMPONENTS := test

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(TCC-monitor-MIT)