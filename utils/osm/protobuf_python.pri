# usage:
# PROTOS = a.proto b.proto
# PROTOPATH = blubb
# include(protobuf_python.pri)

protobuf_python.name  = protobuf python
protobuf_python.input = PROTOS
protobuf_python.output  = ${QMAKE_FILE_BASE}_pb2.py
protobuf_python.commands = protoc -I=${QMAKE_FILE_PATH} --python_out=. ${QMAKE_FILE_IN}
protobuf_python.variable_out = GENERATED_FILES
QMAKE_EXTRA_COMPILERS += protobuf_python
