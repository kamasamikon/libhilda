/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>

#define ARRLEN 50

/* XXX: Please add -rdynamic when compile */
void show_trace(void)
{
	void *array[ARRLEN];
	size_t size;
	char **strings;
	size_t i;

	size = backtrace(array, ARRLEN);
	strings = backtrace_symbols(array, size);

	printf("Obtained %zd stack frames.\n", size);

	for (i = 0; i < size; i++)
		printf("%s\n", strings[i]);

	free(strings);
}

