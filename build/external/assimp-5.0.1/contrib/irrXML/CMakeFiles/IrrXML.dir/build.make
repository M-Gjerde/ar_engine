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

# Include any dependencies generated for this target.
include external/assimp-5.0.1/contrib/irrXML/CMakeFiles/IrrXML.dir/depend.make

# Include the progress variables for this target.
include external/assimp-5.0.1/contrib/irrXML/CMakeFiles/IrrXML.dir/progress.make

# Include the compile flags for this target's objects.
include external/assimp-5.0.1/contrib/irrXML/CMakeFiles/IrrXML.dir/flags.make

external/assimp-5.0.1/contrib/irrXML/CMakeFiles/IrrXML.dir/irrXML.cpp.o: external/assimp-5.0.1/contrib/irrXML/CMakeFiles/IrrXML.dir/flags.make
external/assimp-5.0.1/contrib/irrXML/CMakeFiles/IrrXML.dir/irrXML.cpp.o: ../external/assimp-5.0.1/contrib/irrXML/irrXML.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/magnus/CLionProjects/ar_engine/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object external/assimp-5.0.1/contrib/irrXML/CMakeFiles/IrrXML.dir/irrXML.cpp.o"
	cd /home/magnus/CLionProjects/ar_engine/build/external/assimp-5.0.1/contrib/irrXML && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/IrrXML.dir/irrXML.cpp.o -c /home/magnus/CLionProjects/ar_engine/external/assimp-5.0.1/contrib/irrXML/irrXML.cpp

external/assimp-5.0.1/contrib/irrXML/CMakeFiles/IrrXML.dir/irrXML.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/IrrXML.dir/irrXML.cpp.i"
	cd /home/magnus/CLionProjects/ar_engine/build/external/assimp-5.0.1/contrib/irrXML && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/magnus/CLionProjects/ar_engine/external/assimp-5.0.1/contrib/irrXML/irrXML.cpp > CMakeFiles/IrrXML.dir/irrXML.cpp.i

external/assimp-5.0.1/contrib/irrXML/CMakeFiles/IrrXML.dir/irrXML.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/IrrXML.dir/irrXML.cpp.s"
	cd /home/magnus/CLionProjects/ar_engine/build/external/assimp-5.0.1/contrib/irrXML && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/magnus/CLionProjects/ar_engine/external/assimp-5.0.1/contrib/irrXML/irrXML.cpp -o CMakeFiles/IrrXML.dir/irrXML.cpp.s

# Object files for target IrrXML
IrrXML_OBJECTS = \
"CMakeFiles/IrrXML.dir/irrXML.cpp.o"

# External object files for target IrrXML
IrrXML_EXTERNAL_OBJECTS =

external/assimp-5.0.1/contrib/irrXML/libIrrXMLd.a: external/assimp-5.0.1/contrib/irrXML/CMakeFiles/IrrXML.dir/irrXML.cpp.o
external/assimp-5.0.1/contrib/irrXML/libIrrXMLd.a: external/assimp-5.0.1/contrib/irrXML/CMakeFiles/IrrXML.dir/build.make
external/assimp-5.0.1/contrib/irrXML/libIrrXMLd.a: external/assimp-5.0.1/contrib/irrXML/CMakeFiles/IrrXML.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/magnus/CLionProjects/ar_engine/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX static library libIrrXMLd.a"
	cd /home/magnus/CLionProjects/ar_engine/build/external/assimp-5.0.1/contrib/irrXML && $(CMAKE_COMMAND) -P CMakeFiles/IrrXML.dir/cmake_clean_target.cmake
	cd /home/magnus/CLionProjects/ar_engine/build/external/assimp-5.0.1/contrib/irrXML && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/IrrXML.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
external/assimp-5.0.1/contrib/irrXML/CMakeFiles/IrrXML.dir/build: external/assimp-5.0.1/contrib/irrXML/libIrrXMLd.a

.PHONY : external/assimp-5.0.1/contrib/irrXML/CMakeFiles/IrrXML.dir/build

external/assimp-5.0.1/contrib/irrXML/CMakeFiles/IrrXML.dir/clean:
	cd /home/magnus/CLionProjects/ar_engine/build/external/assimp-5.0.1/contrib/irrXML && $(CMAKE_COMMAND) -P CMakeFiles/IrrXML.dir/cmake_clean.cmake
.PHONY : external/assimp-5.0.1/contrib/irrXML/CMakeFiles/IrrXML.dir/clean

external/assimp-5.0.1/contrib/irrXML/CMakeFiles/IrrXML.dir/depend:
	cd /home/magnus/CLionProjects/ar_engine/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/magnus/CLionProjects/ar_engine /home/magnus/CLionProjects/ar_engine/external/assimp-5.0.1/contrib/irrXML /home/magnus/CLionProjects/ar_engine/build /home/magnus/CLionProjects/ar_engine/build/external/assimp-5.0.1/contrib/irrXML /home/magnus/CLionProjects/ar_engine/build/external/assimp-5.0.1/contrib/irrXML/CMakeFiles/IrrXML.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : external/assimp-5.0.1/contrib/irrXML/CMakeFiles/IrrXML.dir/depend

