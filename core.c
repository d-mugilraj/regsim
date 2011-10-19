#include "regulatory.h"

int main(void)
{
	int r;

	r = regulatory_init();
	if (r)
		return r;

	return 0;
}
