#--------------------------------------------------------------------------
# ProgUtils
# Utility programs for use in other programs
#--------------------------------------------------------------------------

PROJECT(ProgUtils)
FILE(GLOB progutils_headers public/*.h)
FILE(GLOB progutils_sources private/*.cpp)

SET(files_progutils ${progutils_headers} ${progutils_sources})
SOURCE_GROUP("progutils" FILES ${files_progutils})

include(FetchContent)
 
FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/Olaxan/FTXUI.git
  GIT_TAG dev-headless
)
 
FetchContent_MakeAvailable(ftxui)

ADD_LIBRARY(progutils STATIC ${files_progutils})
TARGET_LINK_LIBRARIES(progutils PUBLIC common term tech ftxui::dom ftxui::screen ftxui::component)
TARGET_INCLUDE_DIRECTORIES(progutils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/public)