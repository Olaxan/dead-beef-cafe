#--------------------------------------------------------------------------
# World
# This module emulates the living world and contains all network simulation,
# that can exist without the player's involvement.
#--------------------------------------------------------------------------

PROJECT(World)
FILE(GLOB world_headers public/*.h)
FILE(GLOB world_sources private/*.cpp)

SET(files_world ${world_headers} ${world_sources})
SOURCE_GROUP("world" FILES ${files_world})

ADD_LIBRARY(world STATIC ${files_world})

TARGET_LINK_LIBRARIES(world PUBLIC tech social services utils proto common)
TARGET_INCLUDE_DIRECTORIES(world PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/public)