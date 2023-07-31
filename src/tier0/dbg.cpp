//===================================
//
// Purpose: Implementation of all things for debugging.
//
//===================================

#//include <string>
//#include "dbg.hpp"

#ifdef _DEBUG

#include <SDL.h>

//In short, std::wstring is needed for manipulating string,
//while std::wstring_view is only needed to read the string unchanged.
void _Assertion(
    const std::wstring_view FileName,
    const int& Line,
    const std::wstring_view Expression,
    const std::wstring_view Message,
    int& Button
) {
    constexpr wchar_t TemplateString[] = L"Assertion failed!\n\nProgram: %s\nFile: %s\nLine: %d\n\nExpression: %s\nMessage: %s";

    wchar_t ProgramName[256];
    GetModuleFileNameEx(GetCurrentProcess(), nullptr, ProgramName, MAX_PATH);
    wchar_t compactPath[MAX_PATH];
    PathCompactPathEx(compactPath, ProgramName, MAX_PATH, 35);
    wcscpy_s(ProgramName, compactPath);

	std::wstring LineC = std::to_wstring(Line);
	size_t MessageSize = wcslen(TemplateString)	+ 1 +	//1 is the null-terminator
						 wcslen(ProgramName)		+
						 FileName.size()			+
						 Expression.size()			+
						 Message.size()				+
						 LineC.size();
	std::wstring AssertMessage(MessageSize, L'\0');
	swprintf_s(
		AssertMessage.data(), MessageSize, TemplateString, ProgramName,
		FileName.data(),
		Line,
		Expression.data(),
		Message.data()
	);

    SDL_Window* window = SDL_CreateWindow(AssertationWindowLabel,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        680, 480,
        0);

    SDL_UpdateWindowSurface(window);

	Button = MessageBoxW(
		nullptr,
		AssertMessage.c_str(),
		L"Assertion failed!",
		MB_ICONERROR | MB_ABORTRETRYIGNORE
	);
	/*switch (Button) {
		// Скорей всего не будет.
		case IDRETRY:
			// https://stackoverflow.com/questions/744055/gcc-inline-assembly-jump-to-label-outside-block?rq=1
			// https://stackoverflow.com/questions/6421433/address-of-labels-msvc
			// https://gcc.gnu.org/onlinedocs/gcc-3.2/gcc/Labels-as-Values.html
			// https://stackoverflow.com/questions/17357199/using-goto-function-across-different-functions
			// https://stackoverflow.com/questions/6166437/64bit-applications-and-inline-assembly
			// https://xkcd.com/292/
			break;
	}*/
}
//If we compile in Release, _Assertation doesn't do anything
//#else
void _Assertion(
	const std::wstring_view FileName,
	const int& Line,
	const std::wstring_view Expression,
	const std::wstring_view Message,
	int& Button
) { }
#endif

