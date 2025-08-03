cmake_minimum_required(VERSION 3.10)
project(donkeycar_lcc_integration)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find required packages
find_package(Qt5 COMPONENTS Core Gui Widgets Quick QuickControls2 REQUIRED)
find_package(PkgConfig REQUIRED)

# Set paths for CPM libraries
set(CPM_LIB_DIR ${CMAKE_SOURCE_DIR}/../../cpm_lab/cpm_lib)
set(CPM_LIB_BUILD_DIR ${CPM_LIB_DIR}/build)

# Include directories for CPM
include_directories(
    ${CPM_LIB_DIR}/include
    ${CPM_LIB_BUILD_DIR}/include
)

# Link directories for CPM
link_directories(
    ${CPM_LIB_BUILD_DIR}/lib
)

# Define the source files
set(SOURCES
    DonkeycarCameraAggregator.cpp
    DonkeycarImageProvider.cpp
    DonkeycarPluginFactory.cpp
)

# Define the header files
set(HEADERS
    DonkeycarCameraAggregator.hpp
    DonkeycarImageProvider.hpp
    DonkeycarPluginFactory.hpp
)

# Define the QML files
set(QML_FILES
    DonkeycarCameraViewer.qml
)

# Create a shared library
add_library(donkeycar_lcc_integration SHARED ${SOURCES} ${HEADERS})

# Link the library with Qt and CPM libraries
target_link_libraries(donkeycar_lcc_integration
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Quick
    Qt5::QuickControls2
    cpm_lib
    fastrtps
)

# Install the library and QML files
install(TARGETS donkeycar_lcc_integration
    LIBRARY DESTINATION lib
)

install(FILES ${QML_FILES}
    DESTINATION share/donkeycar_lcc_integration/qml
)

# Create a simple test application
add_executable(donkeycar_camera_viewer_test 
    test/test_camera_viewer.cpp
    test/resources.qrc
)

target_link_libraries(donkeycar_camera_viewer_test
    donkeycar_lcc_integration
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Quick
    Qt5::QuickControls2
)