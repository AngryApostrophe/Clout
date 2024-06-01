# Building Clout

## 1. Clone the repository
## 2. Build Assimp
* https://github.com/assimp/assimp/blob/master/Build.md
* From Assimp directory: 
```bash
cmake CMakeLists.txt
```
* If using Visual Studio, open Assimp.sln, ensure you're on Release config and build the ALL_BUILD project
## 3. Build Clout
* From Clout root directory:
 ```bash
 cd build
cmake ..\CMakeLists.txt
```
* Cmake will generate the required Visual Studio solution and project files
* Open \build\Clout.sln and build Clout
* Note assimp-vc143-mt.dll and glew32.dll are already placed in the project bin folder.  Ensure your working directory is set here when running.
