cmake_minimum_required(VERSION 3.16)

# Sets project name
project(final_project LANGUAGES CXX C)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Sets C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Specifies required Qt components
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Gui)
find_package(Qt6 REQUIRED COMPONENTS OpenGL)
find_package(Qt6 REQUIRED COMPONENTS OpenGLWidgets)
find_package(Qt6 REQUIRED COMPONENTS Xml)

# Allows you to include files from within those directories, without prefixing their filepaths
include_directories(src)

# Specifies .cpp and .h files to be passed to the compiler
add_executable(${PROJECT_NAME}
    src/main.cpp

    src/mainwindow.cpp

    src/mainwindow.h
    src/utils/aspectratiowidget/aspectratiowidget.hpp

    src/utils/rgba.h
    src/settings.h
    src/settings.cpp
    src/utils/sceneparser.h src/utils/sceneparser.cpp
    src/utils/scenedata.h
    src/utils/scenefilereader.h src/utils/scenefilereader.cpp
    src/camera/camera.cpp src/camera/camera.h

    src/raymarch/raymarchscene.h src/raymarch/raymarchscene.cpp
    src/raymarch/raymarchobj.h

    src/realtime.h src/realtime.cpp
    src/realtimerender.cpp
    resources/raymarch.frag resources/raymarch.vert
    src/utils/shaderloader.h
    resources/fxaa.frag
    resources/fullscreen.vert
    resources/mvp.vert

    src/utils/ltc_matrix.h
    resources/hdr.frag
    resources/color.frag
    resources/blur.frag
)

# GLM: this creates its library and allows you to `#include "glm/..."`
add_subdirectory(glm)

# GLEW: this creates its library and allows you to `#include "GL/glew.h"`
add_library(StaticGLEW STATIC glew/src/glew.c)
include_directories(${PROJECT_NAME} PRIVATE glew/include)

# Specifies libraries to be linked (Qt components, glew, etc)
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt::Core
    Qt::Gui
    Qt::OpenGL
    Qt::OpenGLWidgets
    Qt::Xml
    StaticGLEW
)

# Specifies other files
qt6_add_resources(${PROJECT_NAME} "Resources"
    PREFIX
        "/"
    FILES
        resources/raymarch.frag
        resources/raymarch.vert
        resources/fullscreen.vert
        resources/fxaa.frag
        resources/mvp.vert
        resources/hdr.frag
        resources/color.frag
        resources/blur.frag
)

# GLEW: this provides support for Windows (including 64-bit)
if (WIN32)
  add_compile_definitions(GLEW_STATIC)
  target_link_libraries(${PROJECT_NAME} PRIVATE
    opengl32
    glu32
  )
endif()

# Set this flag to silence warnings on Windows
if (MSVC OR MSYS OR MINGW)
  set(CMAKE_CXX_FLAGS "-Wno-volatile")
endif()
# Set this flag to silence warnings on MacOS
if (APPLE)
  set(CMAKE_CXX_FLAGS "-Wno-deprecated-volatile")
endif()
