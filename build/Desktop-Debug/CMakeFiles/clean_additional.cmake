# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles/MirrorRoomSimulation_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/MirrorRoomSimulation_autogen.dir/ParseCache.txt"
  "MirrorRoomSimulation_autogen"
  )
endif()
