#define PW_DEPRECATED(v)	({ __typeof__(v) _v SPA_DEPRECATED = (v); (void)_v; (v); })

#define PW_KEY_MEDIA_TYPE "media.type"
#define PW_KEY_PRIOTITY_MASTER PW_DEPRECATED("priority.master")