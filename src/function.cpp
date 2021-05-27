#include "stdafx.h"
#include "function.h"
#include "types.h"

using namespace std;
using namespace clang;

namespace {

Str dump(const Stmt*, bool r = false);

Str dump_cast(const Expr* val, QualType to)
{
  QualType from = val->getType();
  if (from->isFunctionType()) // extract function pointer before call
    return dump(val);

  Str to_s = get_type(to);
  Str from_s = get_type(from);
  if (to_s == from_s)
    return dump(val);

  if (from->isPointerType() && to->isPointerType()
      && (from->getPointeeType().isConstQualified() == to->getPointeeType().isConstQualified())) {
    return dump(val) + ".cast::<"s + get_type(to->getPointeeType()) + ">()"s;
  }

  Str dmp = dump(val);
  if (to->isPointerType() && dmp == "0"s) {
    Str op = to->getPointeeType().isConstQualified() ?
              "std::ptr::null::<"s :
              "std::ptr::null_mut::<"s;
    return op + get_type(to->getPointeeType()) + ">()"s;
  }

  return "std::mem::transmute::<"s + from_s + ", "s + to_s + ">("s + dmp + ')';
}

Str dump(const Stmt* s, bool remove_parens)
{
  Str res;
  const BinaryOperator* bo;
  const CallExpr* ce;
  const CastExpr* ca;
  const MemberExpr* me;
  const UnaryOperator* uo;
  switch (s->getStmtClass())
  {
  case Stmt::BinaryOperatorClass:
    bo = static_cast<const BinaryOperator*>(s);
    return dump(bo->getLHS()) + ' ' + bo->getOpcodeStr().str() + ' ' + dump(bo->getRHS());
  case Stmt::CStyleCastExprClass:
    ca = static_cast<const CastExpr*>(s);
    return dump_cast(ca->getSubExpr(), ca->getType());
  case Stmt::CallExprClass:
    ce = static_cast<const CallExpr*>(s);
    res = dump(ce->getCallee()) + '(';
    for (unsigned i = 0; i < ce->getNumArgs(); ++i) {
      res += dump(ce->getArg(i), true);
      if (i < ce->getNumArgs() - 1)
        res += ", "s;
    }
    return res + ')';
  case Stmt::CompoundStmtClass:
    for (Stmt* c : static_cast<const CompoundStmt*>(s)->body()) {
      res += dump(c) + ";\n"s;
    }
    return res;
  case Stmt::DeclRefExprClass:
    return static_cast<const DeclRefExpr*>(s)->getDecl()->getNameAsString();
  case Stmt::DeclStmtClass:
    for (const Decl* d : static_cast<const DeclStmt*>(s)->decls())
    {
      // auto rd = dynamic_cast<const RecordDecl*>(d); // already works as anon type without effort
      auto vd = dynamic_cast<const VarDecl*>(d);
      if (vd) {
        if (res.size())
          res += ";\n"s;
        res += "let mut "s + vd->getNameAsString() + ": "s + get_type(vd->getType());
        if (vd->getInit())
          res += " = "s + dump(vd->getInit());
      }
    }
    return res;
  case Stmt::ImplicitCastExprClass:
    ca = static_cast<const CastExpr*>(s);
    return dump_cast(ca->getSubExpr(), ca->getType());
  case Stmt::IntegerLiteralClass:
    return static_cast<const IntegerLiteral*>(s)->getValue().toString(10, true);
  case Stmt::MemberExprClass:
    me = static_cast<const MemberExpr*>(s);
    res = dump(me->getBase());
    if (me->isArrow())
      res = "(*"s + res + ')';
    return res + '.' + me->getMemberDecl()->getNameAsString();
  case Stmt::ParenExprClass:
    res = dump(static_cast<const ParenExpr*>(s)->getSubExpr(), remove_parens);
    if (!remove_parens)
      return res = '(' + res + ')';
    return res;
  case Stmt::ReturnStmtClass:
    return "return "s + dump(static_cast<const ReturnStmt*>(s)->getRetValue(), true);
  case Stmt::UnaryOperatorClass:
    uo = static_cast<const UnaryOperator*>(s);
    return UnaryOperator::getOpcodeStr(uo->getOpcode()).str() + dump(uo->getSubExpr());
  default:
    return "<ASTDUMP:"s + s->getStmtClassName() + '>';
  }
}

}

Fun::Fun(const FunctionDecl* d)
{
  rd_proto(d);
  is_inline = 0;
  if (d->isInlined()) {
    is_inline = 1;
    rd_body(d);
  }
}

void Fun::rd_proto(const FunctionDecl* d)
{
  name = d->getNameAsString();
  QualType rt = d->getDeclaredReturnType();

  vector<Str> argNames;
  vector<Str> argTypes;
  for (unsigned i=0; i < d->getNumParams(); ++i) {
    const ParmVarDecl* a = d->getParamDecl(i);
    Str n = escape_name(a->getNameAsString());
    QualType t = a->getType();
    Str ts = get_type(t);
    if (t->isConstantArrayType())
      ts = "&mut "s + ts;
    if (t->isPointerType() && t->getPointeeType()->isFunctionType()) {
      // decaying function to pointer in Rust is a struggle
      // + could not have been an array start anyway
      ts = "Option<"s + get_type(t->getPointeeType()) + '>';
    }
    argNames.push_back(n);
    argTypes.push_back(ts);
  }
  if (d->isVariadic()) {
    argTypes.push_back("..."s);
  }

  decl = "pub fn "s + name + '(';
  type = "unsafe extern fn("s;
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

void Fun::rd_body(const FunctionDecl* d)
{
  decl.insert(3, " unsafe"s);
  decl += " {\n"s + dump(d->getBody()) + '}';
}
