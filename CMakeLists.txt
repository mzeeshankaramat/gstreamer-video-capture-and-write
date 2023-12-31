cmake_minimum_required(VERSION 3.10)
project(gst-qt-app)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_AUTOMOC ON)

find_package(Qt5Widgets REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(OpenCV 4.5 REQUIRED)

find_package(PkgConfig)
pkg_check_modules(GST REQUIRED gstreamer-1.0)
pkg_check_modules(GST_VIDEO REQUIRED gstreamer-video-1.0)

add_executable(gst-qt-app app.cpp app.h)

target_link_libraries(gst-qt-app Qt5::Widgets Qt5::Core ${GST_LIBRARIES} ${GST_VIDEO_LIBRARIES} ${OpenCV_LIBS} )
target_include_directories(gst-qt-app PRIVATE ${GST_INCLUDE_DIRS} ${GST_VIDEO_INCLUDE_DIRS})
target_compile_options(gst-qt-app PRIVATE ${GST_CFLAGS_OTHER})

#Copy resources to the build directory
add_custom_target(copy_resources ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/resources
    ${CMAKE_CURRENT_BINARY_DIR}/resources
)

# Ensure the target that relies on the resources depends on the above target
add_dependencies(gst-qt-app copy_resources)
