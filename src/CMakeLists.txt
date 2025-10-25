
add_executable(data_parcer data_parcer.cpp)

target_link_libraries(data_parcer PRIVATE ssl_cert)
