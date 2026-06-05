cmake_minimum_required(VERSION 3.23)

foreach(required_name IN ITEMS
        PAR_BEATDOWN_TOOL
        PAR_BEATDOWN_SOURCE_DIR
        PAR_BEATDOWN_INPUT
        PAR_BEATDOWN_EXPECTED
        PAR_BEATDOWN_OUTPUT)
    if(NOT DEFINED ${required_name})
        message(FATAL_ERROR "missing ${required_name}")
    endif()
endforeach()

set(command_arguments "${PAR_BEATDOWN_INPUT}" -o "${PAR_BEATDOWN_OUTPUT}")
if(DEFINED PAR_BEATDOWN_INCLUDE)
    foreach(include_name IN LISTS PAR_BEATDOWN_INCLUDE)
        list(APPEND command_arguments --include "${include_name}")
    endforeach()
endif()

execute_process(
    COMMAND "${PAR_BEATDOWN_TOOL}" ${command_arguments}
    WORKING_DIRECTORY "${PAR_BEATDOWN_SOURCE_DIR}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr)

if(NOT result EQUAL 0)
    message(FATAL_ERROR
        "par-beatdown failed: ${result}\nstdout:\n${stdout}\nstderr:\n${stderr}")
endif()

file(READ "${PAR_BEATDOWN_EXPECTED}" expected)
file(READ "${PAR_BEATDOWN_OUTPUT}" actual)

if(NOT expected STREQUAL actual)
    message(FATAL_ERROR
        "tracker timeline output differs from gold JSON\n"
        "expected: ${PAR_BEATDOWN_EXPECTED}\n"
        "actual: ${PAR_BEATDOWN_OUTPUT}")
endif()
