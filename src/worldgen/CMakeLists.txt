#--------------------------------------------------------------------------
# WorldGen
# This module implements features to create a game world from configurable
# parameters.
#--------------------------------------------------------------------------

PROJECT(WorldGen)
FILE(GLOB worldgen_headers public/*.h)
FILE(GLOB worldgen_sources private/*.cpp)

SET(files_worldgen ${worldgen_headers} ${worldgen_sources})
SOURCE_GROUP("worldgen" FILES ${files_worldgen})

ADD_LIBRARY(worldgen STATIC ${files_worldgen})

TARGET_LINK_LIBRARIES(worldgen PUBLIC social world utils)
TARGET_INCLUDE_DIRECTORIES(worldgen PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/public)