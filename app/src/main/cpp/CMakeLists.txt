cmake_minimum_required(VERSION 3.22.1)
project(RootInjector CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- payload: the library injected into the target process ---
add_library(payload SHARED
    payload/payload_main.cpp
)
target_link_libraries(payload log)
set_target_properties(payload PROPERTIES OUTPUT_NAME "payload")

# --- injector: a standalone PIE binary, run as root via `su -c`. It is
# never dlopen'd by anything, only executed directly, so renaming its
# output to a .so is purely so the Android Gradle Plugin bundles it next to
# the real shared libraries in lib/<ABI>/ instead of discarding it. Confirm
# after a build that lib/arm64-v8a/libinjector.so exists in the APK and is
# sized like a real binary, not 0 bytes -- see README "Packaging check".
add_executable(libinjector
    injector/injector_main.cpp
    injector/proc_maps.cpp
    injector/remote_call_arm64.cpp
)
target_link_libraries(libinjector log dl)
set_target_properties(libinjector PROPERTIES SUFFIX ".so")
