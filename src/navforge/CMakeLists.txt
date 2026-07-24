cmake_minimum_required(VERSION 3.8)
project(navforge)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)

include_directories(include)

add_executable(topic_relay src/interface/topic_relay.cpp)
ament_target_dependencies(topic_relay
  rclcpp nav_msgs geometry_msgs tf2 tf2_ros tf2_geometry_msgs)

add_executable(static_tf_publisher src/tf/static_tf_publisher.cpp)
ament_target_dependencies(static_tf_publisher
  rclcpp geometry_msgs tf2 tf2_ros)

install(TARGETS topic_relay static_tf_publisher
  DESTINATION lib/${PROJECT_NAME})

ament_package()
