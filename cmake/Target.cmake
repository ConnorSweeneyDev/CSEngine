add_executable("${NAME}")
if(DEFINED DEPENDENCIES)
  add_dependencies("${NAME}" ${DEPENDENCIES})
endif()

target_compile_options("${NAME}" PRIVATE "${COMPILER_FLAGS}")
target_compile_definitions("${NAME}" PRIVATE "${COMPILER_DEFINITIONS}")
target_include_directories("${NAME}" PRIVATE "${INCLUDE_DIRECTORIES}")
target_include_directories("${NAME}" SYSTEM PRIVATE "${SYSTEM_INCLUDE_DIRECTORIES}")
target_sources("${NAME}" PRIVATE "${HEADER_FILES}" "${SOURCE_FILES}")
target_link_options("${NAME}" PRIVATE "${LINKER_FLAGS}")
target_link_directories("${NAME}" PRIVATE "${LIBRARY_DIRECTORIES}")
target_link_libraries("${NAME}" PRIVATE "${LIBRARIES}")
