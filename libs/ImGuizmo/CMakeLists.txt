include(FetchContent)

FetchContent_Declare(
  ImGuizmo
  GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git
  GIT_SHALLOW TRUE
  GIT_TAG 1.9
)

set (IMGUIZMO_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(ImGuizmo)

target_link_libraries(imguizmo PUBLIC imgui)
