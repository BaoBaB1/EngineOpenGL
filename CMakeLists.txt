cmake_minimum_required(VERSION 3.13.0)
set(CMAKE_CXX_STANDARD 17)
set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(PROJECT_NAME "OpenGLEngine")
project(${PROJECT_NAME})
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_subdirectory(libs)

file(GLOB_RECURSE SOURCES
	"src/*.h"
	"src/*.hpp"
	"src/*.cpp"
	"src/glsl/*.*"
	)

add_executable(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/src/core/OpenGLMain.cpp ${SOURCES})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_definitions(glm PUBLIC GLM_ENABLE_EXPERIMENTAL)
target_link_libraries(${PROJECT_NAME} PUBLIC glfw glad glm imgui assimp ImGuiFileDialog ImGuizmo stb)
set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${ROOT_DIR})

add_subdirectory(tests)

