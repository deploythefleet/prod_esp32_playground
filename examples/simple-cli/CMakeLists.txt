cmake_minimum_required(VERSION 3.16)

# Using C++ 17
set(CMAKE_CXX_STANDARD 17)

# This must be above the include of project.cmake to 
# properly set the sdkconfig file location
set(SDKCONFIG "${CMAKE_BINARY_DIR}/sdkconfig")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Enable minimal build configuration. This drastically reduces the
# build time by not compiling modules you don't use. The trade off 
# is that you have to manually include any components your project
# depends on in the CMakeLists.txt files.
idf_build_set_property(MINIMAL_BUILD ON)

# Set your project name here. This will be used to name your binary
project(simple-cli)
