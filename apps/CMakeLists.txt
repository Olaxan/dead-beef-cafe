#--------------------------------------------------------------------------
# Client and server applications -- executables out.
#--------------------------------------------------------------------------

PROJECT(App)

SET(files_test_app "code/dbc.h" "code/dbc.cpp" "code/dbc_win.h" "code/dbc_win.cpp" "code/dbc_test.cpp")
SOURCE_GROUP("dbc_test" FILES ${files_test_app})

SET(files_client_app "code/dbc.h" "code/dbc.cpp" "code/dbc_win.h" "code/dbc_win.cpp" "code/dbc_client.cpp")
SOURCE_GROUP("dbc_client" FILES ${files_client_app})

SET(files_server_app "code/dbc.h" "code/dbc.cpp" "code/dbc_win.h" "code/dbc_win.cpp" "code/dbc_server.cpp")
SOURCE_GROUP("dbc_server" FILES ${files_server_app})


FIND_PACKAGE(Protobuf REQUIRED)
FIND_PACKAGE(Threads REQUIRED)

ADD_EXECUTABLE(dbc_test ${files_test_app})
TARGET_LINK_LIBRARIES(dbc_test PRIVATE world basicos programs common)

ADD_EXECUTABLE(dbc_client ${files_client_app})
TARGET_LINK_LIBRARIES(dbc_client PRIVATE world basicos programs proto common)

ADD_EXECUTABLE(dbc_server ${files_server_app})
TARGET_LINK_LIBRARIES(dbc_server PRIVATE world basicos programs proto common)

ADD_CUSTOM_COMMAND(TARGET dbc_client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:dbc_client>/assets
)

ADD_CUSTOM_COMMAND(TARGET dbc_client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/lib
    $<TARGET_FILE_DIR:dbc_client>
)