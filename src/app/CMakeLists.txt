#--------------------------------------------------------------------------
# Client and server applications -- executables out.
#--------------------------------------------------------------------------

PROJECT(App)

SET(files_test_app "code/dbc_test.cpp")
SOURCE_GROUP("dbc_test" FILES ${files_test_app})

SET(files_client_app "code/dbc_client.cpp")
SOURCE_GROUP("dbc_client" FILES ${files_client_app})

SET(files_server_app "code/dbc_server.cpp")
SOURCE_GROUP("dbc_server" FILES ${files_server_app})

FIND_PACKAGE(asio CONFIG REQUIRED)
FIND_PACKAGE(Protobuf REQUIRED)
FIND_PACKAGE(Threads REQUIRED)

ADD_EXECUTABLE(dbc_test ${files_test_app})
ADD_DEPENDENCIES(dbc_test prog)
TARGET_LINK_LIBRARIES(dbc_test PRIVATE world prog proto protobuf::libprotoc protobuf::libprotobuf Threads::Threads)

#ADD_EXECUTABLE(dbc_client ${files_client_app})
#ADD_DEPENDENCIES(dbc_client prog)
#TARGET_LINK_LIBRARIES(dbc_client PRIVATE world prog proto protobuf::libprotoc protobuf::libprotobuf Threads::Threads asio::asio)

#ADD_EXECUTABLE(dbc_server ${files_server_app})
#ADD_DEPENDENCIES(dbc_server prog)
#TARGET_LINK_LIBRARIES(dbc_server PRIVATE world prog proto protobuf::libprotoc protobuf::libprotobuf Threads::Threads asio::asio)
