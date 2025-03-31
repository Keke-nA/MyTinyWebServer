project(lock)

# 查找线程库（pthread）
find_package(Threads REQUIRED)

file(GLOB SOURCES "*.cpp")

foreach(source_file ${SOURCES})
  # 提取不带扩展名的文件名（如 "main.cpp" -> "main"）
  get_filename_component(exec_name ${source_file} NAME_WE)
  # 创建可执行文件
  add_executable(${exec_name} ${source_file})
  # 链接 pthread 库（推荐方式）
  target_link_libraries(${exec_name} PRIVATE Threads::Threads)
endforeach()
