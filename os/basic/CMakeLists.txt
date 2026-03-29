#--------------------------------------------------------------------------
# BasicOS
# BasicOSrams that can run on hosts in the world
#--------------------------------------------------------------------------

PROJECT(BasicOS)
FILE(GLOB basicos_headers public/*.h)
FILE(GLOB basicos_sources private/*.cpp)

SET(files_basicos ${basicos_headers} ${basicos_sources})
SOURCE_GROUP("basicos" FILES ${files_basicos})

ADD_LIBRARY(basicos STATIC ${files_basicos})
TARGET_LINK_LIBRARIES(basicos PRIVATE common tech progbasic)
TARGET_INCLUDE_DIRECTORIES(basicos PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/public)