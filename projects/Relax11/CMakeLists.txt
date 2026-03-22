# Top-level CMakeLists.txt for the Relax11 project.
#
# Build instructions (from your Chaste build directory):
#   cmake <path-to-Chaste-source>   # re-run cmake to pick up the new project
#   make Relax11                    # build the executable

cmake_minimum_required(VERSION 3.16.3)

find_package(Chaste COMPONENTS cell_based)

chaste_do_project(Relax11)
