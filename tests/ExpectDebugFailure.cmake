execute_process(COMMAND "${PROGRAM}" --inject-error RESULT_VARIABLE result
    OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 20)
if("${result}" STREQUAL "0" OR NOT error MATCHES "OpenGL.*type=0x824c")
    message(FATAL_ERROR "Expected the synchronous GL error callback to terminate the process: ${result}\n${output}\n${error}")
endif()
