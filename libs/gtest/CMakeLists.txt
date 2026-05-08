include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_SHALLOW TRUE
  GIT_TAG v1.16.0
)

set(BUILD_GTEST ON CACHE BOOL "" FORCE)
if (WIN32)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif()
FetchContent_MakeAvailable(googletest)
