cmake_minimum_required(VERSION 3.23)

foreach(required_name IN ITEMS
        BEAT_KEYS_TOOL
        BEAT_KEYS_SOURCE_DIR
        BEAT_KEYS_EXPECTED_STDERR)
    if(NOT DEFINED ${required_name})
        message(FATAL_ERROR "missing ${required_name}")
    endif()
endforeach()

set(command_arguments)
if(DEFINED BEAT_KEYS_ARGUMENTS)
    list(APPEND command_arguments ${BEAT_KEYS_ARGUMENTS})
endif()

execute_process(
    COMMAND "${BEAT_KEYS_TOOL}" ${command_arguments}
    WORKING_DIRECTORY "${BEAT_KEYS_SOURCE_DIR}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr)

if(result EQUAL 0)
    message(FATAL_ERROR "beat-keys unexpectedly succeeded")
endif()
if(NOT stdout STREQUAL "")
    message(FATAL_ERROR "beat-keys wrote unexpected stdout:\n${stdout}")
endif()
string(FIND "${stderr}" "${BEAT_KEYS_EXPECTED_STDERR}" stderr_index)
if(stderr_index EQUAL -1)
    message(FATAL_ERROR
        "beat-keys stderr did not contain expected text\n"
        "expected: ${BEAT_KEYS_EXPECTED_STDERR}\n"
        "actual: ${stderr}")
endif()
string(LENGTH "${stderr}" stderr_length)
if(stderr_length GREATER 240)
    message(FATAL_ERROR
        "beat-keys stderr is too long: ${stderr_length}\n${stderr}")
endif()
