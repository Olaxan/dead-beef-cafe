#--------------------------------------------------------------------------
# Common
# This module emulates the living common and contains all network simulation,
# that can exist without the player's involvement.
#--------------------------------------------------------------------------

PROJECT(Common)
FILE(GLOB common_headers public/*.h)
FILE(GLOB common_sources private/*.cpp)

SET(files_common ${common_headers} ${common_sources})
SOURCE_GROUP("common" FILES ${files_common})

ADD_LIBRARY(common STATIC ${files_common})

FIND_PACKAGE(Threads REQUIRED)
FIND_PACKAGE(Protobuf REQUIRED)
FIND_PACKAGE(ICU COMPONENTS uc io i18n REQUIRED)

TARGET_LINK_LIBRARIES(common PUBLIC proto protobuf::libprotobuf ICU::uc ICU::io ICU::i18n)
TARGET_INCLUDE_DIRECTORIES(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/public)