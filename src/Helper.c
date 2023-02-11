#include "Helper.h"


int i32_add_overflow(int32_t a1, int32_t a2, int32_t* res)
{
	if (a1 >= 0) {
		if (INT_MAX - a1 < a2) {
			return 0;
		}
	}
	else {
		if (a2 < INT_MIN - a1) {
			return 0;
		}
	}
	*res = a1 + a2;
	return 1;
}