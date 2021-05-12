#include "stdafx.h"
#include "macro.h"
#include "types.h"

using namespace std;
using namespace clang;

namespace {

Preprocessor* ctx;

void dump(vector<Token>& stack, int to)
{
  fprintf(stderr, "Stack:\n");
  for (Token t : stack)
  {
    Str tn = t.is(tok::identifier) ? t.getIdentifierInfo()->getName().str() : ""s;
    fprintf(stderr, "  %s %s\n", t.getName(), tn.c_str());
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

    dump(stack, 0);
    if (i >= from + 2 && stack[i-1].is(tok::hashhash)) {
      name = stack[i].getIdentifierInfo()->getName().str(); // could be stale from replace
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
    if (mi->isFunctionLike())
    {
      // obtain arg replacement lists
      int i_arg = 0;
      vector<Token> arr;
      int paren_bal = 0;
      int j = i + 2;
      for (;; ++j)
      {
        if (j >= to || i_arg >= (int)mi->getNumParams()) {
          printf("to %d iarg %d\n", to, i_arg);
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

}

Str fold_macro(IdentifierInfo& id, Preprocessor& cctx, const map<Str, UPtr<Fun>>& externs)
{
  ctx = &cctx;

  Str name = id.getName().str();
  const MacroInfo* mi = ctx->getMacroInfo(&id);
  if (mi->getNumParams() > 0)
    return ""s;

  Token t;
  t.setKind(tok::identifier);
  t.setIdentifierInfo(&id);
  vector<Token> stack { t };

  expand(stack, 0, 1);
  //dump(stack, 0);
  if (stack.size() == 1 && stack[0].is(tok::identifier)) {
    Str alias = stack[0].getIdentifierInfo()->getName().str();
    auto p = externs.find(alias);
    if (p != externs.end()) {
      return "pub const "s + name + ": "s + p->second->type + " = "s + alias + ';';
    }
  }


  return ""s;
}
