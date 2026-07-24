cmake_minimum_required(VERSION 3.8)
project(navforge_bringup)

find_package(ament_cmake REQUIRED)

install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}
)

install(PROGRAMS
  launch/config_utils.py
  DESTINATION share/${PROJECT_NAME}/launch
)

ament_package()
