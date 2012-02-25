/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <stdio.h>
#include <string.h>

#include <kmem.h>
#include <klog.h>

#include <opt-rpc-common.h>

/**
 * \brief Parse the command line and return a offset array
 * of each opt.
 *
 * Caller must provide a LARGE ENOUGH array for ofsarr
 */
int get_argv(char cmdline[], int ofsarr[])
{
	int inspace = 1, argc = 0, i, len = strlen(cmdline);
	char c;

	for (i = 0; i < len; i++) {
		c = cmdline[i];
		if (inspace) {
			if ((' ' != c) && ('\r' != c) && ('\n' != c)) {
				inspace = 0;
				ofsarr[argc++] = i;
			}
		} else {
			if ((' ' == c) || ('\r' == c) || ('\n' == c)) {
				inspace = 1;
			}
		}
		if ((' ' == c) || ('\r' == c) || ('\n' == c))
			cmdline[i] = 0;
	}
	ofsarr[argc] = -1;
	return argc;
}

