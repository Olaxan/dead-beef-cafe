#--------------------------------------------------------------------------
# Client and server applications -- executables out.
#--------------------------------------------------------------------------

PROJECT(App)

SET(files_test_app "code/dbc_test.cpp")
SOURCE_GROUP("test_app" FILES ${files_test_app})

SET(files_client_app "code/dbc_client.cpp")
SOURCE_GROUP("client_app" FILES ${files_client_app})

SET(files_server_app "code/dbc_server.cpp")
SOURCE_GROUP("server_app" FILES ${files_server_app})

FIND_PACKAGE(asio CONFIG REQUIRED)

ADD_EXECUTABLE(test_app ${files_test_app})
TARGET_LINK_LIBRARIES(test_app PRIVATE world prog)

ADD_EXECUTABLE(client_app ${files_client_app})
TARGET_LINK_LIBRARIES(client_app PRIVATE asio::asio)

ADD_EXECUTABLE(server_app ${files_server_app})
TARGET_LINK_LIBRARIES(server_app PRIVATE asio::asio)

#ADD_DEPENDENCIES(server_app server)