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

set(QT_SRC_DIR C:/Qt/5.15.0/msvc2019_64)

set(Qt5_DIR ${QT_SRC_DIR}/lib/cmake/Qt5)
find_package(Qt5 COMPONENTS Widgets REQUIRED)
target_link_libraries(${PROJECT_NAME} Qt5::Widgets)

if (MSVC)
	add_custom_command(
		TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy
		${QT_SRC_DIR}/bin/Qt5Cored.dll
		$<TARGET_FILE_DIR:${PROJECT_NAME}>)

			add_custom_command(
		TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy
		${QT_SRC_DIR}/bin/Qt5Widgetsd.dll
		$<TARGET_FILE_DIR:${PROJECT_NAME}>)

			add_custom_command(
		TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy
		${QT_SRC_DIR}/bin/Qt5Guid.dll
		$<TARGET_FILE_DIR:${PROJECT_NAME}>)
endif()


#set(TEXAS_ENABLE_PNG_READ OFF)
#set(TEXAS_ENABLE_KTX_READ OFF)
#set(TEXAS_ENABLE_KTX_SAVE OFF)
#set(TEXAS_ENABLE_DYNAMIC_ALLOCATIONS OFF)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Texas")
target_link_libraries(${PROJECT_NAME} Texas)
