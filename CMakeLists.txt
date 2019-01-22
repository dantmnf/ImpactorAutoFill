cmake_minimum_required (VERSION 3.0)
project (ImpactorAutoFill C CXX)

set(CompilerFlags CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE)
foreach(CompilerFlag ${CompilerFlags})
  string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
endforeach()

if(MSVC)
  set_source_files_properties(loader.c autofill.c PROPERTIES LANGUAGE CXX)
else()
  message(WARNING "Detours library only supports MSVC")
endif()
add_definitions(-D_WIN32_WINNT=0x0700)
include_directories(detours/src)
add_library(detours STATIC
    detours/src/detours.cpp
    detours/src/modules.cpp
    detours/src/disasm.cpp
    detours/src/image.cpp
    detours/src/creatwth.cpp
    detours/src/disolx86.cpp
    detours/src/disolx64.cpp
    detours/src/disolia64.cpp
    detours/src/disolarm.cpp
    detours/src/disolarm64.cpp
)
add_definitions(-D_CRT_SECURE_NO_WARNINGS)
add_executable(loader WIN32 loader.c)
add_library(autofill SHARED autofill.c version-shim.c version-shim.def)
target_link_libraries(loader detours)
target_link_libraries(autofill detours advapi32 credui)
