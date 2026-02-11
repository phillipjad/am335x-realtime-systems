#include <stdint.h>
#include <stdio.h>

int32_t main(void) {
#if defined(DEBUG)
	printf("DEBUG MODE ENABLED\n");
#elif defined(NDEBUG)
	printf("RELEASE MODE ENABLED\n");
#else
	printf("NO MODE SELECTED\n");
	exit(1);
#endif
	printf("Hello world from p1!\n");
	return 0;
}
