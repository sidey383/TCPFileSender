cmake_minimum_required(VERSION 3.26)
project(TCPFile)

set(CMAKE_CXX_STANDARD 17)

add_executable(tcpServer src/server.cpp src/tcpFileServer.cpp src/timeUtil.cpp tcp/tcp.cpp tcp/tcpHolder.cpp tcp/tcpServer.cpp)

add_executable(tcpClient src/client.cpp src/tcpFileClient.cpp src/timeUtil.cpp tcp/tcp.cpp tcp/tcpHolder.cpp tcp/tcpClient.cpp)