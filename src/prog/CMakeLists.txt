#--------------------------------------------------------------------------
# Prog
# Programs that can run on hosts in the world
#--------------------------------------------------------------------------

PROJECT(Prog)
FILE(GLOB prog_headers code/*.h)
FILE(GLOB prog_sources code/*.cpp)

SET(files_prog ${prog_headers} ${prog_sources})
SOURCE_GROUP("prog" FILES ${files_prog})

ADD_LIBRARY(prog STATIC ${files_prog})
TARGET_LINK_LIBRARIES(prog PRIVATE world)
TARGET_INCLUDE_DIRECTORIES(prog INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/code/)