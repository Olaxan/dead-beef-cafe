#--------------------------------------------------------------------------
# ProgWorld
# This module emulates the living progworld and contains all network simulation,
# that can exist without the player's involvement.
#--------------------------------------------------------------------------

PROJECT(ProgWorld)
FILE(GLOB progworld_headers public/*.h)
FILE(GLOB progworld_sources private/*.cpp)

SET(files_progworld ${progworld_headers} ${progworld_sources})
SOURCE_GROUP("progworld" FILES ${files_progworld})

ADD_LIBRARY(progworld STATIC ${files_progworld})

TARGET_LINK_LIBRARIES(progworld PUBLIC world term)
TARGET_INCLUDE_DIRECTORIES(progworld PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/public)