cd /home/ubuntu/quiet
rm -rf build
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make

cp /home/ubuntu/quiet/quiet-profiles.json /home/ubuntu/quiet/build/bin/quiet-profiles.json
echo "hello" > /home/ubuntu/quiet/build/bin/hello.txt
