#include "function.h"

// create function alias or inline function
Str fold_macro(clang::IdentifierInfo& id,
              clang::Preprocessor& ctx,
              const std::map<Str, UPtr<Fun>>& externs,
              bool function);
