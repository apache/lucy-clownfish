#include "charmony.h"

#ifdef CHY_HAS_STDBOOL_H
# include <stdbool.h>
#elif !defined(__cplusplus)
typedef char bool;
# define true 1
# define false 0
#endif

#define CMARK_ATTRIBUTE(list)
