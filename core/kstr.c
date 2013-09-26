/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <hilda/kstr.h>
#include <hilda/kmem.h>
#include <hilda/kstr.h>

kint kstr_cmp(const kchar *a_str0, const kchar *a_str1, kint a_len)
{
	if (!a_str0 || !a_str1)
		return 0;

	if (a_len > 0)
		return strncmp(a_str0, a_str1, a_len);
	else
		return strcmp(a_str0, a_str1);
}

kint kstr_icmp(const kchar *a_str0, const kchar *a_str1, kint a_len)
{
	if (!a_str0 || !a_str1)
		return 0;

	if (0 >= a_len)
		a_len = 0x7FFFFFFF;

	do {
		if (tolower((unsigned char) *a_str0) !=
				tolower((unsigned char) *a_str1++)) {
			return (int) tolower((unsigned char) *a_str0) -
				(int) tolower((unsigned char) *--a_str1);
		}
		if (*a_str0++ == 0)
			break;
	} while (--a_len != 0);
	return 0;
}

kchar *kstr_dup(const kchar *a_str)
{
	kint slen;
	kchar *ret;

	if (a_str) {
		slen = strlen(a_str);
		ret = kmem_alloc(slen + 1, char);
		memcpy(ret, a_str, slen + 1);
		return ret;
	}
	return knil;
}

kchar *kstr_subs(kchar *a_str, kchar a_from, kchar a_to)
{
	kchar *p = a_str;

	while (*p) {
		if (*p == a_from)
			*p = a_to;
		p++;
	}
	return a_str;
}

kint kstr_escxml(const kchar *a_is, kchar **a_os, kint *a_ol)
{
	const kchar *s;
	kchar *end;
	kint buflen = *a_ol;
	kint and_cnt = 0, lt_cnt = 0, gt_cnt = 0;
	kint quot_cnt = 0, tab_cnt = 0, nl_cnt = 0;

	if (!a_is)
		return -1;

	if (!*a_os) {
		/* caller not provide buffer, allocate here */
		kint ilen = strlen(a_is);
		for (s = a_is; *s; s++) {
			switch (*s) {
			case '&':
				and_cnt++;
				break;
			case '<':
				lt_cnt++;
				break;
			case '>':
				gt_cnt++;
				break;
			case '"':
				quot_cnt++;
				break;
			case 9:
				tab_cnt++;
				break;
			case 10:
			case 13:
				nl_cnt++;
				break;
			default:
				break;
			}
		}

		*a_ol = ilen + (and_cnt * 4) + (lt_cnt * 3) + (gt_cnt * 3) +
			(quot_cnt * 5) + (tab_cnt * 3) + (nl_cnt * 4) + 1;
		*a_os = (kchar *) kmem_alloc(*a_ol, char);
		if (!*a_os)
			return -1;
		buflen = *a_ol;
	}
	(*a_os)[0] = '\0';
	end = *a_os;

	for (s = a_is; *s; s++) {
		if ((s - a_is) > buflen) {
			*end = '\0';
			break;
		}

		switch (*s) {
		case '&':
			strcat(end, "&amp;");
			end += 5;
			break;
		case '<':
			strcat(end, "&lt;");
			end += 4;
			break;
		case '>':
			strcat(end, "&gt;");
			end += 4;
			break;
		case '"':
			strcat(end, "&quot;");
			end += 6;
			break;
		case 9:
			strcat(end, "&#9;");
			end += 4;
			break;
		case 10:
			strcat(end, "&#10;");
			end += 5;
			break;
		case 13:
			strcat(end, "&#13;");
			end += 5;
			break;
		default:
			*(end + 0) = *s;
			*(end + 1) = '\0';
			end += 1;
		}
	}

	*a_ol = end - (*a_os);
	return 0;
}

kint kstr_toint(const kchar *s, kint *ret)
{
	kint val;
	kint i, c, num_start = 0;

	if (s[0] == '+' || s[0] == '-')
		num_start = 1;

	if (s[num_start] == '0' && (s[num_start + 1] == 'x' ||
				s[num_start + 1] == 'X')) {
		for (i = num_start + 2; (c = s[i]); i++)
			if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')
						|| (c >= 'A' && c <= 'F'))) {
				return -1;
			}
		errno = 0;
		val = strtol(s, 0, 16);
	} else {
		for (i = num_start; (c = s[i]); i++)
			if (!(c >= '0' && c <= '9')) {
				return -1;
			}
		errno = 0;
		val = strtol(s, 0, 10);
	}

	if (errno != 0)
		return -1;
	*ret = val;
	return 0;
}

char *kstr_trim_left(char *str)
{
	char *p, *po;

	for (p = str; *p; p++) {
		if (isspace(*p))
			continue;

		if (p == str)
			break;

		for (po = str; *p; p++)
			*po++ = *p;
		*po++ = '\0';
		break;
	}

	return str;
}

char *kstr_trim_right(char *str)
{
	int i, len = strlen(str);

	for (i = len - 1; i >= 0; i--) {
		if (isspace(str[i]))
			continue;
		str[i + 1] = '\0';
		break;
	}

	return str;
}

char *kstr_trim(char *str)
{
	kstr_trim_right(str);
	kstr_trim_left(str);
	return str;
}

