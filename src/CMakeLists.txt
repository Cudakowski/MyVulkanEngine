
set(SOURCES
    ../assets/shader.slang
    app.cpp
    basic_render_system.cpp
    camera_controller.cpp
    main.cpp
    mpp_buffer.cpp
    mpp_camera.cpp
    mpp_descriptors.cpp
    mpp_device.cpp
    mpp_game_object.cpp
    mpp_loader.cpp
    mpp_model.cpp
    mpp_pipeline.cpp
    mpp_renderer.cpp
    mpp_swap_chain.cpp
    mpp_texture.cpp
    mpp_window.cpp
    external_impl.cpp
    scene_input_handler.cpp
)


add_executable(${PROJECT_NAME} ${SOURCES})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_EXTENSIONS OFF)

set_target_properties(${PROJECT_NAME} PROPERTIES 
    DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} # Do Visual Studio
)


target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(${PROJECT_NAME} PRIVATE 
    VK_NO_PROTOTYPES
    NOMINMAX 
    _USE_MATH_DEFINES
)

if (WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE 
        VK_USE_PLATFORM_WIN32_KHR 
        _CRT_SECURE_NO_WARNINGS
    )
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /EHsc)
    endif()
elseif (LINUX)
    target_compile_definitions(${PROJECT_NAME} PRIVATE VK_USE_PLATFORM_WAYLAND_KHR)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE 
    ${SDL_LIBRARY} 
    ktx
    tinyobj
    ${Slang_LIBRARY}
)

if (NOT WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm-header-only)
endif()

# skopiowanie folderu assets do builda
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
)