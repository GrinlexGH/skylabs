# About
This is my own game engine and I plan to include all the best that other game engines have in it.

Requirements for build:
* CMake
* Vulkan SDK
* MSVC, g++/clang++

For other libraries cmake uses `build_dependencies` script. **You must install all dependencies to build these libraries manually.**

Discord: grinlex

## Style guide
### General
- Use 4 spaces for indentation.
- Use space before opening brace instead of new line (classes and structs are exceptions).
- Use space after control statement.
```cpp
int Func() {
    if (bar) {
    }
}
```

### Variables
- Use camelCase.
- Use `g_` prefix for globals.
- Use `m_` prefix for members.

### Functions
- Use PascalCase.

### Classes and structs
- Use new line after brace.
- Do NOT use indentation before access modifier.
- Use I prefix for interfaces, C prefix for classes
```cpp
class CClass : IBase
{
public:

};
```

### Comments
- Use space after start of comment.
---
Useful links:
- https://utf8everywhere.org/
