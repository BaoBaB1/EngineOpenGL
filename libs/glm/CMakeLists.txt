include(FetchContent)

FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_SHALLOW TRUE
  GIT_TAG        1.0.1
)

set(GLM_ENABLE_CXX_17 ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glm)

target_compile_definitions(glm PUBLIC GLM_ENABLE_EXPERIMENTAL)
