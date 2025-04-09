# 设置项目名称
project(pool)

# 尝试查找 MySQL 库
find_package(MySQL QUIET)

if(MySQL_FOUND)
    message(STATUS "Found MySQL: ${MySQL_INCLUDE_DIRS}")
    # 收集库文件源
    set(POOL_LIB_SRCS SqlConnPool.cpp)
    # 构建库，修改库名为 PoolLib
    add_library(PoolLib ${POOL_LIB_SRCS})
    # 包含当前目录作为头文件搜索路径
    target_include_directories(PoolLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    # 包含 MySQL 头文件目录
    target_include_directories(PoolLib PUBLIC ${MySQL_INCLUDE_DIRS})
    # 链接 MySQL 库
    target_link_libraries(PoolLib PRIVATE ${MySQL_LIBRARIES})
    # 构建测试可执行文件
    add_executable(sqlconnpool_test sqlconnpooltest.cpp)
    # 测试程序链接库和 MySQL 库，修改库名为 PoolLib
    target_link_libraries(sqlconnpool_test PRIVATE PoolLib ${MySQL_LIBRARIES})
else()
    message(STATUS "MySQL not found, using direct mysqlclient link.")
    # 收集库文件源
    set(POOL_LIB_SRCS SqlConnPool.cpp)
    # 构建库，修改库名为 PoolLib
    add_library(PoolLib ${POOL_LIB_SRCS})
    # 包含当前目录作为头文件搜索路径
    target_include_directories(PoolLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    # 直接链接 mysqlclient 库
    target_link_libraries(PoolLib PRIVATE mysqlclient)
    # 构建测试可执行文件
    add_executable(sqlconnpool_test sqlconnpooltest.cpp)
    # 测试程序链接库和 mysqlclient 库，修改库名为 PoolLib
    target_link_libraries(sqlconnpool_test PRIVATE PoolLib mysqlclient)
endif()    