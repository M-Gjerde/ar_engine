glslc ./defaultVert.vert -o defaultVert.spv
glslc ./defaultFrag.frag -o defaultFrag.spv
glslc ./PhongLight.frag -o phongLightFrag.spv
glslc ./lamp.vert -o lampVert.spv
glslc ./lamp.frag -o lampFrag.spv

glslc ./experimental/computeShader.comp -o ./experimental/computeShader.spv
glslc ./experimental/copyShader.comp -o ./experimental/copyShader.spv