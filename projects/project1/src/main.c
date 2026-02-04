#include <stdio.h>

int main(void) {
	#if defined(DEBUG)
    		printf("DEBUG MODE ENABLED\n");
	#elif defined(NDEBUG)
		printf("RELEASE MODE ENABLED\n");
	#else
		printf("NO MODE SELECTED\n");
	#endif

    printf("Hello world from p1!\n");
    return 0;
}
