#include "stdafx.h"
#include "macro.h"
#include "types.h"

//#define TRACE

using namespace std;
using namespace clang;

namespace {

Preprocessor* ctx;

void dump(vector<Token>& stack, int to)
{
	fprintf(stderr, "Stack:\n");
	for (Token t : stack)
	{
		Str tn = t.is(tok::identifier) ? t.getIdentifierInfo()->getName().str() :
				t.is(tok::numeric_constant) ? Str(t.getLiteralData(), t.getLength()) :
				""s;
		fprintf(stderr, "	%s %s\n", t.getName(), tn.c_str());
	}
}

void bail_out(const char* reason, vector<Token> stack)
{
	fprintf(stderr, "Macro: %s\n", reason);
	dump(stack, 0);
	abort();
}

int expand(vector<Token>& stack, int from, int to);

// replace arguments in expanded function-like macro body
// Return: body growth
int replace_args(vector<Token>& stack, int from, int to, const map<Str, vector<Token>>& args)
{
	int initial_to = to;
	for (int i = from; i < to; ++i)
	{
		if (!stack[i].is(tok::identifier))
			continue;
		const IdentifierInfo* id = stack[i].getIdentifierInfo();
		Str name = id->getName().str();
		auto p = args.find(name);

		int rlen = 1;
		bool rep = p != args.end();
		if (rep) {
			rlen = p->second.size();
			stack.erase(stack.begin() + i);
			stack.insert(stack.begin() + i, p->second.begin(), p->second.end());
		}

		if (i >= from + 2 && stack[i-1].is(tok::hashhash) && stack[i].is(tok::identifier))
		{
			// name may now be stale from replace
			name = stack[i].getIdentifierInfo()->getName().str();
			Token t = stack[i-2];
			if (stack[i-2].is(tok::identifier)) {
				t.setKind(tok::identifier);
				Str newname = stack[i-2].getIdentifierInfo()->getName().str() + name;
				t.setIdentifierInfo(&ctx->getIdentifierTable().get(newname));
			}
			stack.erase(stack.begin() + i - 2, stack.begin() + i);
			i -= 2;
			to -= 2;
			stack[i] = t;
		}
		else if (rep && i + rlen < to && stack[i+rlen].isNot(tok::hashhash)) {
			rlen += expand(stack, i, i + rlen);
		}

		i += rlen - 1;
		to += rlen - 1;
	}

	return to - initial_to;
}

unordered_set<Str> no_recursion;

int expand(vector<Token>& stack, int from, int to)
{
	int initial_to = to;

	// examine each token for replace
	for (int i = from; i < to; ++i)
	{
		if (!stack[i].is(tok::identifier))
			continue;
		const IdentifierInfo* id = stack[i].getIdentifierInfo();
		const MacroInfo* mi = ctx->getMacroInfo(id);

		Str name = id->getName().str();
		if (!mi || no_recursion.count(name))
			continue;

		int erase_to = i + 1;
		map<Str, vector<Token>> args;
		if (mi->isFunctionLike() && i < to - 1 && stack[i + 1].is(tok::l_paren))
		{
			// obtain arg replacement lists
			int i_arg = 0;
			vector<Token> arr;
			int paren_bal = 0;
			int j = i + 2;
			for (;; ++j)
			{
				if (j >= to || i_arg >= (int)mi->getNumParams()) {
					fprintf(stderr, "to %d iarg %d\n", to, i_arg);
					bail_out("bad function-like macro call", stack);
				}

				Token t = stack[j];
				if (t.is(tok::l_paren))
					++paren_bal;
				else if (paren_bal > 0 && t.is(tok::r_paren))
					--paren_bal;
				else if (paren_bal == 0 && t.isOneOf(tok::comma, tok::r_paren)) {
					args[mi->params()[i_arg++]->getName().str()] = arr;
					arr.clear();
					if (t.is(tok::r_paren))
						break;
					continue;
				}

				arr.push_back(t);
			}
			erase_to = j + 1;
		}

		stack.erase(stack.begin() + i, stack.begin() + erase_to);
		vector<Token> rlist(mi->tokens_begin(), mi->tokens_end());
		stack.insert(stack.begin() + i, rlist.begin(), rlist.end());
		int growth = replace_args(stack, i, i + rlist.size(), args);
		no_recursion.insert(name);
		growth += expand(stack, i, i + rlist.size() + growth);
		no_recursion.erase(name);
		int growth2 = rlist.size() - (erase_to - i);
		to += growth + growth2;
		i += growth + rlist.size() - 1;
	}

	return to - initial_to;
}

struct ConstExpr
{
	Str value;
	Str type;
};

long get_num(ConstExpr& t)
{
	return stol(t.value, 0, 0);
}

bool is_num(const ConstExpr& e)
{
	return e.type.size()
		&& e.type != "id"s && e.type != "str"s && e.type != "type"s;
}

#ifdef TRACE
void dump(const vector<ConstExpr>& s)
{
	fprintf(stderr, "stack:\n");
	for (auto& e : s)
		fprintf(stderr, "%s %s\n", e.value.c_str(), e.type.c_str());
}
#endif

// Constant-fold, to retrieve one value with one type
void fold(vector<ConstExpr>& stack, int from, int to)
{
	ConstExpr res;
	bool progress;

#define P2(b) for (int i = from; i < to - 1; ++i) { \
	ConstExpr t0 = stack[i], t1 = stack[i + 1]; \
	ConstExpr r = t0; \
	b \
	progress = true;\
	stack.erase(stack.begin() + i + 1, stack.begin() + i + 2); \
	to -= 1;\
	stack[i] = r; \
}

#define P3(b) for (int i = from; i < to - 2; ++i) { \
	ConstExpr t0 = stack[i], t1 = stack[i + 1], t2 = stack[i + 2]; \
	ConstExpr r = t0; \
	b \
	progress = true;\
	stack.erase(stack.begin() + i + 1, stack.begin() + i + 3); \
	to -= 2;\
	stack[i] = r; \
}

#define P4(b) for (int i = from; i < to - 3; ++i) { \
	ConstExpr t0 = stack[i], t1 = stack[i + 1], t2 = stack[i + 2], t3 = stack[i + 3]; \
	ConstExpr r = t0; \
	b \
	progress = true;\
	stack.erase(stack.begin() + i + 1, stack.begin() + i + 4); \
	to -= 3;\
	stack[i] = r; \
}

#define NBIN(op, c) P3({ \
	if (!(is_num(t0) && is_num(t2) && t1.value == op)) \
		continue; \
	long a = get_num(t0); \
	long b = get_num(t2); \
	c \
})

	do
	{
		progress = false;

		NBIN(">>"s, { r.value = to_string(a >> b); })
		NBIN("|"s, { r.value = to_string(a | b); })

		P2({
			if (!(t0.value == "-"s && is_num(t1)))
				continue;
			r.type = t1.type;
			if (r.type[0] == 'u')
				r.type[0] = 'i';
			r.value = '-' + t1.value;
		})

		P4({
			if (!(t0.value == "("s
					&& (t1.type == "type"s || (t1.type == "id"s && ptr_typedefs.count(t1.value)))
					&& t2.value == ")"s
					&& is_num(t3)))
				continue;
			r.type = t1.value;
			bool is_ptr = (int)r.type.find('*') >= 0;
			if (t1.type == "id"s) {
				r.type = t1.value;
				is_ptr = true;
			}
			if (is_ptr)
				r.value = "unsafe { std::mem::transmute("s + t3.value + "isize) }"s;
			else
				r.value = t3.value;
		})

		P3({
			if (!(t0.value == "("s && is_num(t1) && t2.value == ")"s))
				continue;
			r = t1;
		})

		P2({
			if (!(t0.type == "type"s && t1.value == "*"s))
				continue;
			r.value = "*mut "s + t0.value;
		})

	} while (progress);

#undef NBIN
#undef P4
#undef P3
#undef P2
}

ConstExpr const_from_tkn(const Token& t)
{
	ConstExpr res;

	static const unordered_map<tok::TokenKind, ConstExpr> ops {
		{tok::amp,            { "&"s,                ""s }},
		{tok::ampamp,         { "&&"s,               ""s }},
		{tok::comma,          { ","s,                ""s }},
		{tok::equalequal,     { "=="s,               ""s }},
		{tok::exclaimequal,   { "!="s,               ""s }},
		{tok::greater,        { ">"s,                ""s }},
		{tok::greatergreater, { ">>"s,               ""s }},
		{tok::hashhash,       { "##"s,               ""s }}, // should not be seen, expand bug
		{tok::kw___attribute, { ""s,                 "attribute"s }},
		{tok::kw__Bool,       { "bool"s,             "type"s }},
		{tok::kw__Complex,    { "complex"s,          "type"s }},
		{tok::kw_char,        { "i8"s,               "type"s }},
		{tok::kw_const,       { "const"s,            ""s }},
		{tok::kw_do,          { "do"s,               ""s }},
		{tok::kw_double,      { "f64"s,              "type"s }},
		{tok::kw_enum,        { "enum"s,             ""s }},
		{tok::kw_extern,      { "extern"s,           ""s }},
		{tok::kw_float,       { "f32"s,              "type"s }},
		{tok::kw_inline,      { "inline"s,           ""s }},
		{tok::kw_int,         { "i32"s,              "type"s }},
		{tok::kw_long,        { "i64"s,              "type"s }},
		{tok::kw_restrict,    { "restrict"s,         ""s }},
		{tok::kw_short,       { "i16"s,              "type"s }},
		{tok::kw_signed,      { "i32"s,              "type"s }},
		{tok::kw_sizeof,      { "sizeof"s,           "id"s }},
		{tok::kw_struct,      { "struct"s,           ""s }},
		{tok::kw_unsigned,    { "u32"s,              "type"s }},
		{tok::kw_void,        { "std::ffi::c_void"s, "type"s }},
		{tok::kw_while,       { "while"s,            ""s }},
		{tok::l_brace,        { "{"s,                ""s }},
		{tok::l_paren,        { "("s,                ""s }},
		{tok::l_square,       { "["s,                ""s }},
		{tok::less,           { "<"s,                ""s }},
		{tok::lessless,       { "<<"s,               ""s }},
		{tok::minus,          { "-"s,                ""s }},
		{tok::period,         { "."s,                ""s }},
		{tok::pipe,           { "|"s,                ""s }},
		{tok::plus,           { "+"s,                ""s }},
		{tok::r_brace,        { "}"s,                ""s }},
		{tok::r_paren,        { ")"s,                ""s }},
		{tok::r_square,       { "]"s,                ""s }},
		{tok::semi,           { ":"s,                ""s }},
		{tok::slash,          { "/"s,                ""s }},
		{tok::star,           { "*"s,                ""s }},
	};

	auto k = t.getKind();
	if (k == tok::char_constant) {
		res.value = Str(t.getLiteralData(), t.getLength());
		res.type = "char"s;
	}
	else if (k == tok::identifier) {
		res.value = t.getIdentifierInfo()->getName().str();
		res.type = "id"s;
	}
	else if (k == tok::numeric_constant) {
		res.value = Str(t.getLiteralData(), t.getLength());
		int f;
		if ((int)res.value.find('.') >= 0) {
			res.type = "f64"s;
			if ((f = res.value.find('F')) >= 0) {
				res.value.erase(f);
				res.type = "f32"s;
			}
			if ((f = res.value.find('L')) >= 0) {
				res.value.erase(f);
				res.type = "f128"s;
			}
		}
		else {
			res.type = "i32";
			unsigned long v = strtoul(res.value.c_str(), nullptr, 0);
			if (v > 0x7FFFFFFF'FFFFFFFF)
				res.type = "u64"s;
			else if (v > 0xFFFFFFFF)
				res.type = "i64"s;
			else if (v > 0x7FFFFFFF)
				res.type = "u64"s;
			if ((f = res.value.find('U')) >= 0 || (f = res.value.find('u')) >= 0) {
				res.value.erase(f);
			}
			if ((f = res.value.find('L')) >= 0 || (f = res.value.find('l')) > 0) {
				res.value.erase(f);
				res.type = res.type[0] + "64"s;
			}
		}
	}
	else if (k == tok::string_literal) {
		res.value = Str(t.getLiteralData(), t.getLength());
		res.type = "str"s;
	}
	else {
		auto it = ops.find(k);
		if (it == ops.end()) {
			fprintf(stderr, "macro: can't extract from ttype %s\n", t.getName());
			abort();
		}
		res = it->second;
	}

	return res;
}

}

Str fold_macro(IdentifierInfo& id, Preprocessor& cctx, const map<Str,
		UPtr<Fun>>& externs, bool function)
{
	ctx = &cctx;

	Str name = id.getName().str();
	const MacroInfo* mi = ctx->getMacroInfo(&id);
	if (mi->getNumParams() > 0)
		return ""s;

	Token t;
	t.startToken();
	t.setKind(tok::identifier);
	t.setIdentifierInfo(&id);
	vector<Token> stack { t };

	expand(stack, 0, 1);

	vector<ConstExpr> cstack;
	for (auto& t : stack)
		cstack.push_back(const_from_tkn(t));
	fold(cstack, 0, cstack.size());

	if (cstack.size() == 1)
	{
		ConstExpr e = cstack[0];
		if (function && e.type == "id"s) {
			auto p = externs.find(e.value);
			if (p != externs.end()) {
				return "pub const "s + name + ": "s + p->second->type + " = "s + e.value + ';';
			}
		}

		if (!function && is_num(e) && e.type != "f128"s)
		{
			Str v = e.value;
			if (v[0] == '0' && v.size() > 1 && v[1] != 'x')
				v.insert(1, "o"s);
			auto it_typedef = ptr_typedefs.find(e.type);
			if (it_typedef != ptr_typedefs.end())
				get_type(it_typedef->second);
			return "pub const "s + name + ": "s + e.type + " = "s + v + ';';
		}

		if (!function && e.type == "str"s) {
			e.value.insert(e.value.size() - 1, "\\0"s);
			return "pub const "s + name + ": &str = "s + e.value + ';';
		}

		if (!function && e.type == "char"s) {
			return "pub const "s + name + ": char = "s + e.value + ';';
		}
	}

#ifdef TRACE
	fprintf(stderr, "===== %s\n", name.c_str());
	dump(cstack);
#endif
	return ""s;
}
