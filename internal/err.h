#ifndef NOCH_INTERNAL_ERR_H_HEADER_GUARD
#define NOCH_INTERNAL_ERR_H_HEADER_GUARD
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h> /* NULL */

#include "def.h"

typedef enum {
	NOCH_OK = 0,

	NOCH_ERR_OUT_OF_MEM,
	NOCH_ERR_FOPEN,
	NOCH_ERR_FREAD,
	NOCH_ERR_PARSER,

	NOCH_ERRS_COUNT,
} noch_err_t;

extern noch_err_t  noch_err;
extern const char *noch_err_msg;

NOCH_DEF noch_err_t  noch_get_err(void);
NOCH_DEF const char *noch_get_err_msg(void);
NOCH_DEF void        noch_clear_err(void);

#ifdef __cplusplus
}
#endif
#endif
