cmake_minimum_required(VERSION 3.28)
set(VCPKG_TARGET_TRIPLET x64-windows)
project(realtime-bilingual-asr)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

IF (NOT CMAKE_BUILD_TYPE STREQUAL Debug)
    include("VC-LTL.cmake")
ENDIF ()

include_directories(${CMAKE_SOURCE_DIR}/wtfdanmaku/include)

find_package(ixwebsocket REQUIRED)
find_package(nlohmann_json REQUIRED)

add_executable(${PROJECT_NAME}
        main.cpp
        AudioCapture.cpp
        SpeechRecognition.cpp
        danmaku.cpp
        assets.rc
)

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        ixwebsocket::ixwebsocket
        nlohmann_json::nlohmann_json
        ole32
        mfplat
        mf
        wmcodecdspuuid
        "${CMAKE_SOURCE_DIR}/sherpa-onnx-c-api.lib"
        bcrypt
)
IF (NOT CMAKE_BUILD_TYPE STREQUAL Debug)
    target_link_libraries(${PROJECT_NAME}
            PRIVATE
            ${CMAKE_SOURCE_DIR}/wtfdanmaku/lib/release/libwtfdanmaku.lib
    )
    file(COPY ${CMAKE_SOURCE_DIR}/wtfdanmaku/lib/release/libwtfdanmaku.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}/)
 ELSE ()
    target_link_libraries(${PROJECT_NAME}
            PRIVATE
            ${CMAKE_SOURCE_DIR}/wtfdanmaku/lib/debug/libwtfdanmaku.lib
    )
    file(COPY ${CMAKE_SOURCE_DIR}/wtfdanmaku/lib/debug/libwtfdanmaku.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}/)
ENDIF ()
