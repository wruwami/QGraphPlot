cmake_minimum_required(VERSION 3.16)
project(ConsumerTest LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QGraphPlot 0.1 REQUIRED)

add_executable(consumer_app main.cpp)
target_link_libraries(consumer_app PRIVATE QGraphPlot::Core QGraphPlot::Widget)
