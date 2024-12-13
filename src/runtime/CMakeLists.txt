
target_sources(usdtweak PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/engine.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/frameRecorder.cpp
)

target_include_directories(usdtweak PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

