sudo apt install libglm-dev libglfw3-dev

# Update graphics driver to enable vulkan support
# cmake gcc etc..
cwd=$(pwd)
# Create Assets folder
mkdir "Assets"
# install glslc in project folder
mkdir "compiler"
cd compiler || exit
git clone https://github.com/google/shaderc
cd shaderc || exit
python3 ./utils/git-sync-deps || exit
mkdir build
cd build || exit
cmake -DCMAKE_BUILD_TYPE=Release .. || exit
make -j8 || exit

echo "Successfully installed glslc compiler in project folder"

echo "Installing vulkan validation layers"
cd "$cwd" || exit
mkdir validation
cd validation || exit
git clone https://github.com/KhronosGroup/Vulkan-ValidationLayers
cd Vulkan-ValidationLayers  || exit
mkdir build
cd build || exit
python3 ../scripts/update_deps.py --dir ../external --arch x64 --config debug || exit
cmake -C ../external/helper.cmake -DCMAKE_BUILD_TYPE=Release .. || exit
# cmake --build . --config Debug || exit
make -j12 || exit
sudo make install || exit
echo "Successfully installed vulkan validation layers"


# Remember to download assets from home ftp server.
