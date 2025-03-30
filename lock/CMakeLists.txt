project(lock)

file(GLOB SOURCES ".cpp")
foreach(source_file ${SOURCES})
  get_filename_component(exec_name ${source_file} NAME_WE)
  add_executable(${exec_name} ${source_file})
endforeach()
