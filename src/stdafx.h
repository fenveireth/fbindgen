
#include <fcntl.h>
#include <unistd.h>

#include <clang/Basic/Version.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>

#include <llvm/Support/CommandLine.h>

#include <regex>
#include <unordered_set>
#include <iostream>

template <class T> using UPtr = std::unique_ptr<T>;
template <class T> using SPtr = std::shared_ptr<T>;
using Str = std::string;
using uint = unsigned int;
