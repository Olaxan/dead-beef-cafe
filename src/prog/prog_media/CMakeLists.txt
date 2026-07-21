#--------------------------------------------------------------------------
# ProgMedia
# This module emulates the living progmedia and contains all network simulation,
# that can exist without the player's involvement.
#--------------------------------------------------------------------------

PROJECT(ProgMedia)
FILE(GLOB progmedia_headers public/*.h)
FILE(GLOB progmedia_sources private/*.cpp)

SET(files_progmedia ${progmedia_headers} ${progmedia_sources})
SOURCE_GROUP("progmedia" FILES ${files_progmedia})

ADD_LIBRARY(progmedia STATIC ${files_progmedia})

TARGET_LINK_LIBRARIES(progmedia PUBLIC common term tech soloud)
TARGET_INCLUDE_DIRECTORIES(progmedia PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/public)