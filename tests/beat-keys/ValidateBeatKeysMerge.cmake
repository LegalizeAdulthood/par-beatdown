cmake_minimum_required(VERSION 3.23)

foreach(required_name IN ITEMS
        BEAT_KEYS_TOOL
        BEAT_KEYS_SOURCE_DIR
        BEAT_KEYS_BASE
        BEAT_KEYS_TIMELINE
        BEAT_KEYS_CONFIG
        BEAT_KEYS_EXPECTED_GENERATED_SOURCE
        BEAT_KEYS_EXPECTED_TRACK_COUNT
        BEAT_KEYS_EXPECTED
        BEAT_KEYS_OUTPUT)
    if(NOT DEFINED ${required_name})
        message(FATAL_ERROR "missing ${required_name}")
    endif()
endforeach()

execute_process(
    COMMAND "${BEAT_KEYS_TOOL}"
        "${BEAT_KEYS_BASE}"
        "${BEAT_KEYS_TIMELINE}"
        "${BEAT_KEYS_CONFIG}"
        -o "${BEAT_KEYS_OUTPUT}"
    WORKING_DIRECTORY "${BEAT_KEYS_SOURCE_DIR}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr)

if(NOT result EQUAL 0)
    message(FATAL_ERROR
        "beat-keys failed: ${result}\nstdout:\n${stdout}\nstderr:\n${stderr}")
endif()

file(READ "${BEAT_KEYS_EXPECTED}" expected)
file(READ "${BEAT_KEYS_OUTPUT}" actual)
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

function(assert_json_array_length field expected_value)
    json_array_length(actual_length "${field}" ${ARGN})
    if(NOT actual_length EQUAL expected_value)
        fail_field_check("${field}"
            "expected ${expected_value}, actual ${actual_length}")
    endif()
endfunction()

assert_json_equal("parameter-catalogs[0]" "input/core-catalog.json"
    parameter-catalogs 0)
assert_json_equal("source.file" "input/source.par" source file)
assert_json_equal("source.name" "Mandel_Demo" source name)
assert_json_equal("tracks[0].parameter" "maxiter" tracks 0 parameter)
assert_json_equal("tracks[0].keys[0].value" "100" tracks 0 keys 0 value)
assert_json_array_length("tracks" "${BEAT_KEYS_EXPECTED_TRACK_COUNT}" tracks)
assert_json_equal("tracks[1].id" "music.color.brightness" tracks 1 id)
assert_json_equal("tracks[1].source" "${BEAT_KEYS_EXPECTED_GENERATED_SOURCE}"
    tracks 1 source)
json_array_length(generated_key_count "tracks[1].keys" tracks 1 keys)
if(NOT generated_key_count GREATER 0)
    fail_field_check("tracks[1].keys" "expected generated keys")
endif()

if(NOT expected STREQUAL actual)
    message(FATAL_ERROR
        "beat-keys merge output differs from gold JSON\n"
        "expected: ${BEAT_KEYS_EXPECTED}\n"
        "actual: ${BEAT_KEYS_OUTPUT}")
endif()
