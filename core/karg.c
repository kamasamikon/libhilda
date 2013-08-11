/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <kmem.h>
#include <kstr.h>
#include <klog.h>

#include <karg.h>

#define ISBLANK(ch) ((ch) == ' ' || (ch) == '\t')
#define INITIAL_MAXARGC 16

void free_argv(char **vector)
{
	char **scan;

	if (vector != NULL) {
		for (scan = vector; *scan != NULL; scan++)
			kmem_free_s(*scan);
		kmem_free(vector);
	}
}

char **build_argv(const char *input, int *arg_c, char ***arg_v)
{
	int squote = 0, dquote = 0, bsquote = 0, argc = 0, maxargc = 0;
	char c, *arg, *copybuf, **argv = NULL, **nargv;

	*arg_c = argc;
	if (input == NULL)
		return NULL;

	copybuf = (char *)kmem_alloc(strlen(input) + 1, char);

	do {
		while (ISBLANK(*input))
			input++;

		if ((maxargc == 0) || (argc >= (maxargc - 1))) {
			if (argv == NULL)
				maxargc = INITIAL_MAXARGC;
			else
				maxargc *= 2;

			nargv = (char **)kmem_realloc(argv,
						maxargc * sizeof(char*));
			if (nargv == NULL) {
				if (argv != NULL) {
					free_argv(argv);
					argv = NULL;
				}
				break;
			}
			argv = nargv;
			argv[argc] = NULL;
		}

		arg = copybuf;
		while ((c = *input) != '\0') {
			if (ISBLANK(c) && !squote && !dquote && !bsquote)
				break;

			if (bsquote) {
				bsquote = 0;
				*arg++ = c;
			} else if (c == '\\') {
				bsquote = 1;
			} else if (squote) {
				if (c == '\'')
					squote = 0;
				else
					*arg++ = c;
			} else if (dquote) {
				if (c == '"')
					dquote = 0;
				else
					*arg++ = c;
			} else {
				if (c == '\'')
					squote = 1;
				else if (c == '"')
					dquote = 1;
				else
					*arg++ = c;
			}
			input++;
		}

		*arg = '\0';
		argv[argc] = kstr_dup(copybuf);
		if (argv[argc] == NULL) {
			free_argv(argv);
			argv = NULL;
			break;
		}
		argc++;
		argv[argc] = NULL;

		while (ISBLANK (*input))
			input++;
	} while (*input != '\0');

	kmem_free_s(copybuf);

	*arg_c = argc;
	*arg_v = argv;

	return argv;
}

int arg_find(int argc, char **argv, const char *opt, int fullmatch)
{
	int i;

	if (fullmatch) {
		for (i = 0; i < argc; i++)
			if (argv[i] && !strcmp(argv[i], opt))
				return i;
	} else {
		int slen = strlen(opt);
		for (i = 0; i < argc; i++)
			if (argv[i] && !strncmp(argv[i], opt, slen))
				return i;
	}
	return -1;
}

#ifdef KARG_TEST

/* Simple little test driver. */

static char *tests[] =
{
	"			a simple command line",
	"arg 'foo' is single\\ quoted",
	"arg \"bar\" is double    quoted",
	"arg \"foo bar\" has embedded			whitespace",
	"arg 'Jack said \\'hi\\'' has single quotes",
	"arg 'Jack said \\\"hi\\\"' has double quotes",
	"a b c d e f g h i j k l m n o p q r s t u v w x y z 1 2 3 4 5 6 7 8 9",

	/* This should be expanded into only one argument.  */
	"trailing-whitespace ",

	"",
	NULL
};

int main()
{
	int i, arg_c;
	char **arg_v;

	char **argv;
	char **test;
	char **targs;
	short ofsarr[222];

	for (test = tests; *test != NULL; test++) {
		printf ("build_argv(\"%s\")\n", *test);

		if ((argv = build_argv(*test, &arg_c, &arg_v)) == NULL)
			printf ("failed!\n\n");
		else {
			for (i = 0; i < arg_c; i++)
				printf("\t\"%s\"\n", arg_v[i]);
		}

		free_argv(argv);
	}

	return 0;
}

#endif	/* MAIN */

