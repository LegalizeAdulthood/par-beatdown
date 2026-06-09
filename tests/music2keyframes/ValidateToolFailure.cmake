cmake_minimum_required(VERSION 3.23)

foreach(required_name IN ITEMS
        MUSIC2KEYFRAMES_TOOL
        MUSIC2KEYFRAMES_SOURCE_DIR
        MUSIC2KEYFRAMES_EXPECTED_STDERR)
    if(NOT DEFINED ${required_name})
        message(FATAL_ERROR "missing ${required_name}")
    endif()
endforeach()

set(command_arguments)
if(DEFINED MUSIC2KEYFRAMES_ARGUMENTS)
    list(APPEND command_arguments ${MUSIC2KEYFRAMES_ARGUMENTS})
endif()

execute_process(
    COMMAND "${MUSIC2KEYFRAMES_TOOL}" ${command_arguments}
    WORKING_DIRECTORY "${MUSIC2KEYFRAMES_SOURCE_DIR}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr)

if(result EQUAL 0)
    message(FATAL_ERROR "music2keyframes unexpectedly succeeded")
endif()
if(NOT stdout STREQUAL "")
    message(FATAL_ERROR "music2keyframes wrote unexpected stdout:\n${stdout}")
endif()
string(FIND "${stderr}" "${MUSIC2KEYFRAMES_EXPECTED_STDERR}" stderr_index)
if(stderr_index EQUAL -1)
    message(FATAL_ERROR
        "music2keyframes stderr did not contain expected text\n"
        "expected: ${MUSIC2KEYFRAMES_EXPECTED_STDERR}\n"
        "actual: ${stderr}")
endif()
string(LENGTH "${stderr}" stderr_length)
if(stderr_length GREATER 240)
    message(FATAL_ERROR
        "music2keyframes stderr is too long: ${stderr_length}\n${stderr}")
endif()
