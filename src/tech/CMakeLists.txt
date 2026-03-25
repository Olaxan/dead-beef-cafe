#--------------------------------------------------------------------------
# Tech
# This module emulates the living tech and contains all network simulation,
# that can exist without the player's involvement.
#--------------------------------------------------------------------------

PROJECT(Tech)
FILE(GLOB tech_headers public/*.h)
FILE(GLOB tech_sources private/*.cpp)

SET(files_tech ${tech_headers} ${tech_sources})
SOURCE_GROUP("tech" FILES ${files_tech})

ADD_LIBRARY(tech STATIC ${files_tech})

TARGET_LINK_LIBRARIES(tech PUBLIC utils services proto common)
TARGET_INCLUDE_DIRECTORIES(tech PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/public)