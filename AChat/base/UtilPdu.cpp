#include "UtilPdu.h"
#include <stdlib.h>
#include <string.h>

/*
 * Warning!!!
 * This function return a static char pointer, caller must immediately copy the string to other place
 */
char* idtourl(uint32_t id)
{
	static char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
	static char buf[64];
	char *ptr;
	uint32_t value = id * 2 + 56;

	// convert to 36 number system
	ptr = buf + sizeof(buf) - 1;
	*ptr = '\0';

	do {
		*--ptr = digits[value % 36];
		value /= 36;
	} while (ptr > buf && value);

	*--ptr = '1';	// add version number

	return ptr;
}

uint32_t urltoid(const char* url)
{
	uint32_t url_len = strlen(url);
	char c;
	uint32_t number = 0;
	for (uint32_t i = 1; i < url_len; i++) {
		c = url[i];

		if (c >= '0' && c <='9') {
			c -= '0';
		} else if (c >= 'a' && c <= 'z') {
			c -= 'a' - 10;
		} else if (c >= 'A' && c <= 'Z') {
			c -= 'A' - 10;
		} else {
			continue;
		}

		number = number * 36 + c;
	}

	return (number - 56) >> 1;
}
