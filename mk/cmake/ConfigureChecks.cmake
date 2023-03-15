include(CheckCXXSourceCompiles)
include(CheckTypeSize)

CHECK_TYPE_SIZE("void*" SIZEOF_VOID_P)
message(STATUS "Size of void* is ${SIZEOF_VOID_P}")

