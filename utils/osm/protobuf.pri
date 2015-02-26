# usage:
# PROTOS = a.proto b.proto
# PROTOPATH = blubb
# include(protobuf.pri)

protobuf_decl.name  = protobuf header
protobuf_decl.input = PROTOS
protobuf_decl.output  = ${QMAKE_FILE_BASE}.pb.h
protobuf_decl.commands = protoc -I=${QMAKE_FILE_PATH} --cpp_out=. ${QMAKE_FILE_IN}
protobuf_decl.variable_out = GENERATED_FILES
QMAKE_EXTRA_COMPILERS += protobuf_decl

protobuf_impl.name  = protobuf implementation
protobuf_impl.input = PROTOS
protobuf_impl.output  = ${QMAKE_FILE_BASE}.pb.cc
protobuf_impl.depends  = ${QMAKE_FILE_BASE}.pb.h
protobuf_impl.commands = $$escape_expand(\\n)
protobuf_impl.variable_out = GENERATED_SOURCES
QMAKE_EXTRA_COMPILERS += protobuf_impl
