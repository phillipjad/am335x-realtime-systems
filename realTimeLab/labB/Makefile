CC_NATIVE := gcc
CC_ARM    := arm-linux-gnueabihf-gcc

CFLAGS := \
	-std=c2x \
	-Wall -Wextra -Wpedantic -Werror \
	-Wformat=2 -Wformat-security \
	-Wshadow -Wundef \
	-Wmissing-prototypes -Wmissing-declarations \
	-Wstrict-prototypes \
	-Wcast-qual -Wcast-align \
	-Wvla \
	-D_POSIX_C_SOURCE=202405L

CFLAGS_RELEASE := -O2
CFLAGS_DEBUG   := -g -O0 -DDEBUG

LDFLAGS := -lpthread -lrt

SRCS := labB.c
HDRS := labB.h

TARGET         := labB
TARGET_DEBUG   := labB_debug
TARGET_ARM     := labB_arm
TARGET_ARM_DBG := labB_arm_debug

.PHONY: all native native-debug arm arm-debug clean

all: native

native: $(SRCS) $(HDRS)
	$(CC_NATIVE) $(CFLAGS) $(CFLAGS_RELEASE) -o $(TARGET) $(SRCS) $(LDFLAGS)

native-debug: $(SRCS) $(HDRS)
	$(CC_NATIVE) $(CFLAGS) $(CFLAGS_DEBUG) -o $(TARGET_DEBUG) $(SRCS) $(LDFLAGS)

arm: $(SRCS) $(HDRS)
	$(CC_ARM) $(CFLAGS) $(CFLAGS_RELEASE) \
		-march=armv7-a -mfpu=neon -mfloat-abi=hard \
		-o $(TARGET_ARM) $(SRCS) $(LDFLAGS)

arm-debug: $(SRCS) $(HDRS)
	$(CC_ARM) $(CFLAGS) $(CFLAGS_DEBUG) \
		-march=armv7-a -mfpu=neon -mfloat-abi=hard \
		-o $(TARGET_ARM_DBG) $(SRCS) $(LDFLAGS)

clean:
	rm -f $(TARGET) $(TARGET_DEBUG) $(TARGET_ARM) $(TARGET_ARM_DBG)
