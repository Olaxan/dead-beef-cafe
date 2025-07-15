cmake_minimum_required(VERSION 3.12)
project(GuiApp)
set(CMAKE_CXX_STANDARD 23)

# Add imgui_bundle
# =================
# 1/  Option 1: if you added imgui_bundle in a subfolder, you can add it to your project with:
if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui_bundle)
    add_subdirectory(external/imgui_bundle)
endif()

## 2/  Option 2: simply fetch imgui_bundle during the build
if (NOT TARGET imgui_bundle)
    message(STATUS "Fetching imgui_bundle...")
    include(FetchContent)
    Set(FETCHCONTENT_QUIET FALSE)
    FetchContent_Declare(imgui_bundle GIT_REPOSITORY https://github.com/pthom/imgui_bundle.git GIT_TAG main)
    FetchContent_MakeAvailable(imgui_bundle)
    
    # Uncomment the next line if you which to also automatically fetch and compile OpenCV for immvision support
    # set(IMMVISION_FETCH_OPENCV ON)
    endif()


# Build your app
# ==============
# imgui_bundle_add_app is a helper function, similar to cmake's "add_executable"
# Usage:
# imgui_bundle_add_app(app_name file1.cpp file2.cpp ...)
#
# Features:
# * It will automatically link the target to the required libraries (hello_imgui, OpenGl, glad, etc)
# * It will embed the assets (for desktop, mobile, and emscripten apps)
# * It will perform additional customization (app icon and name on mobile platforms, etc)
#
# Now you can build your app with
#     mkdir build && cd build && cmake .. && cmake --build .

# By default, we install in a portable way (i.e. assets and executable are in the same folder)

set(HELLOIMGUI_USE_GLFW3 ON CACHE BOOL "" FORCE)
set(HELLOIMGUI_HAS_OPENGL3 ON CACHE BOOL "" FORCE)

FILE(GLOB gui_headers code/*.h)
FILE(GLOB gui_sources code/*.cpp)

SET(files_gui ${gui_headers} ${gui_sources})
SOURCE_GROUP("gui" FILES ${files_gui})

imgui_bundle_add_app(dbc_gui dbc_gui.cpp ${files_gui})

TARGET_LINK_LIBRARIES(dbc_gui PRIVATE world)
TARGET_INCLUDE_DIRECTORIES(dbc_gui INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/code/)