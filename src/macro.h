#include "function.h"

// names of pointer types
extern std::map<Str, clang::QualType> ptr_typedefs;

// create function alias or inline function
Str fold_macro(clang::IdentifierInfo& id,
		clang::Preprocessor& ctx,
		const std::map<Str, UPtr<Fun>>& externs,
		bool function);
