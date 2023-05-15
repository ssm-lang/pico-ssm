# Yes, globbing is terrible practice in CMake but I don't care.
file(GLOB program_dirs)

foreach(dir ${program_dirs})
    add_subdirectory(${dir})
endforeach()
