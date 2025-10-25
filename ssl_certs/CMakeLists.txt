
add_library(ssl_cert root_cert.h root_cert.cpp)

target_include_directories(ssl_cert INTERFACE .)
target_link_libraries(ssl_cert PRIVATE ssl crypto)
