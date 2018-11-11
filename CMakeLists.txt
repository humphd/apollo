cmake_minimum_required(VERSION 3.0)

set (CURL_LIBRARY "-lcurl")
set (CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++14")

set (PROJECT_DEPENDENCIES "${CMAKE_SOURCE_DIR}/dependencies")

find_package(CURL REQUIRED)

add_executable (apollo src/main.cpp src/curl_obj.cpp)

include_directories (${CURL_INCLUDE_DIR} "${CMAKE_SOURCE_DIR}/Dependencies/libpqxx/include" "${PROJECT_DEPENDENCIES}/libpq/include" "${PROJECT_DEPENDENCIES}/openssl/include")

target_link_libraries (apollo ${CURL_LIBRARIES} "${PROJECT_DEPENDENCIES}/libpqxx/lib/libpqxx.a" "${PROJECT_DEPENDENCIES}/libpq/lib/libpq.a" "${PROJECT_DEPENDENCIES}/openssl/lib/libssl.a" "${PROJECT_DEPENDENCIES}/openssl/lib/libcrypto.a")