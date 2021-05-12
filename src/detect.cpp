#include "stdafx.h"

using namespace std;

vector<Str> detect_clang()
{
  int pip[2] = {};
  pipe(pip);
  int orig_out = dup(1);
  int orig_err = dup(2);
  dup2(pip[1], 1);
  dup2(pip[1], 2);

  char buff[2048];
  FILE* out = popen("clang -E -v -", "w");
  pclose(out);
  close(pip[1]);
  read(pip[0], buff, 2047);
  dup2(orig_out, 1);
  dup2(orig_err, 2);
  close(orig_out);
  close(orig_err);
  close(pip[0]);

  vector<Str> res;
  const char* cur = strstr(buff, "#include <...> search starts here:");
  while ((cur = strchr(cur, '/'))) {
    const char* c2 = strchr(cur, '\n');
    res.push_back(Str(cur, c2 - cur));
    cur = c2;
  }

  return res;
}

void call_pkgconf(Str name, vector<Str>& clang_args, vector<Str>& libs)
{
  Str cmd = "pkgconf --cflags --libs "s + name;
  FILE* proc = popen(cmd.c_str(), "r");
  char buff[256] = {};
  int rd = fread(buff, 1, 255, proc);
  pclose(proc);
  if (rd <= 0)
    exit(1);

  char* tok = strtok(buff, " \n");
  do {
    if (!strncmp(tok, "-l", 2))
      libs.push_back(tok + 2);
    else
      clang_args.push_back(tok);
  } while ((tok = strtok(nullptr, " \n")));
}
