#--------------------------------------------------------------------------
# Abstr
# Abstractions: i.e. interface code performing collected OS functions.
#--------------------------------------------------------------------------

PROJECT(Abstr)
FILE(GLOB abstr_headers public/*.h)
FILE(GLOB abstr_sources private/*.cpp)

SET(files_abstr ${abstr_headers} ${abstr_sources})
SOURCE_GROUP("abstr" FILES ${files_abstr})

FIND_PACKAGE(ICU COMPONENTS uc io i18n REQUIRED)
FIND_PACKAGE(CLI11 CONFIG REQUIRED)

ADD_LIBRARY(abstr STATIC ${files_abstr})
TARGET_LINK_LIBRARIES(abstr PRIVATE world ICU::uc ICU::io ICU::i18n CLI11::CLI11)
TARGET_INCLUDE_DIRECTORIES(abstr PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/public)