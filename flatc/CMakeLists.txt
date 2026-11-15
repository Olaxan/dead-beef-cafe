FIND_PACKAGE(FlatBuffers CONFIG REQUIRED)

FILE(GLOB FLATBUFFER_SCHEMAS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.fbs")
SET(FLATBUFFER_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
SET(FLATBUFFER_GENERATED_HEADERS)

FOREACH(schema IN LISTS FLATBUFFER_SCHEMAS)
	GET_FILENAME_COMPONENT(schema_name "${schema}" NAME_WE)
	SET(generated_header "${FLATBUFFER_GENERATED_DIR}/${schema_name}_generated.h")
	SET(embedded_schema_header "${FLATBUFFER_GENERATED_DIR}/${schema_name}_bfbs_generated.h")

	ADD_CUSTOM_COMMAND(
		OUTPUT "${generated_header}" "${embedded_schema_header}"
		COMMAND "${CMAKE_COMMAND}" -E make_directory "${FLATBUFFER_GENERATED_DIR}"
		COMMAND flatbuffers::flatc --cpp --bfbs-gen-embed -I "${CMAKE_CURRENT_SOURCE_DIR}" -o "${FLATBUFFER_GENERATED_DIR}" "${schema}"
		DEPENDS ${FLATBUFFER_SCHEMAS} flatbuffers::flatc
		COMMENT "Generating FlatBuffers header for ${schema_name}"
		VERBATIM
	)
	LIST(APPEND FLATBUFFER_GENERATED_HEADERS "${generated_header}")
	LIST(APPEND FLATBUFFER_GENERATED_HEADERS "${embedded_schema_header}")
ENDFOREACH()

ADD_CUSTOM_TARGET(generate_flatbuffers_schemas DEPENDS ${FLATBUFFER_GENERATED_HEADERS})

ADD_LIBRARY(flatbuffers_schemas INTERFACE)
TARGET_INCLUDE_DIRECTORIES(flatbuffers_schemas INTERFACE "${FLATBUFFER_GENERATED_DIR}")
TARGET_LINK_LIBRARIES(flatbuffers_schemas INTERFACE flatbuffers::flatbuffers)
ADD_DEPENDENCIES(flatbuffers_schemas generate_flatbuffers_schemas)
