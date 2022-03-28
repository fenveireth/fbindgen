#include "stdafx.h"

using namespace std;

Str to_string(const llvm::APInt& i)
{
#if CLANG_VERSION_MAJOR == 12
	return i.toString(10, true);
#elif CLANG_VERSION_MAJOR >= 13
	return toString(i, 10, true);
#else
#error "clang version not recognized"
#endif
}

bool is_anon(const Str& type_name)
{
#if CLANG_VERSION_MAJOR == 12
	return type_name.find(":(anonymous at") != (size_t)-1;
#elif CLANG_VERSION_MAJOR >= 13
	return type_name.find(":(unnamed at") != (size_t)-1;
#else
#error "clang version not recognized"
#endif
}
