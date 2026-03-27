#--------------------------------------------------------------------------
# Client and server applications -- executables out.
#--------------------------------------------------------------------------

PROJECT(App)

SET(files_test_app "code/dbc.h" "code/dbc.cpp" "code/dbc_win.h" "code/dbc_win.cpp" "code/dbc_test.cpp")
SOURCE_GROUP("dbc_test" FILES ${files_test_app})

SET(files_client_app "code/dbc.h" "code/dbc.cpp" "code/dbc_win.h" "code/dbc_win.cpp" "code/dbc_client_program.cpp" "code/dbc_server_program.cpp" "code/dbc_client.cpp")
SOURCE_GROUP("dbc_client" FILES ${files_client_app})

SET(files_server_app "code/dbc.h" "code/dbc.cpp" "code/dbc_win.h" "code/dbc_win.cpp" "code/dbc_client_program.cpp" "code/dbc_server_program.cpp" "code/dbc_server.cpp")
SOURCE_GROUP("dbc_server" FILES ${files_server_app})

FIND_PACKAGE(asio CONFIG REQUIRED)
FIND_PACKAGE(Protobuf REQUIRED)
FIND_PACKAGE(Threads REQUIRED)

ADD_EXECUTABLE(dbc_test ${files_test_app})
ADD_DEPENDENCIES(dbc_test programs)
TARGET_LINK_LIBRARIES(dbc_test PRIVATE world programs common)

ADD_EXECUTABLE(dbc_client ${files_client_app})
ADD_DEPENDENCIES(dbc_client programs)
TARGET_LINK_LIBRARIES(dbc_client PRIVATE world programs proto common asio::asio)

ADD_EXECUTABLE(dbc_server ${files_server_app})
ADD_DEPENDENCIES(dbc_server programs)
TARGET_LINK_LIBRARIES(dbc_server PRIVATE world programs proto common asio::asio)

ADD_CUSTOM_COMMAND(TARGET dbc_client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:dbc_client>/assets
)