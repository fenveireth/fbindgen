#ifdef __cplusplus
# if __cplusplus >= 201103L
#  define SPA_ALIGNOF alignof
# endif
#elif __STDC_VERSION__ >= 202311L
#  define SPA_ALIGNOF alignof
#else
# include <stdbool.h>
# if __STDC_VERSION__ >= 201112L
#  define SPA_ALIGNOF _Alignof
# endif
#endif
#ifndef SPA_ALIGNOF
#define SPA_ALIGNOF __alignof__
#endif