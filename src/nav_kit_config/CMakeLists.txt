cmake_minimum_required(VERSION 3.8)
project(nav_kit_config)

find_package(ament_cmake REQUIRED)

install(DIRECTORY
  profiles
  modes
  params
  robots
  maps
  rviz
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
