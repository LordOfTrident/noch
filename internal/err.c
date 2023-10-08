#ifndef NOCH_INTERNAL_ERR_H_IMPLEMENTATION_GUARD
#define NOCH_INTERNAL_ERR_H_IMPLEMENTATION_GUARD
#ifdef __cplusplus
extern "C" {
#endif

#include "err.h"
#include "assert.h"

static const char *noch_err_to_str_map[NOCH_ERRS_COUNT] = {
	"Ok", /* NOCH_OK */

	"Failed to open file",             /* NOCH_ERR_FOPEN */
	"Failed to read from file stream", /* NOCH_ERR_FREAD */
	"Error while parsing",             /* NOCH_ERR_PARSER */
};

noch_err_t  noch_err;
const char *noch_err_msg = "Ok";

#define NOCH_FOPEN_FAIL()    noch_set_err(NOCH_ERR_FOPEN,      NULL)
#define NOCH_FREAD_FAIL()    noch_set_err(NOCH_ERR_FREAD,      NULL)
#define NOCH_PARSER_ERR(MSG) noch_set_err(NOCH_ERR_PARSER,     MSG)

#define NOCH_CHECK_ALLOC(PTR)                  \
	do {                                       \
		if (PTR == NULL)                       \
			NOCH_ASSERT(0 && "Out of memory"); \
	} while (0)

int noch_set_err(noch_err_t err, const char *msg) {
	NOCH_ASSERT(err < NOCH_ERRS_COUNT && err >= 0);
	noch_err     = err;
	noch_err_msg = msg == NULL? noch_err_to_str_map[err] : msg;
	return -1;
}

NOCH_DEF noch_err_t noch_get_err(void) {
	return noch_err;
}

NOCH_DEF const char *noch_get_err_msg(void) {
	return noch_err_msg;
}

NOCH_DEF void noch_clear_err(void) {
	noch_err     = NOCH_OK;
	noch_err_msg = NULL;
}

#ifdef __cplusplus
}
#endif
#endif
