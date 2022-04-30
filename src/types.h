
extern FILE* out;

/// must be set to work out type sizes
extern const clang::ASTContext* compiler_context;

Str escape_name(Str);

/// Write to file if needed, return allocated name
Str get_type(clang::QualType);

bool had_typedef(Str);
