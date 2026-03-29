#--------------------------------------------------------------------------
# WPrograms
# This module emulates the living wprograms and contains all network simulation,
# that can exist without the player's involvement.
#--------------------------------------------------------------------------

PROJECT(WPrograms)
FILE(GLOB wprograms_headers public/*.h)
FILE(GLOB wprograms_sources private/*.cpp)

SET(files_wprograms ${wprograms_headers} ${wprograms_sources})
SOURCE_GROUP("wprograms" FILES ${files_wprograms})

ADD_LIBRARY(wprograms STATIC ${files_wprograms})

TARGET_LINK_LIBRARIES(wprograms PUBLIC world)
TARGET_INCLUDE_DIRECTORIES(wprograms PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/public)