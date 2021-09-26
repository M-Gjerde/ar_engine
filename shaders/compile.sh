glslc ./triangle.vert -o triangle.vert.spv
glslc ./triangle.frag -o triangle.frag.spv

glslc ./imgui/ui.vert -o ./imgui/ui.vert.spv
glslc ./imgui/ui.frag -o ./imgui/ui.frag.spv

glslc ./gltfloading/mesh.vert -o ./gltfloading/mesh.vert.spv
glslc ./gltfloading/mesh.frag -o ./gltfloading/mesh.frag.spv


glslc ./filtercube.vert -o filtercube.vert.spv
glslc ./irradiancecube.frag -o irradiancecube.frag.spv
glslc ./prefilteredenvmap.frag -o prefilteredenvmap.frag.spv

glslc ./genbrdflut.vert -o genbrdflut.vert.spv
glslc ./genbrdflut.frag -o genbrdflut.frag.spv

glslc ./pbr.vert -o pbr.vert.spv
glslc ./pbr_khr.frag -o pbr_khr.frag.spv

glslc ./skybox.vert -o skybox.vert.spv
glslc ./skybox.frag -o skybox.frag.spv