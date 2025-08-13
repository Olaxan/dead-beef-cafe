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

#TARGET_LINK_LIBRARIES(world PRIVATE x)
FIND_PACKAGE(Protobuf REQUIRED)
FIND_PACKAGE(ICU COMPONENTS uc io i18n REQUIRED)

TARGET_LINK_LIBRARIES(world PUBLIC utils proto protobuf::libprotobuf ICU::uc ICU::io ICU::i18n)
TARGET_INCLUDE_DIRECTORIES(world PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/public)