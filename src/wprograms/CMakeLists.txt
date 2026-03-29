#--------------------------------------------------------------------------
# WorldProg
# This module emulates the living worldprog and contains all network simulation,
# that can exist without the player's involvement.
#--------------------------------------------------------------------------

PROJECT(WorldProg)
FILE(GLOB worldprog_headers public/*.h)
FILE(GLOB worldprog_sources private/*.cpp)

SET(files_worldprog ${worldprog_headers} ${worldprog_sources})
SOURCE_GROUP("worldprog" FILES ${files_worldprog})

ADD_LIBRARY(worldprog STATIC ${files_worldprog})

TARGET_LINK_LIBRARIES(worldprog PUBLIC world)
TARGET_INCLUDE_DIRECTORIES(worldprog PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/public)