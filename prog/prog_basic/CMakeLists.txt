#--------------------------------------------------------------------------
# ProgBasic
# ProgBasicrams that can run on hosts in the world
#--------------------------------------------------------------------------

PROJECT(ProgBasic)
FILE(GLOB progbasic_headers public/*.h)
FILE(GLOB progbasic_sources private/*.cpp)

SET(files_progbasic ${progbasic_headers} ${progbasic_sources})
SOURCE_GROUP("progbasic" FILES ${files_progbasic})

FIND_PACKAGE(CLI11 CONFIG REQUIRED)

ADD_LIBRARY(progbasic STATIC ${files_progbasic})
TARGET_LINK_LIBRARIES(progbasic PRIVATE common tech CLI11::CLI11)
TARGET_INCLUDE_DIRECTORIES(progbasic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/public)