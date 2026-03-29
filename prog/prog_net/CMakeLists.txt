#--------------------------------------------------------------------------
# ProgNet
# ProgNetrams that can run on hosts in the world
#--------------------------------------------------------------------------

PROJECT(ProgNet)
FILE(GLOB prognet_headers public/*.h)
FILE(GLOB prognet_sources private/*.cpp)

SET(files_prognet ${prognet_headers} ${prognet_sources})
SOURCE_GROUP("prognet" FILES ${files_prognet})

FIND_PACKAGE(asio CONFIG REQUIRED)
FIND_PACKAGE(CLI11 CONFIG REQUIRED)

ADD_LIBRARY(prognet STATIC ${files_prognet})
TARGET_LINK_LIBRARIES(prognet PRIVATE common tech CLI11::CLI11 asio::asio)
TARGET_INCLUDE_DIRECTORIES(prognet PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/public)