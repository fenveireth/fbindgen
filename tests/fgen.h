
#define SUFFIX _1

#define RENAME_3(f, s) f ## s
#define RENAME_2(f, s) RENAME_3(f, s)
#define RENAME(f) RENAME_2(f, SUFFIX)

#define prerenamed_fun RENAME(prerenamed_fun)

void prerenamed_fun();
