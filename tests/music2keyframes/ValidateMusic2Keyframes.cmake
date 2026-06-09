cmake_minimum_required(VERSION 3.23)

foreach(required_name IN ITEMS
        MUSIC2KEYFRAMES_TOOL
        MUSIC2KEYFRAMES_SOURCE_DIR
        MUSIC2KEYFRAMES_BASE
        MUSIC2KEYFRAMES_TIMELINE
        MUSIC2KEYFRAMES_CONFIG
        MUSIC2KEYFRAMES_EXPECTED
        MUSIC2KEYFRAMES_OUTPUT)
    if(NOT DEFINED ${required_name})
        message(FATAL_ERROR "missing ${required_name}")
    endif()
endforeach()

execute_process(
    COMMAND "${MUSIC2KEYFRAMES_TOOL}"
        "${MUSIC2KEYFRAMES_BASE}"
        "${MUSIC2KEYFRAMES_TIMELINE}"
        "${MUSIC2KEYFRAMES_CONFIG}"
        -o "${MUSIC2KEYFRAMES_OUTPUT}"
    WORKING_DIRECTORY "${MUSIC2KEYFRAMES_SOURCE_DIR}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr)

if(NOT result EQUAL 0)
    message(FATAL_ERROR
        "music2keyframes failed: ${result}\nstdout:\n${stdout}\nstderr:\n${stderr}")
endif()

file(READ "${MUSIC2KEYFRAMES_EXPECTED}" expected)
file(READ "${MUSIC2KEYFRAMES_OUTPUT}" actual)
string(REPLACE "\r\n" "\n" expected "${expected}")
string(REPLACE "\r\n" "\n" actual "${actual}")

function(fail_field_check field message)
    message(FATAL_ERROR "field check failed: ${field}: ${message}")
endfunction()

function(json_get_value output field)
    string(JSON value ERROR_VARIABLE json_error GET "${actual}" ${ARGN})
    if(NOT json_error STREQUAL "NOTFOUND")
        fail_field_check("${field}" "${json_error}")
    endif()
    set(${output} "${value}" PARENT_SCOPE)
endfunction()

function(json_array_length output field)
    string(JSON value ERROR_VARIABLE json_error LENGTH "${actual}" ${ARGN})
    if(NOT json_error STREQUAL "NOTFOUND")
        fail_field_check("${field}" "${json_error}")
    endif()
    set(${output} "${value}" PARENT_SCOPE)
endfunction()

function(assert_json_equal field expected_value)
    json_get_value(actual_value "${field}" ${ARGN})
    if(NOT "${actual_value}" STREQUAL "${expected_value}")
        fail_field_check("${field}"
            "expected '${expected_value}', actual '${actual_value}'")
    endif()
endfunction()

function(assert_json_number_equal field expected_value)
    json_get_value(actual_value "${field}" ${ARGN})
    if(NOT actual_value EQUAL expected_value)
        fail_field_check("${field}"
            "expected ${expected_value}, actual ${actual_value}")
    endif()
endfunction()

function(assert_json_array_empty field)
    json_array_length(actual_length "${field}" ${ARGN})
    if(NOT actual_length EQUAL 0)
        fail_field_check("${field}" "expected empty array")
    endif()
endfunction()

assert_json_equal("schema" "par-beatdown.music2keyframes-overlay" schema)
assert_json_number_equal("version" 1 version)
assert_json_equal("generator.name" "music2keyframes" generator name)
assert_json_equal("generator.version" "0.1.0" generator version)
assert_json_equal("source.base_animation" "${MUSIC2KEYFRAMES_BASE}"
    source base_animation)
assert_json_equal("source.timeline" "${MUSIC2KEYFRAMES_TIMELINE}"
    source timeline)
assert_json_equal("source.adapter_config" "${MUSIC2KEYFRAMES_CONFIG}"
    source adapter_config)
assert_json_array_empty("keyframes" keyframes)
assert_json_array_empty("diagnostics.warnings" diagnostics warnings)

if(NOT expected STREQUAL actual)
    message(FATAL_ERROR
        "music2keyframes output differs from gold JSON\n"
        "expected: ${MUSIC2KEYFRAMES_EXPECTED}\n"
        "actual: ${MUSIC2KEYFRAMES_OUTPUT}")
endif()
