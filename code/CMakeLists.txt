# 设置项目名称
project(code)

# 添加可执行文件
add_executable(MyTinyWebServer main.cpp)

# 添加子目录
add_subdirectory(buffer)
add_subdirectory(http)
add_subdirectory(log)
add_subdirectory(pool)
add_subdirectory(server)
add_subdirectory(timer)

# 链接库（确保库名正确，如 PoolLib 而非之前的 sqlconnpool）
target_link_libraries(
  MyTinyWebServer
  BufferLib
  HttpLib
  LogLib
  PoolLib # 确认库名与 pool/CMakeLists.txt 中的 add_library 一致
  ServerLib
  TimerLib)
