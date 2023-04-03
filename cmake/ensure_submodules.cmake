# Update git submodules if user forgot to
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
    # Update submodules as needed
    option(GIT_SUBMODULE "Check submodules during build" ON)
    if(GIT_SUBMODULE)
        message(STATUS "Submodule update")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init
                        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                        RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
        endif()

        # We don't want to use --recursive because pico-sdk contains tinyusb,
        # which contains quite a few submodules that aren't very useful.
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init
                        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/lib/pico-sdk
                        RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init in pico-sdk failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
        endif()
    endif()
endif()

if(NOT EXISTS "${CMAKE_SOURCE_DIR}/lib/pico-sdk/CMakeLists.txt")
    message(FATAL_ERROR "pico-sdk was not downloaded! ${CMAKE_SOURCE_DIR}/lib/pico-sdk/CMakeLists.txt GIT_SUBMODULE was turned off or failed. Please update submodules and try again.")
endif()

if(NOT EXISTS "${CMAKE_SOURCE_DIR}/lib/ssm-runtime/Makefile")
    message(FATAL_ERROR "ssm-runtime was not downloaded! GIT_SUBMODULE was turned off or failed. Please update submodules and try again.")
endif()
