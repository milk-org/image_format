project(lib_image_format_project)

include_directories ("${PROJECT_SOURCE_DIR}/src")
include_directories ("${PROJECT_SOURCE_DIR}/..")
add_library(image_format SHARED image_format.c)

install(TARGETS image_format DESTINATION lib)
install(FILES image_format.h DESTINATION include)

