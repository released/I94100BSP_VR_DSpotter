#ifndef SDK_TEST_TYPES_H
#define SDK_TEST_TYPES_H

/* Use the public SDK API with host-width handles in the mock tests. */
#define BASE_TYPES_H__
#define _CRT_SECURE_NO_WARNINGS
#include <stdint.h>
typedef unsigned char BYTE;
typedef short SHORT;
typedef int INT;
typedef void VOID;
#define HANDLE void *

#endif
