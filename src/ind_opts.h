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

#ifndef __IN_OPTIONS_H__
#define __IN_OPTIONS_H__

/*
 * in_option structure
 * -------------------
 *
 * This data structure is used to define each possible option.
 * Fields are :
 *      opt_tocken : If non zero, the value is returned by in_opts_next() when option encountered,
 *                   If zero, in_opts_next() does not return but affect the option variable and continue
 *                   with the next argument.
 *      opt_sname  : Short name (one letter)
 *      opt_lname  : Long name
 *      opt_lnasz  : Long name length
 *      opt_hasarg : Option argument indicator :
 *                   Possible values are :
 *                     - IN_OPTARG_NONE     : No arguments
 *                     - IN_OPTARG_REQUIRED : An argument is required
 *      opt_varptr : Integer variable address (NULL if none)
 *      opt_varval : Value operand for variable operation
 *      opt_varope : Operation ti apply on variable referenced by opt_varptr when operation encountered
 *                   Possible value are :
 *                     - IN_OPTARG_OPENONE : variable is not affected
 *                     - IN_OPTARG_OPESET  : variable ::= value
 *                     - IN_OPTARG_OPEADD  : variable ::= variable + value
 *                     - IN_OPTARG_OPESUB  : variable ::= variable - value
 *                     - IN_OPTARG_OPEAND  : variable ::= variable .AND. value
 *                     - IN_OPTARG_OPEOR   : variable ::= variable .OR.  value
 *                     - IN_OPTARG_OPEXOR  : variable ::= variable .XOR. value
 *	opt_info   : Option information for in_opts_usage()
 */
struct in_option {
	int	    opt_tocken;		/* Tocken (value returned by getnextoption */
	int         opt_sname;		/* Short option                            */
	const char *opt_lname;		/* Long option                             */
	size_t      opt_lnasz;		/* Long option size                        */
	int         opt_hasarg;		/* Has argument                            */
#define IN_OPTARG_NONE		0	/* Option has no arguments                 */
#define IN_OPTARG_REQUIRED	1	/* Option require an argument              */
	int        *opt_varptr;		/* Variable pointer (NULL if none)         */
	int         opt_varval;		/* Variable value                          */
	int         opt_varope;		/* Variable operation			   */
#define IN_OPTARG_OPENONE	0	/* No operation                            */
#define IN_OPTARG_OPESET	1	/* Variable <- value			   */
#define IN_OPTARG_OPEADD	2	/* Variable <- variable + value            */
#define IN_OPTARG_OPESUB	3	/* Variable <- variable - value            */
#define IN_OPTARG_OPEAND	4	/* Variable <- variable .AND. value        */
#define IN_OPTARG_OPEOR		5	/* Variable <- variable .OR.  value        */
#define IN_OPTARG_OPEXOR	6	/* Variable <- variable .XOR. value        */
#define IN_OPTARG_HELP	       90	/* Display help and exits                  */
#define IN_OPTARG_VERSION      91	/* Display version and exits               */
	const char *opt_info;		/* Option information (for usage())        */
};

#define IN_OPT_ENTRY(tocken, sname, lname, hasarg, varptr, varval, varope, info) \
  { tocken, sname, lname, sizeof(lname) - 1, hasarg, varptr, varval, varope, info }

extern       void  in_opts_prepare(const char *, const char *, const char *, const char *, const struct in_option *, size_t);
#define in_opts_prepare_old(a, b, c, d, e) in_opts_prepare(a, b, c, "", d, e)
extern       void  in_opts_usage(int status)         __attribute__((noreturn));
extern       void  in_opts_version(const char *)     __attribute__((noreturn));
extern       int   in_opts_next(int *, char ***, char **);
extern const char *in_opts_progname(void);

#endif /*! __IN_OPTIONS_H__*/
