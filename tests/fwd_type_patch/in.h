struct T;

typedef void (*r1)(struct T* ref);

void r2(struct T* ref);

inline void r3()
{
	struct T* ptr = 0;
}

struct T {
	int member;
};
