
add_library(tinygltf STATIC 
    tinygltf/tiny_gltf_v3.c
)

target_compile_definitions(tinygltf PUBLIC TINYGLTF3_ENABLE_FS)

target_include_directories(tinygltf PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/tinygltf)