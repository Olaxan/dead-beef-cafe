#--------------------------------------------------------------------------
# Programs
# Programsrams that can run on hosts in the world
#--------------------------------------------------------------------------

PROJECT(Programs)
FILE(GLOB programs_headers public/*.h)
FILE(GLOB programs_sources private/*.cpp)

SET(files_programs ${programs_headers} ${programs_sources})
SOURCE_GROUP("programs" FILES ${files_programs})

FIND_PACKAGE(ICU COMPONENTS uc io i18n REQUIRED)
FIND_PACKAGE(CLI11 CONFIG REQUIRED)

ADD_LIBRARY(programs STATIC ${files_programs})
TARGET_LINK_LIBRARIES(programs PRIVATE world ICU::uc ICU::io ICU::i18n CLI11::CLI11)
TARGET_INCLUDE_DIRECTORIES(programs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/public)