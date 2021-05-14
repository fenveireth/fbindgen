#include "stdafx.h"
#include "detect.h"
#include "macro.h"
#include "types.h"

using namespace std;
using namespace clang;
using namespace clang::tooling;

namespace {

vector<regex> filter_functions;
vector<regex> filter_consts;
vector<regex> filter_types;
vector<regex> filter_inlines;

bool exports(const Str& name, const vector<regex>& lst)
{
  for (auto& r : lst) {
    if (regex_match(name, r))
      return true;
  }

  return false;
}

map<Str, UPtr<Fun>> functions;
map<Str, Str> globals;

void traverse(const Decl* d)
{
  auto kind = d->getKind();
  if (kind == Decl::Function) {
    auto fun = dynamic_cast<const FunctionDecl*>(d);
    Str funname = fun->getNameAsString();
    if (exports(funname, fun->isInlined() ? filter_inlines : filter_functions))
      functions[funname] = make_unique<Fun>(fun);
  }
  else if (kind == Decl::Enum)
  {
    auto enu = dynamic_cast<const EnumDecl*>(d);
    for (const EnumConstantDecl* c : enu->enumerators()) {
      Str name = c->getNameAsString();
      if (exports(name, filter_consts)) {
        Str ty = get_type(enu->getIntegerType());
        printf("pub const %s: %s = %s;\n", name.c_str(), ty.c_str(), c->getInitVal().toString(10, true).c_str());
      }
    }
  }
  else if (kind == Decl::Typedef) {
    auto td = dynamic_cast<const TypedefNameDecl*>(d);
    Str name = td->getNameAsString();
    if (exports(name, filter_types)) {
      Str to = get_type(td->getUnderlyingType());
      if (name != to) {
        add_typedef(name, to);
        printf("pub type %s = %s;\n", name.c_str(), to.c_str());
      }
    }
  }
  else if (kind == Decl::Var) {
    auto vd = dynamic_cast<const VarDecl*>(d);
    Str name = vd->getNameAsString();
    if (vd->hasExternalStorage() && exports(name, filter_consts)) {
      QualType t = vd->getType();
      globals[name] = get_type(t);
    }
  }
//  else
//    printf("UNK (td): %s\n", d->getDeclKindName());
}

class FGConsumer : public ASTConsumer
{
public:
  bool HandleTopLevelDecl(DeclGroupRef g) override
  {
    for (Decl* d : g)
      traverse(d);

    return true;
  }
};

SPtr<Preprocessor> preprocessor;

class FEAction : public ASTFrontendAction
{
public:
  UPtr<ASTConsumer> CreateASTConsumer(CompilerInstance& ci, StringRef file) override
  {
    preprocessor = ci.getPreprocessorPtr();
    compiler_context = &(ci.getASTContext());
    return make_unique<FGConsumer>();
  }
};

void dump(const vector<Str>& link)
{
  printf("extern {\n");
  for (auto& p: globals) {
    printf("  pub static %s: %s;\n", p.first.c_str(), p.second.c_str());
  }

  for (auto& p : functions) {
    if (!p.second->is_inline) {
      printf("  %s;\n", p.second->decl.c_str());
    }
  }
  printf("}\n");

  for (const Str& lib : link)
    printf("#[link(name=\"%s\")] extern {}\n", lib.c_str());

  // alias functions via #define
  auto& ids = preprocessor->getIdentifierTable();
  for (const auto& it : ids)
  {
    IdentifierInfo& id = ids.get(it.first());
    Str name = id.getName().str();
    if (id.hasMacroDefinition() && exports(name, filter_inlines)) {
      Str def = fold_macro(id, *preprocessor, functions);
      if (def.size())
        printf("%s\n", def.c_str());
    }
  }

  for (const auto& p : functions) {
    if (p.second->is_inline) {
      printf("%s\n", p.second->decl.c_str());
    }
  }
}

llvm::cl::OptionCategory toolCat("fgen options"s);

}

int main(int argc, const char** argv)
{
  auto include_dirs = detect_clang();
  vector<Str> cmd_line;
  for (auto arg : include_dirs) {
    cmd_line.push_back("-I"s + arg);
  }

  vector<Str> pkgconf_deps;
  char buff[64];
  while (scanf("%63s", buff) == 1) {
    Str line(buff);
    if (!strncmp("Pkgconf:", buff, 8))
      call_pkgconf(line.substr(8), cmd_line, pkgconf_deps);
    else if (!strncmp("Functions:", buff, 10))
      filter_functions.push_back(regex(line.substr(10)));
    else if (!strncmp("Constants:", buff, 10))
      filter_consts.push_back(regex(line.substr(10)));
    else if (!strncmp("Types:", buff, 6))
      filter_types.push_back(regex(line.substr(6)));
    else if (!strncmp("Inlines:", buff, 8))
      filter_inlines.push_back(regex(line.substr(8)));
    else {
      fprintf(stderr, "unrecognized filter type: %s\n", buff);
      return 1;
    }
  }

  auto db = FixedCompilationDatabase("."s, cmd_line);
  vector<Str> sources { argv[1] };
  ClangTool tool(db, sources);

  int res = tool.run(newFrontendActionFactory<FEAction>().get());
  if (res == 0) {
    dump(pkgconf_deps);
  }

  return res;
}
