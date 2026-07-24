cmake_minimum_required(VERSION 3.8)
project(navforge_config)

find_package(ament_cmake REQUIRED)

# maps/ holds local user data (gitignored); ensure the directory exists at install time.
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/maps)

install(DIRECTORY
  profiles
  modes
  params
  maps
  rviz
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
