cmake_minimum_required(VERSION 3.12)
project(http LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_AUTOMOC ON)

find_package(Qt5 REQUIRED COMPONENTS Core Network)

add_library(http
	src/cachedhttp.h
	src/cachedhttp.cpp
	src/http.h
	src/http.cpp
	src/httpreply.h
	src/httprequest.h
	src/httpreply.cpp
	src/localcache.h
	src/localcache.cpp
	src/networkhttpreply.h
	src/networkhttpreply.cpp
	src/throttledhttp.h
	src/throttledhttp.cpp
)

target_link_libraries(http Qt5::Network)
