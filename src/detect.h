
// gather standard include directories, to used with '-I' arguments
std::vector<Str> detect_clang();

void call_pkgconf(Str name, std::vector<Str>& clang_args, std::vector<Str>& libs);
