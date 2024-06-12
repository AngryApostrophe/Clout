# Building Clout from source

## Clone the repository
```bash
git clone --recurse-submodules http://www.github.com/AngryApostrophe/Clout Clout
```
In addition to Clout, this also clones all required submodules.

## Windows
Clout can be built either from a Visual Studio solution, or directly from CMake

### 1. Visual Studio
Open a command window in Clout's root folder
```bash
cd build
cmake ../CMakeLists.txt
```

This will generate the Visual Studio solution/project.  Open the generated solution in /Build.  Select Build Solution and VS will build all required submodules.

### 2.  CMake
Open the root CMakeLists.txt directly in Visual Studio or other environment of your choice.  Files will be built in /Build.

## Linux
From Clout root directory: 
```bash
cd build
cmake ../CMakeLists.txt
make
```
Clout and all additional submodules will be built.  Resulting binary will be in /Build/bin.

### Ubuntu libraries
The following are some of the libraries required to build Clout and its submodules:
* libglu1-mesa-dev
* zlib1g-dev
* Everything at https://www.glfw.org/docs/3.3/compile.html