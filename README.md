# About
What is skylabs? This is my own game engine and I plan to include all the best that other game engines have in it.

If you want to contact me, you can do it via Discord: grinlex

# For contributors
We will be very grateful if you write documentation for all functions. Example of writing documentation:
```c++
/**
* @brief Initializes BaseApplication variables
* @throws std::bad_alloc by std::filesystem::current_path(),
*   if memory cannot be allocated.
* @throws std::filesystem::filesystem_error by std::filesystem::current_path(),
*   if underlying OS API errors occur and constructed with the OS error code as
* the error code argument.
*/
static void Init();
/**
* @brief Adds the given value to the PATH environment variable.
* @param path - The absolute path to the folder.
* @throws current_func_exception if getenv_s() cannot find PATH.
* @throws current_func_exception with error code if _wgetenv_s failed.
*/
static void AddLibSearchPath(const std::string_view path);
```
Please note that here are exceptions that can be thrown from functions called by this function. We use [doxygen style](https://www.doxygen.nl/manual/docblocks.html) with @'s.
