#--------------------------------------------------------------------------
# Social
# This module contains social concepts (businesses, people, telephony, etc.)
#--------------------------------------------------------------------------

PROJECT(Social)
FILE(GLOB social_headers public/*.h)
FILE(GLOB social_sources private/*.cpp)

SET(files_social ${social_headers} ${social_sources})
SOURCE_GROUP("social" FILES ${files_social})

ADD_LIBRARY(social STATIC ${files_social})
TARGET_LINK_LIBRARIES(social PUBLIC common)
TARGET_INCLUDE_DIRECTORIES(social PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/public)