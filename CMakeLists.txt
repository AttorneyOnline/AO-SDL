cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("saiban" LANGUAGES CXX C)
add_executable ( ${CMAKE_PROJECT_NAME}
    "src/main.cpp"

    "src/render/Shader.cpp"

    "src/utils/Log.cpp"
    
)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE include)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE third-party/stb)

# SDL2
add_subdirectory(third-party/SDL)
if(TARGET SDL2::SDL2main)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE SDL2::SDL2main)
endif()
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE SDL2::SDL2)

find_package(OpenGL)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${OPENGL_LIBRARIES})

if (WIN32)
    # Use GLEW release binaries on Windows because Windows sucks ass
    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE third-party/glew/include)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third-party/glew/lib/Release/x64/glew32.lib)

    # Copy DLLs to binary location
    add_custom_command(
        TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/third-party/glew/bin/Release/x64/glew32.dll "$<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>"
        VERBATIM
    )
    add_custom_command(
        TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:SDL2::SDL2>" "$<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>"
        VERBATIM
    )
endif()