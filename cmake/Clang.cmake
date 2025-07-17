if(NOT "${IS_SUBDIRECTORY}")
  set(FORMAT_MARKER_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/FormatMarkers")
  file(
    GLOB_RECURSE FORMAT_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/${PROGRAM_DIRECTORY}/*.*"
    "${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_SOURCE_DIRECTORY}/*.*"
  )
  file(MAKE_DIRECTORY "${FORMAT_MARKER_DIRECTORY}")
  foreach(FILE ${FORMAT_FILES})
    get_filename_component(FILE_NAME "${FILE}" NAME)
    set(FORMAT_MARKER "${FORMAT_MARKER_DIRECTORY}/${FILE_NAME}.format")
    add_custom_command(
      OUTPUT "${FORMAT_MARKER}"
      COMMAND clang-format -i "${FILE}"
      COMMAND "${CMAKE_COMMAND}" -E touch "${FORMAT_MARKER}"
      DEPENDS "${FILE}"
    )
    list(APPEND FORMAT_MARKERS "${FORMAT_MARKER}")
  endforeach()
  add_custom_target("Format" DEPENDS "${FORMAT_MARKERS}")
  list(APPEND DEPENDENCIES "Format")

  set(TEMPORARY_COMPILE_COMMANDS_FILE "${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json")
  set(COMPILE_COMMANDS_FILE "${CMAKE_CURRENT_SOURCE_DIR}/compile_commands.json")
  set(COMPILE_COMMANDS_CONTENT "[\n")
  foreach(FILE ${SOURCE_FILES})
    list(LENGTH SOURCE_FILES SOURCE_FILE_COUNT)
    get_filename_component(FILE_EXTENSION "${FILE}" EXT)
    string(FIND "${FILE}" "${PROGRAM_SOURCE_DIRECTORY}" POS)
    if(NOT "${POS}" EQUAL "-1")
      set(IS_EXTERNAL OFF)
    else()
      set(IS_EXTERNAL ON)
    endif()
    if("${FILE_EXTENSION}" MATCHES "cpp")
      set(
        COMPILE_COMMANDS_CONTENT
        "${COMPILE_COMMANDS_CONTENT}  {\n    \"directory\": \"${CMAKE_CURRENT_SOURCE_DIR}\",\n    \"file\": \"${FILE}\",\n    \"command\": \"clang++ -std=c++${CMAKE_CXX_STANDARD} "
      )
    else()
      set(
        COMPILE_COMMANDS_CONTENT
        "${COMPILE_COMMANDS_CONTENT}  {\n    \"directory\": \"${CMAKE_CURRENT_SOURCE_DIR}\",\n    \"file\": \"${FILE}\",\n    \"command\": \"clang "
      )
    endif()
    if(NOT ${IS_EXTERNAL})
      set(
        COMPILE_COMMANDS_CONTENT
        "${COMPILE_COMMANDS_CONTENT}-Wall -Wextra -Wpedantic -Wconversion -Wshadow-all -Wundef -Wdeprecated -Wtype-limits -Wcast-qual -Wcast-align -Wfloat-equal -Wparentheses -Wunreachable-code-aggressive -Wformat=2 "
      )
    endif()
    foreach(DIRECTORY ${INCLUDE_DIRECTORIES})
      set(COMPILE_COMMANDS_CONTENT "${COMPILE_COMMANDS_CONTENT}-I\\\"${DIRECTORY}\\\" ")
    endforeach()
    foreach(DIRECTORY ${SYSTEM_INCLUDE_DIRECTORIES})
      set(COMPILE_COMMANDS_CONTENT "${COMPILE_COMMANDS_CONTENT}-isystem\\\"${DIRECTORY}\\\" ")
    endforeach()
    foreach(DEFINITION ${COMPILER_DEFINITIONS})
      set(COMPILE_COMMANDS_CONTENT "${COMPILE_COMMANDS_CONTENT}-D${DEFINITION} ")
    endforeach()
    set(COMPILE_COMMANDS_CONTENT "${COMPILE_COMMANDS_CONTENT}-c \\\"${FILE}\\\"\"\n  }")
    math(EXPR COMPILE_COMMANDS_INDEX "${COMPILE_COMMANDS_INDEX} + 1")
    if(NOT "${COMPILE_COMMANDS_INDEX}" EQUAL "${SOURCE_FILE_COUNT}")
      set(COMPILE_COMMANDS_CONTENT "${COMPILE_COMMANDS_CONTENT},\n")
    else()
      set(COMPILE_COMMANDS_CONTENT "${COMPILE_COMMANDS_CONTENT}\n]")
    endif()
  endforeach()
  file(WRITE "${TEMPORARY_COMPILE_COMMANDS_FILE}" "${COMPILE_COMMANDS_CONTENT}")
  add_custom_command(
    OUTPUT "${COMPILE_COMMANDS_FILE}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${TEMPORARY_COMPILE_COMMANDS_FILE}" "${COMPILE_COMMANDS_FILE}"
    DEPENDS "${TEMPORARY_COMPILE_COMMANDS_FILE}"
  )
  add_custom_target("GenerateCompileCommands" ALL DEPENDS "${COMPILE_COMMANDS_FILE}")
  list(APPEND DEPENDENCIES "GenerateCompileCommands")
endif()
