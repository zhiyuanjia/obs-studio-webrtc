export CMAKE_PREFIX_PATH=/usr/local/Cellar/qt/5.10.1/lib/cmake
export Qt5_DIR=/usr/local/Cellar/qt/5.10.1/
sudo rm -rf build
./CI/before-script-osx.sh
cd build && sudo make -j 8 && cd ..
sudo ./CI/before-deploy-osx.sh
