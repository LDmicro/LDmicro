# LDmicro 
LDmicro is a program for creating, developing and editing [ladder diagrams](https://en.wikipedia.org/wiki/Ladder_logic),
simulation of a ladder diagram work
and the compilation of ladder diagrams into the native hexadecimal firmware code of the Atmel AVR and Microchip PIC controllers.

Official [LDmicro Ladder Logic for PIC and AVR Home Page](http://cq.cx/ladder.pl)  
Official [LDmicro Forum](http://cq.cx/ladder-forum.pl)  

Download executable binaries buildXXXX.zip from [Latest release](https://github.com/LDmicro/LDmicro/releases/latest)  
Unofficial [HOW TO:](https://github.com/LDmicro/LDmicro/wiki/HOW-TO)  

### Building LDmicro

#### Building with Cmake

For building LDmicro with Cmake you need Cmake itself, Perl interpreter and C++11 compiler.

You should use out-of-source-tree builds, so create e.g. a directory
`build` in the main ldmicro directory. (If you choose to build in other
directory, replace the `..` in the following instructions with path pointing
to the root/ldmicro directory.)

To build with MSYS + [mingw-w64](http://mingw-w64.org) + Make:
```sh
$ mkdir build
$ cd build
$ cmake -G "MSYS Makefiles" ..
$ make
```

To build with MSYS + mingw-w64 + [Ninja](http://martine.github.io/ninja):
```sh
$ mkdir build
$ cd build
$ cmake -G "Ninja" ..
$ ninja
```

To build within [MSYS2](http://msys2.github.io), make sure you have these
MSYS2 packages installed:
* `make`
* `mingw-w64-i686-gcc`, `mingw-w64-i686-cmake`

### Build with Microsoft Visual Studio 2017

Visual Studio 2017 supports CMake build system, so you may just follow these
instructions.
1. Start Visual Studio 2017.
2. In menu File, choose submenu Open and Folder.
3. In the open dialog, navigate to LDmicro/ldmicro folder and open it.
4. In menu CMake, choose to Build all.

### Build with Older Version of Microsoft Visual Studio

To build with older Microsoft Visual Studio 2013 or 2015, you have to
generate project files manually:
```sh
$ mkdir build
$ cd build
$ cmake -G "Visual Studio 12 2013" ..           # MSVC 2013, 32-bit build
$ cmake -G "Visual Studio 12 2013 Win64" ..     # MSVC 2013, 64-bit build
$ cmake -G "Visual Studio 14 2015" ..           # MSVC 2015, 32-bit build
$ cmake -G "Visual Studio 14 2015 Win64" ..     # MSVC 2015, 64-bit build
```
Then open the generated solution file `build/ldmicro.sln` in Visual Studio and
build the target `ALL_BUILD`.

You can also choose LDmicro language. For that, you should set LDLANG variable for Cmake.
LDLANG can be one of those values: EN, DE, ES, FR, IT, PT, TR, RU, JA or ALL.
```sh
cmake -G "Ninja" -DLDLANG=DE ..
```
