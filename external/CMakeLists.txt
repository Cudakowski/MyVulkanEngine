include(FetchContent)


if (NOT WIN32) 
    FetchContent_Declare(GLM
        GIT_REPOSITORY https://github.com/g-truc/glm
        GIT_TAG 1.0 # Można zaktualizować do nowszej wersji
        GIT_SHALLOW ON
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(GLM)
endif()

# Kompilacja biblioteki KTX
set(KTX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ktx)
set(KTX_SOURCES
    ${KTX_DIR}/lib/texture.c
    ${KTX_DIR}/lib/hashlist.c
    ${KTX_DIR}/lib/checkheader.c
    ${KTX_DIR}/lib/swap.c
    ${KTX_DIR}/lib/memstream.c
    ${KTX_DIR}/lib/filestream.c
    ${KTX_DIR}/lib/vkloader.c
)
add_library(ktx STATIC ${KTX_SOURCES})

target_include_directories(ktx PUBLIC 
    ${KTX_DIR}/include
    ${KTX_DIR}/other_include
)

if (TARGET Vulkan::Vulkan)
    target_link_libraries(ktx PUBLIC Vulkan::Vulkan)
elseif (APPLE)
    target_link_libraries(ktx PUBLIC $ENV{VULKAN_SDK}/lib/libvulkan.dylib)
elseif (NOT WIN32)
    target_link_libraries(ktx PUBLIC $ENV{VULKAN_SDK}/lib/libvulkan.so)
endif()


add_library(tinyobj INTERFACE)
target_include_directories(tinyobj INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/tinyobj)


set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui)

add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)

target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

target_compile_features(imgui PUBLIC cxx_std_20)

target_link_libraries(imgui PUBLIC
    Vulkan::Vulkan
    SDL3::SDL3  
)
target_compile_definitions(imgui PUBLIC
    VK_NO_PROTOTYPES
    IM_VK_USE_VOLK
)