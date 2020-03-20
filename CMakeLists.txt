cmake_minimum_required(VERSION 3.13)
project(TexasTextureConverter)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_executable(${PROJECT_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp"
                               "${CMAKE_CURRENT_SOURCE_DIR}/include/MainTexasWindow.hpp"
                               "${CMAKE_CURRENT_SOURCE_DIR}/include/ImageTab.hpp"
                               "${CMAKE_CURRENT_SOURCE_DIR}/src/MainTexasWindow.cpp"
                               "${CMAKE_CURRENT_SOURCE_DIR}/src/ImageTab.cpp")
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

set(Qt5_DIR "C:/Qt/5.14.1/msvc2017_64/lib/cmake/Qt5")
#set(Qt5_DIR "C:/Qt/5.12.6/msvc2017/lib/cmake/Qt5")
find_package(Qt5 COMPONENTS Widgets REQUIRED)
target_link_libraries(${PROJECT_NAME} Qt5::Widgets)


#set(TEXAS_ENABLE_PNG_READ OFF)
#set(TEXAS_ENABLE_KTX_READ OFF)
#set(TEXAS_ENABLE_KTX_SAVE OFF)
#set(TEXAS_ENABLE_DYNAMIC_ALLOCATIONS OFF)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Texas")
target_link_libraries(${PROJECT_NAME} Texas)
