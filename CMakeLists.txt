cmake_minimum_required(VERSION 3.25.1)

set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

set(PROTOC_COMPILE_FLAGS
	-Wall
	-Wextra
	-Werror
	-pedantic
	-std=c99
	)

set(PROTOC_SOURCES
	src/protoc/main.c
	src/protoc/out.c
	src/protoc/io.c
	src/protoc/parse.c
	src/protoc/token.c
	)

project(protoc)

add_executable(protoc ${PROTOC_SOURCES})
target_compile_options(protoc PUBLIC ${PROTOC_COMPILE_FLAGS})
target_include_directories(protoc PUBLIC include)

add_custom_target(proto_example
	COMMAND bin/protoc -m -o include/protocols/example src/protocols/example.proto
	DEPENDS protoc src/protocols/example.proto)

add_custom_target(proto_multimessage
	COMMAND bin/protoc -m -o include/protocols/multimessage src/protocols/multimessage.proto
	DEPENDS protoc src/protocols/multimessage.proto)

include(CTest)

add_executable(test_0 src/test/simple_serialize.c)
add_dependencies(test_0 proto_example)
target_compile_options(test_0 PUBLIC ${PROTOC_COMPILE_FLAGS})
target_include_directories(test_0 PUBLIC include)
add_test(NAME TEST0 COMMAND bin/test_0)