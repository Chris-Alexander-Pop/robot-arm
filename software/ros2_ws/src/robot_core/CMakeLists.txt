cmake_minimum_required(VERSION 3.8)
project(robot_core)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)

include_directories(include)

add_library(robot_core_domain
  src/domain/kinematics_solver.cpp
  src/domain/trajectory_planner.cpp
)

target_include_directories(robot_core_domain PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

add_executable(hardware_bridge_node src/nodes/hardware_bridge_node.cpp)
ament_target_dependencies(hardware_bridge_node rclcpp)
target_link_libraries(hardware_bridge_node robot_core_domain)

add_executable(planning_node src/nodes/planning_node.cpp)
ament_target_dependencies(planning_node rclcpp)
target_link_libraries(planning_node robot_core_domain)

install(TARGETS
  robot_core_domain
  hardware_bridge_node
  planning_node
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY include/
  DESTINATION include/
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
