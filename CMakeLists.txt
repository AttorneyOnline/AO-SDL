cmake_minimum_required (VERSION 3.8)

project ("tsurushiage" LANGUAGES CXX C)
add_executable ( ${CMAKE_PROJECT_NAME}
    "src/main.cpp"
    "src/utils/Log.cpp"
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE include)

add_subdirectory(lib/libsaiban)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE lib/libsaiban/include)

# SDL2
add_subdirectory(third-party/SDL)
if(TARGET SDL2::SDL2main)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE SDL2::SDL2main)
endif()
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE SDL2::SDL2)

