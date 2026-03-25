#--------------------------------------------------------------------------
# Utils
# This module contains tools that can be good to have.
#--------------------------------------------------------------------------

PROJECT(Utils)
FILE(GLOB utils_headers public/*.h)
FILE(GLOB utils_sources private/*.cpp)

SET(files_utils ${utils_headers} ${utils_sources})
SOURCE_GROUP("utils" FILES ${files_utils})

ADD_LIBRARY(utils STATIC ${files_utils})
TARGET_LINK_LIBRARIES(utils PUBLIC common)
TARGET_INCLUDE_DIRECTORIES(utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/public)