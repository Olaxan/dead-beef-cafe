#--------------------------------------------------------------------------
# World
# This module emulates the living world and contains all network simulation,
# that can exist without the player's involvement.
#--------------------------------------------------------------------------

PROJECT(World)
FILE(GLOB world_headers code/*.h)
FILE(GLOB world_sources code/*.cpp)

SET(files_world ${world_headers} ${world_sources})
SOURCE_GROUP("world" FILES ${files_world})

ADD_LIBRARY(world STATIC ${files_world})

#TARGET_LINK_LIBRARIES(world PRIVATE x)

TARGET_INCLUDE_DIRECTORIES(world INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/code/)