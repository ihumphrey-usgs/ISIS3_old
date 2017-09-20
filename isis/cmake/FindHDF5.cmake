# CMake module for find_package(HDF5)
# Finds include directory and all applicable libraries
#
# Sets the following:
#   HDF5_INCLUDE_DIR
#   HDF5_LIBRARY

find_path(HDF5_INCLUDE_DIR
  NAME hdf5.h
  PATH_SUFFIXES hdf5
)

find_library(HDF5_LIBRARY         NAMES hdf5)
find_library(HDF5_CPP_LIBRARY     NAMES hdf5_cpp)
find_library(HDF5_HL_LIBRARY      NAMES hdf5_hl)
find_library(HDF5_HLCPP_LIBRARY   NAMES hdf5_hl_cpp)

get_filename_component(HDF5_ROOT_INCLUDE_DIR "${HDF5_INCLUDE_DIR}" DIRECTORY)
