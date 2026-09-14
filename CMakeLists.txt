cmake_minimum_required(VERSION 3.13.0)
set(CMAKE_CXX_STANDARD 23)
set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(PROJECT_NAME "FuryEngine")
project(${PROJECT_NAME})
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(FURY_BUILD_TESTS "Build fury engine tests" OFF)

add_subdirectory(libs)

file(GLOB_RECURSE SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/glsl/*
)

if (NOT WIN32)
	list(FILTER SOURCES	EXCLUDE	REGEX ".*platform/windows.*")
endif()

add_executable(${PROJECT_NAME} ${SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(${PROJECT_NAME} PUBLIC glfw glad glm imgui assimp ImGuiFileDialog imguizmo stb spdlog)
set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${ROOT_DIR})
if (WIN32)
	target_compile_definitions(${PROJECT_NAME} PRIVATE NOMINMAX)
endif()
target_compile_definitions(${PROJECT_NAME} PRIVATE GLM_FORCE_CTOR_INIT)

# For debugging
# if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
#     target_compile_options(${PROJECT_NAME} PRIVATE
#         -fsanitize=address
#         -fno-omit-frame-pointer
#         -g
#     )
#     target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address)
# endif()

if (FURY_BUILD_TESTS)
	add_subdirectory(tests)
endif()

