cmake_minimum_required(VERSION 4.0)
project(ion-server)
set(CMAKE_CXX_STANDARD 23)

find_package(OpenSSL REQUIRED)

add_executable(ion-server main.cpp)

target_link_libraries(ion-server PRIVATE OpenSSL::SSL OpenSSL::Crypto)