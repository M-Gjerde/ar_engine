glslc ./triangle.vert -o triangle.vert.spv
glslc ./triangle.frag -o triangle.frag.spv

glslc ./imgui/ui.vert -o ./imgui/ui.vert.spv
glslc ./imgui/ui.frag -o ./imgui/ui.frag.spv

glslc ./gltfloading/mesh.vert -o ./gltfloading/mesh.vert.spv
glslc ./gltfloading/mesh.frag -o ./gltfloading/mesh.frag.spv
