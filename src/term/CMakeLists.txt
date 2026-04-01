#--------------------------------------------------------------------------
# Term
# This module contains tools that can be good to have.
#--------------------------------------------------------------------------

PROJECT(Term)
FILE(GLOB term_headers public/*.h)
FILE(GLOB term_sources private/*.cpp)

SET(files_term ${term_headers} ${term_sources})
SOURCE_GROUP("term" FILES ${files_term})

include(FetchContent)
 
FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/Olaxan/FTXUI.git
  GIT_TAG main
)
 
FetchContent_MakeAvailable(ftxui)

ADD_LIBRARY(term STATIC ${files_term})
TARGET_LINK_LIBRARIES(term PUBLIC common ftxui::dom ftxui::screen ftxui::component)
TARGET_INCLUDE_DIRECTORIES(term PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/public)