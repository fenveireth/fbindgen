
/// Display in base 10, signed
Str to_string(const llvm::APInt&);

/// Type is a nameless struct/union
bool is_anon(const Str& type_name);

Str fileinfo_iterator_get_name(const clang::SourceManager::fileinfo_iterator& it);