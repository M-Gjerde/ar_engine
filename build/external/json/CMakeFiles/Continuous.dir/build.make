# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.17

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Disable VCS-based implicit rules.
% : %,v


# Disable VCS-based implicit rules.
% : RCS/%


# Disable VCS-based implicit rules.
% : RCS/%,v


# Disable VCS-based implicit rules.
% : SCCS/s.%


# Disable VCS-based implicit rules.
% : s.%


.SUFFIXES: .hpux_make_needs_suffix_list


# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /snap/clion/129/bin/cmake/linux/bin/cmake

# The command to remove a file.
RM = /snap/clion/129/bin/cmake/linux/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/magnus/CLionProjects/ar_engine

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/magnus/CLionProjects/ar_engine/build

# Utility rule file for Continuous.

# Include the progress variables for this target.
include external/json/CMakeFiles/Continuous.dir/progress.make

external/json/CMakeFiles/Continuous:
	cd /home/magnus/CLionProjects/ar_engine/build/external/json && /snap/clion/129/bin/cmake/linux/bin/ctest -D Continuous

Continuous: external/json/CMakeFiles/Continuous
Continuous: external/json/CMakeFiles/Continuous.dir/build.make

.PHONY : Continuous

# Rule to build all files generated by this target.
external/json/CMakeFiles/Continuous.dir/build: Continuous

.PHONY : external/json/CMakeFiles/Continuous.dir/build

external/json/CMakeFiles/Continuous.dir/clean:
	cd /home/magnus/CLionProjects/ar_engine/build/external/json && $(CMAKE_COMMAND) -P CMakeFiles/Continuous.dir/cmake_clean.cmake
.PHONY : external/json/CMakeFiles/Continuous.dir/clean

external/json/CMakeFiles/Continuous.dir/depend:
	cd /home/magnus/CLionProjects/ar_engine/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/magnus/CLionProjects/ar_engine /home/magnus/CLionProjects/ar_engine/external/json /home/magnus/CLionProjects/ar_engine/build /home/magnus/CLionProjects/ar_engine/build/external/json /home/magnus/CLionProjects/ar_engine/build/external/json/CMakeFiles/Continuous.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : external/json/CMakeFiles/Continuous.dir/depend

