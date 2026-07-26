cmake_minimum_required(VERSION 3.22)
project(epoch_android_pathway LANGUAGES CXX)

set(NATIVE_APP_GLUE "${ANDROID_NDK}/sources/android/native_app_glue")
add_library(native_app_glue STATIC "${NATIVE_APP_GLUE}/android_native_app_glue.c")
target_include_directories(native_app_glue PUBLIC "${NATIVE_APP_GLUE}")

add_library(epoch_android SHARED native_main.cpp)
target_compile_features(epoch_android PRIVATE cxx_std_23)
target_compile_definitions(epoch_android PRIVATE EPOCH_PLATFORM_ANDROID=1 EPOCH_GLES=1 EPOCH_TIER0_ONLY=1)
target_compile_options(epoch_android PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(epoch_android PRIVATE native_app_glue android log EGL GLESv3)
