#--------------------------------------------------------------------------
# Utils
# This module contains tools that can be good to have.
#--------------------------------------------------------------------------

PROJECT(Utils)
FILE(GLOB utils_headers code/*.h)
FILE(GLOB utils_sources code/*.cpp)

SET(files_utils ${utils_headers} ${utils_sources})
SOURCE_GROUP("utils" FILES ${files_utils})

ADD_LIBRARY(utils STATIC ${files_utils})

TARGET_INCLUDE_DIRECTORIES(utils INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/code/)