#include <stdio.h>
#include <errno.h>
#include <stdbool.h>

#include "regulatory.h"
#include "reg.h"

int regulatory_init(void)
{
	int r;

	r = reglib_core_init();
	if (r)
		return r;

	return 0;
}
