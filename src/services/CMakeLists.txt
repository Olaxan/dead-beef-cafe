#--------------------------------------------------------------------------
# Services
# This module emulates the living services and contains all network simulation,
# that can exist without the player's involvement.
#--------------------------------------------------------------------------

PROJECT(Services)
FILE(GLOB services_headers public/*.h)
FILE(GLOB services_sources private/*.cpp)

SET(files_services ${services_headers} ${services_sources})
SOURCE_GROUP("services" FILES ${files_services})

ADD_LIBRARY(services STATIC ${files_services})

#TARGET_LINK_LIBRARIES(services PUBLIC)
TARGET_INCLUDE_DIRECTORIES(services PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/public)