cmake_minimum_required(VERSION 3.26)
project(TCPFile)

set(CMAKE_CXX_STANDARD 17)

add_executable(tcpServer src/tcpFileServer.cpp tcp/tcpServer.cpp tcp/tcpError.cpp tcp/tcpHolder.cpp)

add_executable(tcpClient src/tcpFileClient.cpp tcp/tcpClient.cpp tcp/tcpError.cpp tcp/tcpHolder.cpp)