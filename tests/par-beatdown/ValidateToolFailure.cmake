cmake_minimum_required(VERSION 3.23)

foreach(required_name IN ITEMS
        PAR_BEATDOWN_TOOL
        PAR_BEATDOWN_SOURCE_DIR
        PAR_BEATDOWN_EXPECTED_STDERR)
    if(NOT DEFINED ${required_name})
        message(FATAL_ERROR "missing ${required_name}")
    endif()
endforeach()

set(command_arguments)
if(DEFINED PAR_BEATDOWN_ARGUMENTS)
    list(APPEND command_arguments ${PAR_BEATDOWN_ARGUMENTS})
endif()

execute_process(
    COMMAND "${PAR_BEATDOWN_TOOL}" ${command_arguments}
    WORKING_DIRECTORY "${PAR_BEATDOWN_SOURCE_DIR}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr)

if(result EQUAL 0)
    message(FATAL_ERROR "par-beatdown unexpectedly succeeded")
endif()
if(NOT stdout STREQUAL "")
    message(FATAL_ERROR "par-beatdown wrote unexpected stdout:\n${stdout}")
endif()
string(FIND "${stderr}" "${PAR_BEATDOWN_EXPECTED_STDERR}" stderr_index)
if(stderr_index EQUAL -1)
    message(FATAL_ERROR
        "par-beatdown stderr did not contain expected text\n"
        "expected: ${PAR_BEATDOWN_EXPECTED_STDERR}\n"
        "actual: ${stderr}")
endif()
string(LENGTH "${stderr}" stderr_length)
if(stderr_length GREATER 240)
    message(FATAL_ERROR
        "par-beatdown stderr is too long: ${stderr_length}\n${stderr}")
endif()
