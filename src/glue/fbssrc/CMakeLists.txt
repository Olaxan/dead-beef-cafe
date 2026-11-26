#--------------------------------------------------------------------------
# fbssrc
# fbssrcrams that can run on hosts in the world
#--------------------------------------------------------------------------

PROJECT(FlatbufDatasource)
FILE(GLOB fbssrc_headers public/*.h)
FILE(GLOB fbssrc_sources private/*.cpp)

SET(files_fbssrc ${fbssrc_headers} ${fbssrc_sources})
SOURCE_GROUP("fbssrc" FILES ${files_fbssrc})

FIND_PACKAGE(mio REQUIRED)

ADD_LIBRARY(FlatbufDatasource STATIC ${files_fbssrc})
TARGET_LINK_LIBRARIES(FlatbufDatasource PRIVATE flatbuffers_schemas common utils tech mio::mio)
TARGET_INCLUDE_DIRECTORIES(FlatbufDatasource PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/public)