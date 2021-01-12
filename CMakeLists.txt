cmake_minimum_required(VERSION 3.4.1)

project(asiohpn)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++17 -g -O1")
set(CMAKE_CXX_COMPILER "g++-9")
set(CXX "g++-9")
set(CC "gcc-9")
set(THREADS_PREFER_PTHREAD_FLAG ON)
set(CMAKE_BUILD_TYPE Debug)


include(GNUInstallDirs)
include(FindProtobuf)

include_directories(
	src/include
	src
	external/dmlc
	${Boost_INCLUDE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
)

find_package(Boost 1.65.0 COMPONENTS system filesystem REQUIRED)
find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)
include_directories(${Protobuf_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS src/include/asiohpn.proto)
add_library(asiohpn_proto ${PROTO_HDRS} ${PROTO_SRCS})
target_link_libraries(asiohpn_proto INTERFACE 
${Protobuf_INCLUDE_DIR}
)


add_library( asiohpn
	src/include/util.cpp
	src/include/threading.cpp
	src/include/network.cpp
	src/api/api_common.cpp
	src/api/server.cpp
	src/api/client.cpp
)
target_link_libraries( asiohpn PUBLIC
	asiohpn_proto	
	tbb
    ${Boost_LIBRARIES}
	${PROTOBUF_LIBRARY}
)

add_executable (server.x src/server.cpp)
target_link_libraries(server.x 
    asiohpn
	asiohpn_proto
	Threads::Threads
)

add_executable (client.x src/client.cpp)
target_link_libraries(client.x 
    asiohpn
	asiohpn_proto
	Threads::Threads
)
