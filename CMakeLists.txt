cmake_minimum_required(VERSION 3.17)
project(encrypt_ccm)
set(CMAKE_CXX_STANDARD 14)

add_compile_options(
        -O3
)

add_library(
        base64-lib
        STATIC
        libs/base64.cpp
)

add_library(
        json-lib
        STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/SimpleJSON/src/JSON.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/SimpleJSON/src/JSONValue.cpp
)

add_library(
        openssl-lib
        STATIC
        IMPORTED
)
set_target_properties(
        openssl-lib
        PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/libs/openssl/libssl.a
)

add_library(
        opensslcrypto-lib
        STATIC
        IMPORTED
)
set_target_properties(
        opensslcrypto-lib
        PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/libs/openssl/libcrypto.a
)

target_link_libraries(
        base64-lib
        json-lib

        openssl-lib
        opensslcrypto-lib
 )

link_libraries(ssl crypto base64-lib)

add_executable(encrypt_ccm main.cpp)
