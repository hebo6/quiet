cd /home/hebo/IdeaProjects/quiet-project/quiet
rm -rf build

cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build build

#sudo cmake --install build

