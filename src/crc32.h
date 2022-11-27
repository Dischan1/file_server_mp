#include <stddef.h>
#include <stdint.h>

uint32_t calc_crc32(const char* s, size_t n)
{
	uint32_t crc = 0xFFFFFFFF;

	for (size_t i = 0; i < n; i++) {
		char ch = s[i];
		for (size_t j = 0; j < 8; j++) {
			uint32_t b = (ch ^ crc) & 1;
			crc >>= 1;
			if (b) crc = crc ^ 0xEDB88320;
			ch >>= 1;
		}
	}

	return ~crc;
}