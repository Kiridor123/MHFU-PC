cmake_minimum_required(VERSION 3.20)

set(MHFU_PROFILE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "Configuring Monster Hunter Freedom Unite profile (ULUS-10391)")

# The AOT corpus is derived from the user's executable and is not committed.
# Until scripts/generate.py has produced it, link an empty stub instead.
file(GLOB MHFU_GENERATED CONFIGURE_DEPENDS "${MHFU_PROFILE_DIR}/generated/*.cpp")

if(MHFU_GENERATED)
    list(LENGTH MHFU_GENERATED MHFU_GENERATED_COUNT)
    message(STATUS "mhfu: ${MHFU_GENERATED_COUNT} generated AOT units found")
    if(MSVC)
        set(MHFU_GENERATED_OPTIONS "/O2;/bigobj")
    else()
        set(MHFU_GENERATED_OPTIONS "-O2;-g0")
    endif()
    set_source_files_properties(${MHFU_GENERATED} PROPERTIES COMPILE_OPTIONS "${MHFU_GENERATED_OPTIONS}")
    add_library(mhfu_generated OBJECT ${MHFU_GENERATED})
    target_include_directories(mhfu_generated PRIVATE
        "${PROJECT_SOURCE_DIR}/include"
        "${MHFU_PROFILE_DIR}/generated")
else()
    message(STATUS "mhfu: no generated AOT units found yet in profiles/mhfu/generated; using stub")
    add_library(mhfu_generated OBJECT host/generated_stub.cpp)
    target_include_directories(mhfu_generated PRIVATE
        "${PROJECT_SOURCE_DIR}/include")
endif()
