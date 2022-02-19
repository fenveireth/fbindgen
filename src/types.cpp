#include "stdafx.h"
#include "clang.h"
#include "types.h"

using namespace std;
using namespace clang;
using namespace llvm;

const ASTContext* compiler_context;

namespace {

Str primitive_type(QualType t)
{
	Type::TypeClass kind = t->getTypeClass();
	if (kind == Type::Builtin)
	{
		auto bt = static_cast<const BuiltinType*>(t.getTypePtr());
		int size = (int)compiler_context->getTypeSize(t);
		if (bt->getKind() == BuiltinType::Void)
			return "std::ffi::c_void";
		if (bt->isSignedInteger())
			return 'i' + to_string(size);
		if (bt->isUnsignedInteger())
			return 'u' + to_string(size);
		if (bt->isFloatingPoint())
			return 'f' + to_string(size);

		fprintf(stderr, "unhandled builtin type: %s\n", t.getAsString().c_str());
		t->dump();
		exit(1);
	}

	if (kind == Type::FunctionProto) {
		auto ft = static_cast<const FunctionProtoType*>(t.getTypePtr());
		Str res = "extern \"C\" fn("s;
		unsigned i = 0;
		for (; i < ft->getNumParams(); ++i) {
			if (i > 0)
				res += ", "s;
			res += get_type(ft->getParamType(i));
		}
		res += ')';
		QualType rt = ft->getReturnType();
		if (rt.getTypePtr() && !rt->isVoidType())
			res += " -> "s + get_type(rt);
		return res;
	}

	return ""s;
}

int anon_ctr;

Str get_name(QualType t)
{
	Str res = t.getAsString();
	if (!strncmp("const ", res.c_str(), 6))
		res = res.substr(6);
	if (!strncmp("struct ", res.c_str(), 7))
		res = res.substr(7);
	if (!strncmp("enum ", res.c_str(), 5))
		res = res.substr(5);
	if (!strncmp("union ", res.c_str(), 6))
		res = res.substr(6);
	return res;
}

struct Field
{
	Str name;
	Str type_name;
	QualType type;
};

Str w_type(QualType t)
{
	Str name = get_name(t);
	Type::TypeClass kind = t->getTypeClass();

	if (kind == Type::Typedef) {
		if (name == "size_t"s)
			return "usize"s;
		if (name == "ssize_t"s)
			return "isize"s;
		QualType can = t.getCanonicalType();
		if (name == get_name(can)) // 'typedef struct {} name' form
			t = can;
		else {
			fprintf(out, "pub type %s = %s;\n", name.c_str(), get_type(can).c_str());
			return name;
		}
	}

	if (t->isRecordType())
	{
		const RecordDecl* trd = static_cast<const RecordType*>(t.getTypePtr())->getDecl();
		if (is_anon(name))
			name = "anon_"s + to_string(anon_ctr++);

		vector<Field> fields;
		for (const FieldDecl* f : trd->fields()) {
			fields.emplace_back();
			Field& nf = fields.back();
			nf.name = f->getNameAsString();
			nf.type = f->getType();
			nf.type_name = get_type(nf.type);
		}

		fprintf(out, "#[repr(C)]\n");
		bool is_union = trd->isUnion();
		if (is_union)
			fprintf(out, "pub union %s {\n", name.c_str());
		else
			fprintf(out, "pub struct %s {\n", name.c_str());
		for (Field& f: fields) {
			Str fn = escape_name(f.name);
			Str ty = f.type_name;
			if (is_union)
				ty = "core::mem::ManuallyDrop<"s + ty + '>';
			if (f.type->isPointerType() && f.type->getPointeeType()->isFunctionType()) {
				// decaying function to pointer in Rust is a struggle
				// + could not have been an array start anyway
				ty = "Option<"s + get_type(f.type->getPointeeType()) + '>';
			}
			fprintf(out, "\tpub %s: %s,\n", fn.c_str(), ty.c_str());
		}
		if (!fields.size())
			fprintf(out, "\t_u: [u8; 0]\n");
		fprintf(out, "}\n");
	}
	else if (t->isEnumeralType()) {
		const EnumDecl* ted = static_cast<const EnumType*>(t.getTypePtr())->getDecl();
		fprintf(out, "pub type %s = %s;\n", name.c_str(), get_type(ted->getIntegerType()).c_str());
	}
	else {
		fprintf(out, "UNK %s\n", name.c_str());
		t->dump();
	}

	return name;
}

map<Str, Str> types;

}

Str escape_name(Str s)
{
	static set<Str> keywords { "box"s, "in"s, "match"s, "move"s, "ref"s, "type"s };
	if (keywords.find(s) != keywords.end())
		s = "r#"s + s;
	if (!s.size())
		s = "anon_"s + to_string(anon_ctr++);
	return s;
}

Str get_type(QualType t)
{
	Type::TypeClass kind = t->getTypeClass();

	if (kind == Type::Elaborated)
		return get_type(t.getCanonicalType());
	if (kind == Type::Paren)
		return get_type(t.getCanonicalType());
	if (kind == Type::Decayed)
		return get_type(static_cast<const DecayedType*>(t.getTypePtr())->getDecayedType());

	if (kind == Type::Pointer) {
		Str cst = t->getPointeeType().isConstQualified() ? "const"s : "mut"s;
		return "*"s + cst + " "s + get_type(t->getPointeeType());
	}

	Str p = primitive_type(t);
	if (p.size())
		return p;

	if (kind == Type::ConstantArray) {
		auto at = static_cast<const ConstantArrayType*>(t.getTypePtr());
		return '[' + get_type(at->getElementType()) + "; "s
				+ to_string(at->getSize()) + ']';
	}

	Str k = get_name(t);
	auto it = types.find(k);
	if (it != types.end())
		return it->second;

	types[k] = k; // temporary, for linked lists
	Str res = w_type(t);
	types[k] = res;
	return res;
}

void add_typedef(Str k, Str v)
{
	types[k] = v;
}
