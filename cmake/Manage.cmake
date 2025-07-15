set(CPM_VERSION "0.42.0")

set(CPM_CMAKE_DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake")
if(NOT EXISTS "${CPM_CMAKE_DESTINATION}")
  file(
    DOWNLOAD
      "https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_VERSION}/CPM.cmake"
      "${CPM_CMAKE_DESTINATION}"
  )
endif()
include("${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake")
