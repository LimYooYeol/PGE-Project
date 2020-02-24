
# PGE File Library

add_library(PGE_FileFormats INTERFACE)
add_library(PGE_FileFormatsQt INTERFACE)

if(POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif()

# Build if Qt support enabled
set(PGEFL_QT_SUPPORT ${PGE_ENABLE_QT})

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/PGE_File_Formats)

add_dependencies(PGE_FileFormats pgefl)
add_dependencies(PGE_FileFormatsQt pgefl_qt)

target_link_libraries(PGE_FileFormats INTERFACE pgefl)
target_link_libraries(PGE_FileFormatsQt INTERFACE pgefl_qt)
