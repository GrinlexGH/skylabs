# About
This is my own game engine and I plan to include all the best that other game engines have in it.

Requirements for build:
* CMake
* Vulkan SDK
* MSVC, g++/clang++

For other libraries cmake uses `build_dependencies` script. **You must install all dependencies to build these libraries manually.**

Discord: grinlex

## Style guide
```cpp
/*
 * Multi-line comment
*/
// class.hpp
namespace my_namespace {
    class CClass : public IBase
    {
    public:
        // 4 spaces for indentation
        int JustFunction() {
            int localVariable = 4;
            for( ) {
                switch( ) {
                    case 0: {
                        ...
                        break;
                    }
                    case 1: break;
                }

                if( ) {
                    ...
                    return localVariable;
                } else if() {
                    return localVariable;
                }
            }
            return m_memberVariable * localVariable;
        }

    private:
        int m_memberVariable;
    };

    CClass g_globalVariable;

    int UseGlobalVariable() {
        return g_globalVariable.JustFunction()
    }
};
```
---
Useful links:
- https://utf8everywhere.org/
- https://github.com/aminosbh/sdl2-cmake-modules/blob/master/FindSDL2.cmake
