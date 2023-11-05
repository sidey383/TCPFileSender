cmake_minimum_required(VERSION 3.26)
project(TCPFile)

set(CMAKE_CXX_STANDARD 17)

add_executable(tcpServer src/tcpFileServer.cpp tcp/tcp.cpp tcp/tcpHolder.cpp tcp/tcpServer.cpp)

add_executable(tcpClient src/tcpFileClient.cpp tcp/tcp.cpp tcp/tcpHolder.cpp tcp/tcpClient.cpp)