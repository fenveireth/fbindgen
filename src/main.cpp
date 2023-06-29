#include "stdafx.h"
#include "clang.h"
#include "detect.h"
#include "macro.h"
#include "types.h"

using namespace std;
using namespace clang;
using namespace clang::tooling;
using namespace llvm;

FILE* out;
SPtr<Preprocessor> preprocessor;
vector<Str> pkgconf_deps;
map<Str, clang::QualType> ptr_typedefs;

namespace {

FILE* out_includes;

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

map<Str, const FunctionDecl*> functions;
map<Str, Str> globals;
map<Str, const RecordDecl*> types;
map<Str, const TypedefNameDecl*> typedefs;

void traverse(const Decl* d)
{
	auto kind = d->getKind();
	if (kind == Decl::Function) {
		auto fun = dynamic_cast<const FunctionDecl*>(d);
		Str funname = fun->getNameAsString();
		if (exports(funname, fun->isInlined() ? filter_inlines : filter_functions))
			functions[funname] = fun;
	}
	else if (kind == Decl::Enum)
	{
		auto enu = dynamic_cast<const EnumDecl*>(d);
		for (const EnumConstantDecl* c : enu->enumerators()) {
			Str name = c->getNameAsString();
			if (exports(name, filter_consts)) {
				Str ty = get_type(enu->getIntegerType());
				fprintf(out, "pub const %s: %s = %s;\n", name.c_str(), ty.c_str(), to_string(c->getInitVal()).c_str());
			}
		}
	}
	else if (kind == Decl::Typedef) {
		auto td = dynamic_cast<const TypedefNameDecl*>(d);
		Str name = td->getNameAsString();
		if (exports(name, filter_types))
			typedefs[name] = td;
		if (td->getCanonicalDecl()->getUnderlyingType()->isPointerType())
			ptr_typedefs[name] = compiler_context->getTypeDeclType(td);
	}
	else if (kind == Decl::Var) {
		auto vd = dynamic_cast<const VarDecl*>(d);
		Str name = vd->getNameAsString();
		if (vd->hasExternalStorage() && exports(name, filter_consts)) {
			QualType t = vd->getType();
			globals[name] = get_type(t);
		}
	}
	else if (kind == Decl::Record) {
		auto rc = dynamic_cast<const RecordDecl*>(d);
		Str name = rc->getNameAsString();
		if (exports(name, filter_types)) {
			types[name] = rc; // write deferred, to ignore forward declarations
		}
	}
//	else
//		fprintf(stderr, "UNK (td): %s\n", d->getDeclKindName());
}

void dump(const vector<Str>& link)
{
	auto& ids = preprocessor->getIdentifierTable();

	map<Str, UPtr<Fun>> parsed_funs;
	for (auto& p : functions)
		parsed_funs[p.first] = make_unique<Fun>(p.second);

	// constants #define
	for (const auto& it : ids)
	{
		IdentifierInfo& id = ids.get(it.first());
		Str name = id.getName().str();
		if (id.hasMacroDefinition() && exports(name, filter_consts)) {
			Str def = fold_macro(id, *preprocessor, parsed_funs, false);
			if (def.size())
				fprintf(out, "%s\n", def.c_str());
		}
	}

	for (auto& p: types) {
		auto rc = p.second;
		QualType t(rc->getTypeForDecl(), 0);
		get_type(t);
	}

	for (auto& p: typedefs)
	{
		Str to = get_type(p.second->getUnderlyingType());
		if (!had_typedef(p.first) && p.first != to) {
			fprintf(out, "pub type %s = %s;\n", p.first.c_str(), to.c_str());
		}
	}

	fprintf(out, "extern {\n");
	for (auto& p: globals) {
		fprintf(out, "	pub static %s: %s;\n", p.first.c_str(), p.second.c_str());
	}

	for (auto& p : parsed_funs) {
		if (!p.second->is_inline)
			fprintf(out, "\t%s;\n", p.second->decl.c_str());
	}
	fprintf(out, "}\n");

	for (const Str& lib : link)
		fprintf(out, "#[link(name=\"%s\")] extern {}\n", lib.c_str());

	// alias functions via #define
	for (const auto& it : ids)
	{
		IdentifierInfo& id = ids.get(it.first());
		Str name = id.getName().str();
		if (id.hasMacroDefinition() && exports(name, filter_inlines)) {
			Str def = fold_macro(id, *preprocessor, parsed_funs, true);
			if (def.size())
				fprintf(out, "%s\n", def.c_str());
		}
	}

	for (const auto& p : parsed_funs) {
		if (p.second->is_inline)
			fprintf(out, "%s\n", p.second->decl.c_str());
	}

	// All opened .h files
	auto it = preprocessor->getSourceManager().fileinfo_begin();
	while (++it != preprocessor->getSourceManager().fileinfo_end())
	{
		fprintf(out_includes, "%s\n", it->first->getName().str().c_str());
	}
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

	void HandleTranslationUnit(ASTContext&) override
	{
		dump(pkgconf_deps);
	}
};

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

llvm::cl::OptionCategory toolCat("fgen options"s);

}

int main(int argc, const char** argv)
{
	auto include_dirs = detect_clang();
	vector<Str> cmd_line;
	for (auto arg : include_dirs) {
		cmd_line.push_back("-I"s + arg);
	}

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

	out = fopen("out.rs", "w");
	out_includes = fopen("includes.txt", "w");
	auto db = FixedCompilationDatabase("."s, cmd_line);
	vector<Str> sources { argv[1] };
	ClangTool tool(db, sources);

	int res = tool.run(newFrontendActionFactory<FEAction>().get());
	fclose(out);
	fclose(out_includes);
	return res;
}
