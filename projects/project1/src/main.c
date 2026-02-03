#include <stdio.h>

int main(void) {
	#ifdef DEBUG
    		printf("DEBUG MODE ENABLED\n");
	#else
		printf("RELEASE MODE ENABLED\n");
	#endif

    printf("Hello world from p1!\n");
    return 0;
}
