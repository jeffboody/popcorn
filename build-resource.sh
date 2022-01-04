export RESOURCE=$PWD/app/src/main/assets/resource.pak

echo RESOURCES
cd resource

# shaders
cd shaders
glslangValidator -V cube.vert    -o cube_vert.spv
glslangValidator -V cube.frag    -o cube_frag.spv
glslangValidator -V cockpit.frag -o cockpit_frag.spv
glslangValidator -V cockpit.vert -o cockpit_vert.spv
cd ..

# pak resources
pak -c $RESOURCE readme.txt
pak -a $RESOURCE shaders/cube_vert.spv
pak -a $RESOURCE shaders/cube_frag.spv
pak -a $RESOURCE shaders/cockpit_frag.spv
pak -a $RESOURCE shaders/cockpit_vert.spv
pak -a $RESOURCE models/cockpit.stl

# cleanup shaders
rm shaders/*.spv
cd ..

# VKUI
cd app/src/main/cpp/libvkk/vkui/resource
./build-resource.sh $RESOURCE
cd ../../../../../../..

echo CONTENTS
pak -l $RESOURCE
