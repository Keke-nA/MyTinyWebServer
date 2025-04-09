# 设置项目名称
project(http)

# 添加库
add_library(HttpLib HttpConn.cpp HttpRequest.cpp HttpResponse.cpp)

# 包含头文件目录
target_include_directories(HttpLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(HttpLib PUBLIC BufferLib)