/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <hilda/kmem.h>
#include <hilda/kstr.h>
#include <hilda/klog.h>

#include <hilda/karg.h>

/*-----------------------------------------------------------------------
 * karg_xxx
 */
#define BLANK(c) ((c) == ' ' || (c) == '\t' || (c) == '\r' || (c) == '\n')
#define INIT_MAX_ARGC 16
#define ISNUL(c) ((c) == '\0')

int karg_free(int argc, char **argv)
{
	int i;

	if (argv == NULL)
		return -1;

	for (i = 0; i < argc; i++)
		kmem_free_s(argv[i]);
	kmem_free(argv);

	return 0;
}

/* XXX: Normal command line, NUL as end, SP as delimiters */
int karg_build(const char *input, int *arg_c, char ***arg_v)
{
	int squote = 0, dquote = 0, bsquote = 0, argc = 0, maxargc = 0;
	char c, *arg, *copybuf, **argv = NULL, **kargv;

	*arg_c = argc;
	if (input == NULL)
		return -1;

	copybuf = kmem_alloc((strlen(input) + 1), char);

	do {
		while (BLANK(*input))
			input++;

		if ((maxargc == 0) || (argc >= (maxargc - 1))) {
			if (argv == NULL)
				maxargc = INIT_MAX_ARGC;
			else
				maxargc *= 2;

			kargv = (char **)kmem_realloc(argv,
					maxargc * sizeof(char*));
			if (kargv == NULL) {
				if (argv != NULL) {
					kmem_free(argv);
					argv = NULL;
				}
				break;
			}
			argv = kargv;
			argv[argc] = NULL;
		}

		arg = copybuf;
		while ((c = *input) != '\0') {
			if (BLANK(c) && !squote && !dquote && !bsquote)
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
		argv[argc] = strdup(copybuf);
		if (argv[argc] == NULL) {
			kmem_free(argv);
			argv = NULL;
			break;
		}
		argc++;
		argv[argc] = NULL;

		while (BLANK(*input))
			input++;
	} while (*input != '\0');

	kmem_free_s(copybuf);

	*arg_c = argc;
	*arg_v = argv;

	return 0;
}

int karg_find(int argc, char **argv, const char *opt, int fullmatch)
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

/* XXX: NUL as delimiters */
char **karg_build_nul(const char *ibuf, int ilen, int *arg_c, char ***arg_v)
{
	int argc = 0, maxargc = 0;
	char c, *arg, *copybuf, **argv = NULL, **kargv;
	const char *input = ibuf;

	*arg_c = argc;
	if (input == NULL)
		return NULL;

	copybuf = (char *)kmem_alloc(ilen, char);

	do {
		while (input - ibuf < ilen) {
			c = *input;
			if (ISNUL(c))
				input++;
			else
				break;
		}

		if ((maxargc == 0) || (argc >= (maxargc - 1))) {
			if (argv == NULL)
				maxargc = 16;
			else
				maxargc *= 2;

			kargv = (char **)kmem_realloc(argv,
					maxargc * sizeof(char*));
			if (kargv == NULL) {
				if (argv != NULL) {
					karg_free(argc, argv);
					argv = NULL;
				}
				break;
			}
			argv = kargv;
			argv[argc] = NULL;
		}

		arg = copybuf;
		while (input - ibuf < ilen) {
			c = *input;
			if (ISNUL(c))
				break;

			*arg++ = c;
			input++;
		}

		*arg = '\0';
		argv[argc] = kstr_dup(copybuf);
		if (argv[argc] == NULL) {
			karg_free(argc, argv);
			argv = NULL;
			break;
		}
		argc++;
		argv[argc] = NULL;

		while (input - ibuf < ilen) {
			c = *input;
			if (ISNUL(c))
				input++;
			else
				break;
		}
	} while (input - ibuf < ilen);

	kmem_free_s(copybuf);

	*arg_c = argc;
	*arg_v = argv;

	return argv;
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
		printf ("karg_build(\"%s\")\n", *test);

		if ((argv = karg_build(*test, &arg_c, &arg_v)) == NULL)
			printf ("failed!\n\n");
		else {
			for (i = 0; i < arg_c; i++)
				printf("\t\"%s\"\n", arg_v[i]);
		}

		karg_free(argc, argv);
	}

	return 0;
}

#endif	/* MAIN */

