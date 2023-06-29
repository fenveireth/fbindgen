typedef void (*sqlite3_destructor_type)(void*);
#define SQLITE_STATIC ((sqlite3_destructor_type)0)