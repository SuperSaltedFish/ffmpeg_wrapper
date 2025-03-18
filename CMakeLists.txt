cmake_minimum_required(VERSION 3.30)
project(ffmpeg_wrapper C)

set(CMAKE_C_STANDARD 11)

add_executable(ffmpeg_wrapper
        main.c
        src/av_encoder.c
        src/av_device_input.c
        src/av_output.c
        src/av_decoder.c
        src/av_filter.c)

set_target_properties(ffmpeg_wrapper PROPERTIES INSTALL_RPATH "./" BUILD_WITH_INSTALL_RPATH TRUE)
target_include_directories(ffmpeg_wrapper PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib/include)

if (UNIX)
    if (APPLE)
        if (CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
            file(GLOB_RECURSE PROJECT_LIBRARIES "${CMAKE_CURRENT_SOURCE_DIR}/lib/macosx_arm64/*.dylib")
        endif ()
    else ()
        if (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
            file(GLOB_RECURSE PROJECT_LIBRARIES "${CMAKE_CURRENT_SOURCE_DIR}/lib/linux_x86_64/*.so")
        endif ()
    endif ()
elseif (WIN32)

endif ()


target_link_libraries(ffmpeg_wrapper ${PROJECT_LIBRARIES})


foreach (LIBRARY ${PROJECT_LIBRARIES})
    get_filename_component(LIBRARY_NAME ${LIBRARY} NAME)
    add_custom_command(TARGET ffmpeg_wrapper POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${LIBRARY} $<TARGET_FILE_DIR:ffmpeg_wrapper>/${INSTALL_RPATH}${LIBRARY_NAME}
            COMMENT "Copying ${LIBRARY_NAME} to build directory"
    )
endforeach ()
