cmake_minimum_required(VERSION 3.8)
project(nav_kit)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)

add_executable(imu_integrator src/processors/imu_integrator.cpp)
ament_target_dependencies(imu_integrator
  rclcpp nav_msgs sensor_msgs geometry_msgs tf2 tf2_geometry_msgs)

add_executable(odom_hub src/odom_hub/odom_hub.cpp)
ament_target_dependencies(odom_hub
  rclcpp nav_msgs geometry_msgs tf2 tf2_ros tf2_geometry_msgs)

add_executable(loc_hub src/loc_hub/loc_hub.cpp)
ament_target_dependencies(loc_hub
  rclcpp nav_msgs geometry_msgs tf2 tf2_ros tf2_geometry_msgs)

install(TARGETS imu_integrator odom_hub loc_hub
  DESTINATION lib/${PROJECT_NAME})

ament_package()
