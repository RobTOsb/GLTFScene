add_library(GLTFScene STATIC
    Animation.cpp
    Instance.cpp
    Texture.cpp
    Material.cpp
    Primitive.cpp
    Mesh.cpp
    GLTFScene.cpp)

target_link_libraries(GLTFScene PRIVATE tinygltf meshoptimizer)

target_include_directories(GLTFScene PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})