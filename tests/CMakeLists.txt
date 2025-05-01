cmake_minimum_required(VERSION 3.13.0)
set(CMAKE_CXX_STANDARD 17)
set(PROJECT_NAME "OpenGLEngineTests")
project(${PROJECT_NAME})

file(GLOB_RECURSE TESTS_SOURCES
	"${CMAKE_CURRENT_SOURCE_DIR}/*.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
	)

# TODO: make library from these sources and link them as lib
get_target_property(ENGINE_SOURCES OpenGLEngine SOURCES)
# exclude file with main.cpp
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*OpenGLMain.*")
get_target_property(ENGINE_INCLUDES OpenGLEngine INCLUDE_DIRECTORIES)
get_target_property(ENGINE_LINKED_LIBS OpenGLEngine LINK_LIBRARIES)

enable_testing()

add_executable(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp ${TESTS_SOURCES} ${ENGINE_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${ENGINE_INCLUDES})
target_link_libraries(${PROJECT_NAME} PRIVATE ${ENGINE_LINKED_LIBS} gtest gtest_main)
if (WIN32)
	target_compile_definitions(${PROJECT_NAME} PRIVATE NOMINMAX)
endif()

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${TESTS_SOURCES})
source_group(EngineSources FILES ${ENGINE_SOURCES} ${ENGINE_INCLUDES})

include(CTest)
add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
