# CMake 最低版本要求
cmake_minimum_required(VERSION 3.10)

# 项目信息
project(myWebServer LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 启用调试符号（可选）
set(CMAKE_BUILD_TYPE Debug)

# 头文件路径
include_directories(
    ${CMAKE_SOURCE_DIR}/code/buffer
    ${CMAKE_SOURCE_DIR}/code/http
    ${CMAKE_SOURCE_DIR}/code/pool
    ${CMAKE_SOURCE_DIR}/code/webserver
    /usr/include/mysql
)

# 链接库路径
link_directories(/usr/lib/x86_64-linux-gnu/)

# 查找所有源文件
file(GLOB_RECURSE BUFFER_SRC ${CMAKE_SOURCE_DIR}/code/buffer/*.cpp)
file(GLOB_RECURSE HTTP_SRC ${CMAKE_SOURCE_DIR}/code/http/*.cpp)
file(GLOB_RECURSE POOL_SRC ${CMAKE_SOURCE_DIR}/code/pool/*.cpp)
file(GLOB_RECURSE WEBSERVER_SRC ${CMAKE_SOURCE_DIR}/code/webserver/*.cpp)

# 主程序文件
set(MAIN_SRC ${CMAKE_SOURCE_DIR}/code/main.cpp)

# 添加可执行文件
add_executable(myWebServer
    ${BUFFER_SRC}
    ${HTTP_SRC}
    ${POOL_SRC}
    ${WEBSERVER_SRC}
    ${MAIN_SRC}
)

# 链接系统库
target_link_libraries(myWebServer
    pthread       # 多线程库
    mysqlclient   # MySQL 客户端库
)

# 安装资源文件到可执行程序目录
install(DIRECTORY resources/ DESTINATION bin/resources)
