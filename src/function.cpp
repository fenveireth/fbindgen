#include "stdafx.h"
#include "function.h"
#include "types.h"

using namespace std;
using namespace clang;

Fun::Fun(const FunctionDecl* d)
{
  rd_proto(d);
  is_inline = 0;
}

void Fun::rd_proto(const FunctionDecl* d)
{
  name = d->getNameAsString();
  QualType rt = d->getReturnType();

  vector<Str> argNames;
  vector<Str> argTypes;
  for (unsigned i=0; i < d->getNumParams(); ++i) {
    const ParmVarDecl* a = d->getParamDecl(i);
    Str n = escape_name(a->getNameAsString());
    QualType t = a->getType();
    Str ts = get_type(t);
    if (t->isConstantArrayType())
      ts = "&mut "s + ts;
    argNames.push_back(n);
    argTypes.push_back(ts);
  }
  if (d->isVariadic()) {
    argTypes.push_back("..."s);
  }

  decl = "pub fn "s + name + '(';
  type = "extern fn("s;
  for (unsigned i=0; i < argTypes.size(); ++i) {
    if (i > 0) {
      decl += ", "s;
      type += ", "s;
    }
    if (i < argNames.size())
      decl += argNames[i] + ": "s;
    decl += argTypes[i];
    type += argTypes[i];
  }
  decl += ')';
  type += ')';
  if (!rt->isVoidType()) {
    decl += " -> "s + get_type(rt);
    type += " -> "s + get_type(rt);
  }
}

