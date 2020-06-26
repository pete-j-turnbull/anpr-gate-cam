cmake_minimum_required(VERSION 2.8)
set (CMAKE_CXX_STANDARD 11)
project( AnprGateCam )

find_package( OpenCV REQUIRED )

include_directories( ${OpenCV_INCLUDE_DIRS} ./src )
set(SOURCES main.cpp ./src/feature.h ./src/train.h ./src/recog.h )

add_executable( AnprGateCam ${SOURCES} )

target_link_libraries( AnprGateCam ${OpenCV_LIBS})
