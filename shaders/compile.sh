glslc ./defaultVert.vert -o defaultVert.spv
glslc ./defaultFrag.frag -o defaultFrag.spv
glslc ./PhongLight.frag -o phongLightFrag.spv
glslc ./lamp.vert -o lampVert.spv
glslc ./lamp.frag -o lampFrag.spv

glslc ./experimental/computeShader.vert -o computeShader.spv