/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <kstr.h>
#include <kmem.h>

/* 0 for success, 1 for v buffer too small, -1 for not found */
int url_getv(const char *str, const char *k, int klen, char v[], int len)
{
	int i;
	const char *p, *start = str;

#define FOUND_END(c) (!(c) || (c) == '&' || (c) == '\r' || (c) == '\n')

	if (klen <= 0)
		klen = strlen(k);

	while (p = strstr(start, k)) {
		p += klen;
		if (p[0] != '=') {
			start = p + 1;
			continue;
		}

		p++;
		for (i = 0; i < len; i++) {
			v[i] = p[i];
			if (FOUND_END(p[i])) {
				v[i] = '\0';
				return 0;
			}
		}
		return 1;
	}
	return -1;
}

int split_ipaddr(const char *addr, char outip[128], unsigned short *outport)
{
	const char *inp;

	if (!addr || !outport)
		return -1;

	for (inp = addr; *inp; inp++)
		if (*inp != ':')
			*outip++ = *inp;
		else
			break;

	*outip = '\0';
	if (*inp != ':')
		return -1;

	*outport = (unsigned short)strtoul(inp + 1, NULL, 10);
	return 0;
}

