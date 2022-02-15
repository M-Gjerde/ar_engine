glslc="../compiler/shaderc/build/glslc/glslc"

$glslc ./imgui/ui.vert -o ./imgui/ui.vert.spv
$glslc ./imgui/ui.frag -o ./imgui/ui.frag.spv

$glslc ./myScene/mesh.vert -o ./myScene/mesh.vert.spv
$glslc ./myScene/mesh.frag -o ./myScene/mesh.frag.spv
$glslc ./myScene/box.vert -o ./myScene/box.vert.spv
$glslc ./myScene/box.frag -o ./myScene/box.frag.spv
$glslc ./myScene/sphere.vert -o ./myScene/sphere.vert.spv
$glslc ./myScene/sphere.frag -o ./myScene/sphere.frag.spv