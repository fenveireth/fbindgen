#define ARG (1 << 0)

void fun(unsigned a);

inline void no_extra_parens_on_cast()
{
	fun(ARG);
}
