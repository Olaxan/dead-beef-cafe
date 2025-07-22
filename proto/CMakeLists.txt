INCLUDE(FindProtobuf)
FIND_PACKAGE(Protobuf REQUIRED)

FILE(GLOB proto_sources "./*.proto")
SET(files_proto ${proto_sources})

message("   --> PROTOBUF IN: ${proto_sources}")

PROTOBUF_GENERATE_CPP(PROTO_SRC PROTO_HEADER ${files_proto})
ADD_LIBRARY(proto ${PROTO_HEADER} ${PROTO_SRC})
TARGET_INCLUDE_DIRECTORIES(proto PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/proto")
TARGET_INCLUDE_DIRECTORIES(proto PRIVATE "${Protobuf_INCLUDE_DIRS}")

# target_include_directories(proto PUBLIC ${CMAKE_BINARY_DIR}/proto)

message("   --> PROTOBUF LIB: ${PROTOBUF_LIBRARIES}")
message("   --> PROTOBUF INCLUDE: ${Protobuf_INCLUDE_DIRS}")
message("   --> PROTOBUF VERSION: ${Protobuf_VERSION}")
message("   --> PROTOBUF Found: ${Protobuf_FOUND}")
message("   --> PROTOBUF SRC: ${PROTO_SRC}")
message("   --> PROTOBUF HEADER: ${PROTO_HEADER}")
message("   --> PROTOBUF Folder: ${CMAKE_BINARY_DIR}/proto")

get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY TARGET_INCLUDE_DIRECTORIES)
message("  This is where the proto directories should be listed")
foreach(dir ${dirs})
  message(STATUS "proto directory is ='${dir}'")
endforeach()