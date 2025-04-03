add_executable(client client.cpp)

target_link_libraries(client PRIVATE Sockpp::sockpp)

include_directories(../external/jsoncons/include)
