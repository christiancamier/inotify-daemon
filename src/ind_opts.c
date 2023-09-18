/* 
 * Copyright: Christian CAMIER & Quentin PERIDON 2022
 * 
 * christian.c@promethee.services
 * 
 * This software is a computer program whose purpose is to manage filesystems
 * events on defined directories.
 * 
 * This software is governed by the CeCILL-B license under French law and
 * abiding by the rules of distribution of free software.  You can  use, 
 * modify and/ or redistribute the software under the terms of the CeCILL-B
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info". 
 * 
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability. 
 * 
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or 
 * data to be ensured and,  more generally, to use and operate it in the 
 * same conditions as regards security. 
 * 
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL-B license and that you accept its terms.
 */

#include "ind_config.h"

#include <sys/types.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "ind_opts.h"

/* Exported functions */
extern       void  in_opts_prepare(const char *, const char *, const char *, const char *, const struct in_option *, size_t);
extern       void  in_opts_usage(int status)     __attribute__((noreturn));
extern       void  in_opts_version(const char *) __attribute__((noreturn));
extern       int   in_opts_next(int *, char ***, char **);
extern const char *in_opts_progname(void);

/* Local functions */
static void usage_options(FILE *);
static void display_version(void) __attribute__((noreturn));


static const char             *program_name = (const char             *)NULL;
static const char             *program_vers = (const char             *)NULL;
static const char             *version_str  = (const char             *)NULL;
static const char             *usage_string = (const char             *)NULL;
static const struct in_option *program_opts = (const struct in_option *)NULL;
static size_t                  program_nopt = (size_t)0;

/*
 * NAME
 *	in_opts_prepare
 * SYNOPSIS
 *	#include <sys/types.h>
 *	#include <in_options.h>
 *	void in_opts_prepare(
 *		const char *prg_name,
 *		const char *prg_version,
 *		const char *prg_usage,
 *		const struct in_option *prg_options,
 *		size_t prg_numopts);
 *
 * DESCRIPTION
 *	This function, called at the beginning of the programm, initialise,
 *	all parametters for the in_options suite. The arguments are :
 *	prg_name:	Name of the program (usualy argv[0]). If name is a path,
 *			the basename part will be retrieved.
 *	prg_version:	Version of the programm
 *	prg_usage:	Usage string
 *	prg_options:	Array of the options for this programm
 *	prg_numopts:	Number of options in prg_options.
 *
 *	The usage string may contain some specifics sequences (a percent character
 *	followed by a letter) having special meaning. The allowed sequences are:
 *	. %O: The options with there own descriptions,
 *	. %P: The capatilized program name
 *	. %p: The program name
 *	. %V: The program version
 *	. %%: A percent character
 *		
 * RETURN VALUE
 *	None
 */
void in_opts_prepare(
	const char             *prg_name,
	const char             *prg_version,
	const char             *prg_usage,
	const char             *prg_version_str,
	const struct in_option *prg_options,
	size_t                  prg_numopts)
{
	register char *ptr;

	/* Store programm name */
	if((char *)NULL != (ptr = strrchr(prg_name, '/'))) ptr = ptr + 1;
	else                                               ptr = (char *)prg_name;
	program_name = (const char *)ptr;

	/* Then program version */
	program_vers = prg_version;

	/* Then program usage */
	usage_string = prg_usage;

	/* Then version string */
	version_str  = prg_version_str;

	/* Then options */
	program_opts = prg_options;
	program_nopt = prg_numopts;

	return;
}

/*
 * NAME
 *	in_opts_usage -- Display program usage and exit
 *
 * SYNOPSIS
 *	#include <sys/types.h>
 *	#include <in_options.h>
 *	void in_opts_usage(int status);
 *
 * DESCRIPTION
 *	Display the usage string and the exit with the given status.
 *	If status is not zero, outputs are processed thru the stderr
 *	stream.
 *	If status is zero, outputs are processed thru the stdout stream
 *
 * RETURN VALUE
 *	None
 */
__attribute__((noreturn)) void in_opts_usage(int status)
{
#define PUTS(S) for(nfoptr = S; *nfoptr; fputc(*(nfoptr++), stream))
	register FILE       *stream;
	register const char *usaptr;
	register const char *nfoptr;

	stream = status ? stderr : stdout;

	fputc('\n', stream);
	usaptr = usage_string;
	while(*usaptr)
	{
		if('%' == *usaptr)
		{
			switch(*(usaptr + 1))
			{
			case 'O':	/* Options */
				usaptr += 1;
				usage_options(stream);
				break;
			case 'P':	/* Capitalized program name */
				fputc(toupper(*program_name), stream);
				PUTS(program_name + 1);
				usaptr += 1;
				break;
			case 'p':	/* Program name */
				PUTS(program_name);
				usaptr += 1;
				break;
			case 'V':	/* Program version */
				PUTS("Ver. ");
				PUTS(program_vers);
				usaptr += 1;
				break;
			case '%':	/* A percent character */
				usaptr += 1;
				/* No break here */
			case '\0':	/* String terminaison */
			default:
				fputc('%', stream);
				break;
			}
		}
		else
		{
			fputc(*usaptr, stream);
		}
		usaptr += 1;
	}
	fputc('\n', stream);
	exit(status);
#undef PUTS
}

/*
 * NAME
 *	in_opts_version -- Display program version and exit
 *
 * SYNOPSIS
 *	#include <sys/types.h>
 *	#include <in_options.h>
 *	void in_opts_version(const char version_string);
 *
 * DESCRIPTION
 *	Display the given version string and exit with status 0.
 *
 *	The version string may contain some specifics sequences (a percent character
 *	followed by a letter) having special meaning. The allowed sequences are:
 *	. %P: The capatilized program name
 *	. %p: The program name
 *	. %V: The program version
 *	. %%: A percent character
 *
 * RETURN VALUE
 *	None
 */
__attribute__((noreturn)) void in_opts_version(const char *version_string)
{
#define PUTS(S) for(nfoptr = S; *nfoptr; fputc(*(nfoptr++), stdout))
	register const char             *verptr;
	register const char             *nfoptr;

	fputc('\n', stdout);
	verptr = version_string;
	while(*verptr)
	{
		if('%' == *verptr)
		{
			switch(*(verptr + 1))
			{
			case 'P':	/* Capitalized program name */
				fputc(toupper(*program_name), stdout);
				PUTS(program_name + 1);
				verptr += 1;
				break;
			case 'p':	/* Program name */
				PUTS(program_name);
				verptr += 1;
				break;
			case 'V':	/* Program version */
				PUTS("Ver. ");
				PUTS(program_vers);
				verptr += 1;
				break;
			case '%':	/* A percent character */
				verptr += 1;
				/* No break here */
			case '\0':	/* String terminaison */
			default:
				fputc('%', stdout);
				break;
			}
		}
		else
		{
			fputc(*verptr, stdout);
		}
		verptr += 1;
	}
	fputc('\n', stdout);
	fflush(stdout);
	exit(0);
#undef PUTS
}

/*
 * NAME
 *	in_opts_next -- Get next option
 *
 * SYNOPSIS
 *	#include <sys/types.h>
 *	#include <in_options.h>
 *	int in_opts_next(int *argc, char ***argv, char **carg);
 *
 * DESCRIPTION
 *	Parse the command-line arguments according to the options
 *	defined using the in_opts_prepare(3ccr) function.
 *
 * RETURN VALUE
 *	None
 */
int in_opts_next(int *argc, char ***argv, char **carg)
{
	register int                      ac;
	register char                   **av;
	register char                    *pa;
	register const struct in_option  *op;
	register size_t                   oc;

	if(0 >= (ac = *argc))
		return 0;

	do
	{
		av = *argv;
		pa = *av;

		if(*pa != '-')
			return 0;

		pa += 1;
		if(*pa == '-')
			goto longopt;

		for(op = program_opts, oc = 0; oc < program_nopt; oc += 1, op += 1)
			if(op->opt_sname == (int)*pa)
				goto shortfnd;
		fprintf(stderr, "Unknown option -%c\n", *pa);
		in_opts_usage(200);
	shortfnd:
		pa += 1;
		if(op->opt_hasarg)
		{
			if(*pa == '\0')
			{
				if(ac < 2)
					goto missarg;
				ac -= 2, *carg = *(++av), av += 1;
				goto optret2;
			}
			*carg=pa;
			goto optret1;
		}
		if(*pa != '\0')
		{
			*(pa - 1) = '-', *av = pa - 1;
			goto optret2;
		}
		goto optret1;
	longopt:
		pa += 1;
		if(*pa == '\0')			/* '--' case */
		{
			*argc = ac - 1;
			*argv = av + 1;
			return 0;
		}

		for(op = program_opts, oc = 0; oc < program_nopt; oc += 1, op += 1)
			if(0 == strncmp(op->opt_lname, pa, op->opt_lnasz))
			{
				int c;
				c = *(pa + op->opt_lnasz);
				if(c == '\0' || c == '=')
					goto longfnd;
			}
		fprintf(stderr, "Unknown option --%s\n", pa);
		in_opts_usage(200);
	longfnd:
		pa += op->opt_lnasz;
		if(op->opt_hasarg)
		{
			if(*pa != '=')
				goto missarg;
			*carg = (pa + 1);
			goto optret1;
		}
		if(*pa != '=')
			goto optret1;
		fprintf(stderr, "Misformed option `--%s'\n", op->opt_lname);
		in_opts_usage(202);
	missarg:
		fprintf(stderr, "Option `-%c, --%s' argument missing\n", op->opt_sname, op->opt_lname);
		in_opts_usage(201);
	optret1:
		if(IN_OPTARG_HELP    == op->opt_varope) in_opts_usage(0);
		if(IN_OPTARG_VERSION == op->opt_varope) display_version();
		ac -= 1;
		av += 1;
	optret2:
		*argc = ac;
		*argv = av;
		if(op->opt_varptr != (int *)NULL)
			switch(op->opt_varope)
			{
			case IN_OPTARG_OPENONE:						break;
			case IN_OPTARG_OPESET:	*op->opt_varptr  = op->opt_varval;	break;
			case IN_OPTARG_OPEADD:	*op->opt_varptr += op->opt_varval;	break;
			case IN_OPTARG_OPESUB:	*op->opt_varptr -= op->opt_varval;	break;
			case IN_OPTARG_OPEAND:	*op->opt_varptr &= op->opt_varval;	break;
			case IN_OPTARG_OPEOR:	*op->opt_varptr |= op->opt_varval;	break;
			case IN_OPTARG_OPEXOR:	*op->opt_varptr ^= op->opt_varval;	break;
			default:							break;
			}
	}
	while(0 == op->opt_tocken && 0 != ac);
	return op->opt_tocken;
}

const char *
in_opts_progname(void)
{
	return program_name;
}

static __attribute__((noreturn)) void display_version()
{
	in_opts_version(version_str);
}

static void usage_options(FILE *stream)
{
	register const struct in_option *optptr;
	register size_t                  optcnt;

	fflush(stream);
	for(optcnt = 0, optptr = program_opts; optcnt < program_nopt; optcnt += 1, optptr += 1)
		fprintf(
			stream,
			" -%c, --%s%s\n\t%s\n",
			optptr->opt_sname,
			optptr->opt_lname,
			optptr->opt_hasarg ? "=xxx" : "",
			optptr->opt_info
			);
	fflush(stream);
	return;
}
