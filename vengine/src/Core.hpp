#pragma once

#ifdef VENGINE_BUILD_DLL
#define VENGINE_API __declspec(dllexport)
#else
#define VENGINE_API __declspec(dllimport)
#endif
