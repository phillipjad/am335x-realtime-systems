#include <stdint.h>
#include <stdio.h>
#include <sys/utsname.h>

int main() {
	struct utsname sys_info = { 0 };
	int32_t result = uname(&sys_info);
	printf("System information: %s, %s, %s, %s\n", sys_info.sysname, sys_info.release, sys_info.version, sys_info.machine);
	printf("hello from BBB\n");
	return 0;
}
