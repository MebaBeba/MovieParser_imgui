cmake_minimum_required(VERSION 3.10)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(ImGuiApp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(CURL REQUIRED libcurl)
pkg_check_modules(JSON REQUIRED nlohmann_json)

set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ImGui)

set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp      # ADD THIS BACK
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

set(IMGUI_HEADERS
    ${IMGUI_DIR}/imgui.h
    ${IMGUI_DIR}/imgui_internal.h
    ${IMGUI_DIR}/imconfig.h
    ${IMGUI_DIR}/backends/imgui_impl_glfw.h
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.h
)

set(PROJECT_SOURCES
    main.cpp
    MovieDB.cpp
)

set(PROJECT_HEADERS
    MovieDB.h
)

add_executable(${PROJECT_NAME} 
    ${PROJECT_SOURCES} 
    ${PROJECT_HEADERS}
    ${IMGUI_SOURCES} 
    ${IMGUI_HEADERS}
)

# Set the GLFW backend file to be compiled as Objective-C++ on macOS
if(APPLE)
    set_source_files_properties(${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        PROPERTIES
        COMPILE_FLAGS "-x objective-c++"
    )
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${GLFW_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
    ${JSON_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIR}
)

if(APPLE)
    target_include_directories(${PROJECT_NAME} PRIVATE
        /opt/homebrew/include
        /usr/local/include
    )
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${OPENGL_LIBRARIES}
    glfw
    ${CURL_LIBRARIES}
    pthread
    dl
)

if(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
        "-framework CoreFoundation"
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE GL_SILENCE_DEPRECATION)
endif()