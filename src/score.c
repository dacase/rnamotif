#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "rnamot.h"
#include "y.tab.h"

#define	MIN(a,b)	((a)<(b)?(a):(b))

#define	ISLVAL(s)	\
	((s)==SYM_ASSIGN||(s)==SYM_PLUS_ASSIGN||(s)==SYM_MINUS_ASSIGN||\
	 (s)==SYM_PERCENT_ASSIGN||(s)==SYM_SLASH_ASSIGN||(s)==SYM_STAR_ASSIGN||\
	 (s)==SYM_PLUS_PLUS||(s)==SYM_MINUS_MINUS)

#define	ISRFLX(s)	\
	((s)==SYM_PLUS_ASSIGN||(s)==SYM_MINUS_ASSIGN||\
	 (s)==SYM_PERCENT_ASSIGN||(s)==SYM_SLASH_ASSIGN||(s)==SYM_STAR_ASSIGN)

extern	int	rm_lineno;
extern	int	rm_emsg_lineno;	
extern	STREL_T	rm_descr[];
extern	int	rm_n_descr;
IDENT_T	*RM_find_id();
IDENT_T	*RM_enter_id();
NODE_T	*RM_node();

#define	LABTAB_SIZE	1000
static	int	labtab[ LABTAB_SIZE ];
static	int	nextlab;
static	int	actlab;
static	VALUE_T	v_lab;

static	int	ifstk[ 100 ];
static	int	ifstkp = 0;

static	int	loopstk[ 100 ];
static	NODE_T	*loopincrstk[ 100 ];
static	int	loopstkp = 0;

static	char	*sc_sbuf;

#define	OP_NOOP		0	/* No Op	 	*/
#define	OP_ACPT		1	/* Accept the candidate	*/
#define	OP_RJCT		2	/* Reject the candidate */
#define	OP_MRK		3	/* Mark the stack	*/
#define	OP_CLS		4	/* Clear the stack	*/
#define	OP_FCL		5	/* Function Call	*/
#define	OP_SCL		6	/* Builtin  Call	*/
#define	OP_STRF		7	/* Str. El. Reference	*/
#define	OP_LDA		8	/* Load Address		*/
#define	OP_LOD		9	/* Load Value		*/
#define	OP_LDC		10	/* Load Constant	*/
#define	OP_STO		11	/* Store top of stack	*/
#define	OP_AND		12	/* McCarthy And		*/
#define	OP_IOR		13	/* McCarthy Or		*/
#define	OP_NOT		14	/* Not			*/
#define	OP_MAT		15	/* Match		*/
#define	OP_INS		16	/* In Pairset		*/
#define	OP_GTR		17	/* Greater Than		*/
#define	OP_GEQ		18	/* Greater or Equal	*/
#define	OP_EQU		19	/* Equal		*/
#define	OP_NEQ		20	/* Not Equal		*/
#define	OP_LEQ		21	/* Less or Equal	*/
#define	OP_LES		22	/* Less Than		*/
#define	OP_ADD		23	/* Addition		*/
#define	OP_SUB		24	/* Subtraction		*/
#define	OP_MUL		25	/* Multiplication	*/
#define	OP_DIV		26	/* Division		*/
#define	OP_MOD		27	/* Modulus		*/
#define	OP_NEG		28	/* Negate		*/
#define	OP_PRST		29	/* Make a pairset	*/
#define	OP_BPR		30	/* Make a pair		*/
#define	OP_I_PP		31	/* use then incr (i++)	*/
#define	OP_PP_I		32	/* incr then use (++i)	*/
#define	OP_I_MM		33	/* use then decr (i--)	*/
#define	OP_MM_I		34	/* decr then use (--i)	*/
#define	OP_FJP		35	/* False Jump		*/
#define	OP_JMP		36	/* Jump			*/
#define	N_OP		37

static	char	*opnames[ N_OP ] = {
	"noop",
	"acpt",
	"rjct",
	"mrk",
	"cls",
	"fcl",
	"scl",
	"strf",
	"lda",
	"lod",
	"ldc",
	"sto",
	"and",
	"ior",
	"not",
	"mat",
	"ins",
	"gtr",
	"geq",
	"equ",
	"neq",
	"leq",
	"les",
	"add",
	"sub",
	"mul",
	"div",
	"mod",
	"neg",
	"prst",
	"bpr",
	"incp",
	"pinc",
	"decp",
	"pdec",
	"fjp",
	"jmp" 
};

#define	SC_LENGTH	0
#define	SC_MISMATCHES	1
#define	SC_MISPAIRS	2
#define	SC_PAIRED	3
#define	SC_STRID	4	
#define	N_SC		5	

static	char	*scnames[ N_SC ] = {
	"length",
	"mismatches",
	"mispairs",
	"paired",
	"STRID"
};

typedef	struct	inst_t	{
	int	i_op;
	VALUE_T	i_val;
} INST_T;

#define	PROG_SIZE	10000
static	INST_T	prog[ PROG_SIZE ];
static	int	l_prog;

#define	MEM_SIZE	10000
static	VALUE_T	mem[ MEM_SIZE ];

static	int	pc;		/* program counter	*/
static	int	mp;		/* mark pointer		*/ 
static	int	sp;		/* stack pointer	*/

static	char	emsg[ 256 ];

void	RM_if();
void	RM_else();
void	RM_endelse();
void	RM_endif();
void	RM_forinit();
void	RM_fortest();
void	RM_forincr();
void	RM_endfor();
void	RM_while();
void	RM_endwhile();
void	RM_break();
void	RM_continue();
void	RM_accept();
void	RM_reject();
void	RM_mark();
void	RM_clear();
void	RM_expr();
void	RM_linkscore();
void	RM_dumpscore();
int	RM_score();

static	void	fixexpr();
static	void	genexpr();
static	int	is_syscall();
static	void	fix_stref();
static	NODE_T	*mk_call_strid();
static	void	fix_call();
static	void	do_fcl();
static	void	do_scl();
static	int	paired();
static	void	do_strf();
static	void	do_lda();
static	void	do_lod();
static	void	do_ldc();
static	void	do_sto();
static	void	do_and();
static	void	do_ior();
static	void	do_not();
static	void	do_mat();
static	void	do_ins();
static	void	do_gtr();
static	void	do_geq();
static	void	do_equ();
static	void	do_neq();
static	void	do_leq();
static	void	do_les();
static	void	do_add();
static	void	do_sub();
static	void	do_mul();
static	void	do_div();
static	void	do_mod();
static	void	do_neg();
static	void	do_prst();
static	void	do_bpr();
static	void	do_i_pp();
static	void	do_pp_i();
static	void	do_i_mm();
static	void	do_mm_i();

static	void	mk_stref_name();
static	void	addnode();
static	void	addinst();
static	void	dumpinst();
static	void	dumpstk();

void	RM_action( np )
NODE_T	*np;
{

	RM_mark();
	RM_expr( 0, np );
	actlab = nextlab;
	nextlab++;
	v_lab.v_type = T_INT;
	v_lab.v_value.v_ival = actlab;
	addinst( OP_FJP, &v_lab );
}

void	RM_endaction()
{

	labtab[ actlab ] = l_prog;
}

void	RM_if( np )
NODE_T	*np;
{

	RM_mark();
	RM_expr( 0, np );
	ifstk[ ifstkp ] = nextlab;
	ifstkp++;
	nextlab += 2;
	v_lab.v_type = T_INT;
	v_lab.v_value.v_ival = ifstk[ ifstkp - 1 ];
	addinst( OP_FJP, &v_lab );
}

void	RM_else()
{

	v_lab.v_type = T_INT;
	v_lab.v_value.v_ival = ifstk[ ifstkp - 1 ];
	addinst( OP_JMP, &v_lab );
	labtab[ ifstk[ ifstkp - 1 ] ] = l_prog;
}

void	RM_endelse()
{

	labtab[ ifstk[ ifstkp - 1 ] + 1 ] = l_prog;
	ifstkp--;
}

void	RM_endif()
{

	labtab[ ifstk[ ifstkp - 1 ] ] = l_prog;
	ifstkp--;
}

void	RM_forinit( np )
NODE_T	*np;
{

	loopstk[ loopstkp ] = nextlab;
	loopstkp++;
	nextlab += 3;
	RM_mark();
	RM_expr( 0, np );
	RM_clear();
}

void	RM_fortest( np )
NODE_T	*np;
{

	labtab[ loopstk[ loopstkp - 1 ] ] = l_prog;
	RM_mark();
	RM_expr( 0, np );
	v_lab.v_type = T_INT;
	v_lab.v_value.v_ival = loopstk[ loopstkp - 1 ] + 2;
	addinst( OP_FJP, &v_lab );
}

void	RM_forincr( np )
NODE_T	*np;
{

	loopincrstk[ loopstkp - 1 ] = np;
}

void	RM_endfor()
{

	labtab[ loopstk[ loopstkp - 1 ] + 1 ] = l_prog;
	RM_mark();
	RM_expr( 0, loopincrstk[ loopstkp - 1 ] );
	RM_clear();
	v_lab.v_type = T_INT;
	v_lab.v_value.v_ival = loopstk[ loopstkp - 1 ];
	addinst( OP_JMP, &v_lab );
	labtab[ loopstk[ loopstkp - 1 ] + 2 ] = l_prog;
	loopstkp--;
}

void	RM_while( np )
NODE_T	*np;
{

	loopstk[ loopstkp ] = nextlab;
	loopstkp++;
	nextlab += 3;
	labtab[ loopstk[ loopstkp -1 ] ] = l_prog;
	RM_mark();
	RM_expr( 0, np );
	v_lab.v_type = T_INT;
	v_lab.v_value.v_ival = loopstk[ loopstkp - 1 ] + 2;
	addinst( OP_FJP, &v_lab );
}

void	RM_endwhile()
{

	v_lab.v_type = T_INT;
	v_lab.v_value.v_ival = loopstk[ loopstkp - 1 ];
	addinst( OP_JMP, &v_lab );
	labtab[ loopstk[ loopstkp - 1 ] + 2 ] = l_prog;
	loopstkp--;
}

void	RM_break()
{

	v_lab.v_type = T_INT;
	v_lab.v_value.v_ival = loopstk[ loopstkp - 1 ] + 1;
	addinst( OP_JMP, &v_lab );
}

void	RM_continue()
{

	v_lab.v_type = T_INT;
	v_lab.v_value.v_ival = loopstk[ loopstkp - 1 ];
	addinst( OP_JMP, &v_lab );
}

void	RM_accept()
{

	addinst( OP_ACPT, NULL );
}

void	RM_reject()
{

	addinst( OP_RJCT, NULL );
}

void	RM_mark()
{

	addinst( OP_MRK, NULL );
}

void	RM_clear()
{

	addinst( OP_CLS, 0 );
}

void	RM_expr( lval, np )
int	lval;
NODE_T	*np;
{
	fixexpr( np );
	genexpr( lval, np );
}

void	RM_linkscore()
{
	int	i;
	INST_T	*ip;

	for( ip = prog, i = 0; i < l_prog; i++, ip++ ){
		if( ip->i_op == OP_FJP || ip->i_op == OP_JMP ){
			ip->i_val.v_value.v_ival =
				labtab[ ip->i_val.v_value.v_ival ];
		}else if( ip->i_op == OP_IOR || ip->i_op == OP_AND ){
			ip->i_val.v_value.v_ival =
				labtab[ ip->i_val.v_value.v_ival ];
		}
	}
}

void	RM_dumpscore( fp )
FILE	*fp;
{
	INST_T	*ip;
	int	i;

	fprintf( fp, "SCORE: %4d inst.\n", l_prog );
	for( ip = prog, i = 0; i < l_prog; i++, ip++ ){
		dumpinst( fp, i, ip );
	}
}

int	RM_score( sbuf )
char	sbuf[];
{
	INST_T	*ip;
	
	if( l_prog <= 0 )
		return( 1 );

	sc_sbuf = sbuf;
	sp = mp = -1;
	for( pc = 0; ; ){
		if( pc < 0 || pc >= l_prog ){
			sprintf( emsg, "RM_score: pc: %d: out of range 0..%d.",
				pc, l_prog - 1 );
			RM_errormsg( 1, emsg );
		}
		ip = &prog[ pc ];

/*
fprintf( stdout, "RM_run, pc = %4d, op = %s\n", pc, opnames[ ip->i_op ] );
dumpstk( stdout, "before op" );
*/

		pc++;
		switch( ip->i_op ){
		case OP_NOOP :
			break;

		case OP_ACPT :
			return( 1 );
			break;
		case OP_RJCT :
			return( 0 );
			break;

		case OP_MRK :
			sp++;
			mem[ sp ].v_type = T_INT;
			mem[ sp ].v_value.v_ival = mp;
			mp = sp;
			break;
		case OP_CLS :
			sp = mp = -1;
			break;

		case OP_FCL :
			do_fcl( ip );
			break;
		case OP_SCL :
			do_scl( ip );
			break;
		case OP_STRF :
			do_strf( ip );
			break;

		case OP_LDA :
			do_lda( ip );
			break;
		case OP_LOD :
			do_lod( ip );
			break;
		case OP_LDC :
			do_ldc( ip );
			break;
		case OP_STO :
			do_sto();
			break;

		case OP_AND :
			do_and( ip );
			break;
		case OP_IOR :
			do_ior( ip );
			break;
		case OP_NOT :
			do_not();
			break;

		case OP_MAT :
			do_mat();
			break;
		case OP_INS :
			do_ins();
			break;
		case OP_GTR :
			do_gtr();
			break;
		case OP_GEQ :
			do_geq();
			break;
		case OP_EQU :
			do_equ();
			break;
		case OP_NEQ :
			do_neq();
			break;
		case OP_LEQ :
			do_leq();
			break;
		case OP_LES :
			do_les();
			break;

		case OP_ADD :
			do_add();
			break;
		case OP_SUB :
			do_sub();
			break;
		case OP_MUL :
			do_mul();
			break;
		case OP_DIV :
			do_div();
			break;
		case OP_MOD :
			do_mod();
			break;
		case OP_NEG :
			do_neg();
			break;

		case OP_PRST :
			do_prst();
			break;
		case OP_BPR :
			do_bpr();
			break;

		case OP_I_PP :
			do_i_pp();
			break;
		case OP_PP_I :
			do_pp_i();
			break;
		case OP_I_MM :
			do_i_mm();
			break;
		case OP_MM_I :
			do_mm_i();
			break;

		case OP_FJP :
			if( !mem[ sp ].v_value.v_ival )
				pc = ip->i_val.v_value.v_ival; 
			sp = mp = -1;
			break;
		case OP_JMP :
			pc = ip->i_val.v_value.v_ival;
			break;
		default :
			sprintf( emsg, "RM_score: unknown op %d.", ip->i_op );
			RM_errormsg( 1, emsg );
			break;
		}
/*
dumpstk( stdout, "after op " );
*/
	}
}

static	void	fixexpr( np )
NODE_T	*np;
{
	int	sc;

	if( np ){
		fixexpr( np->n_left );
		fixexpr( np->n_right );
		if( np->n_sym == SYM_STREF ){
			fix_stref( np );
		}else if( np->n_sym == SYM_CALL ){
			fix_call( np );
		}
	}
}

static	void	genexpr( lval, np )
int	lval;
NODE_T	*np;
{
	char	name[ 20 ];
	int	l_andor;
	VALUE_T	v_expr;

	if( np ){
		if( np->n_sym == SYM_CALL ){
			addinst( OP_MRK, NULL );
		}else if( np->n_sym == SYM_LCURLY ){
			addinst( OP_MRK, NULL );
		}

		genexpr( ISLVAL( np->n_sym ), np->n_left );

		if( ISRFLX( np->n_sym ) )
			genexpr( 0, np->n_left );

		if( np->n_sym == SYM_OR || np->n_sym == SYM_AND ){
			l_andor = nextlab;
			nextlab++;
			addnode( lval, np, l_andor );
			genexpr( 0, np->n_right );
			labtab[ l_andor ] = l_prog;
		}else{
			genexpr( 0, np->n_right );
			addnode( lval, np, 0 );
		}

		if( np->n_sym == SYM_STREF ){
			addinst( OP_STRF, NULL );
		}else if( np->n_sym == SYM_LCURLY ){
			addinst( OP_PRST, NULL );
		}
	}
}

static	int	is_syscall( np )
NODE_T	*np;
{
	int	i;
	char	*sp;

	sp = np->n_val.v_value.v_pval;
	for( i = 0; i < N_SC; i++ ){
		if( !strcmp( sp, scnames[ i ] ) )
			return( i );
	}
	return( UNDEF );
}

static	void	fix_stref( np )
NODE_T	*np;
{
	NODE_T	*np1, *np2, *np3;
	NODE_T	*n_index, *n_tag, *n_pos, *n_len;
	char	*ip, *sp;
	VALUE_T	v_expr;

	n_index = n_tag = n_pos = n_len = NULL;
	for( np1 = np->n_right; np1; np1 = np1->n_right ){
		np2 = np1->n_left;
		np3 = np2->n_left;
		if( np3->n_sym == SYM_IDENT ){
			ip = np3->n_val.v_value.v_pval;
			if( !strcmp( ip, "index" ) ){
				if( n_index != NULL ){
					rm_emsg_lineno = n_index->n_lineno;
					RM_errormsg( 1,
		"fix_stref: index parameter may not appear more than once." );
				}else
					n_index = np2->n_right;
			}else if( !strcmp( ip, "tag" ) ){
				if( n_tag != NULL ){
					rm_emsg_lineno = n_tag->n_lineno;
					RM_errormsg( 1,
		"fix_stref: tag parameter may not appear more than once." );
				}else
					n_tag = np2->n_right;
			}else if( !strcmp( ip, "pos" ) ){
				if( n_pos != NULL ){
					rm_emsg_lineno = n_pos->n_lineno;
					RM_errormsg( 1,
		"fix_stref: pos parameter may not appear more than once." );
				}else
					n_pos = np2->n_right;
			}else if( !strcmp( ip, "len" ) ){
				if( n_len != NULL ){
					rm_emsg_lineno = n_len->n_lineno;
					RM_errormsg( 1,
		"fix_stref: len parameter may not appear more than once." );
				}else
					n_len = np2->n_right;
			}else{
				rm_emsg_lineno = np3->n_lineno;
				sprintf( emsg,
					"fix_stref: unknown parameter: '%s'.",
					ip );
				RM_errormsg( 1, emsg );
			}
		}
	}

	rm_emsg_lineno = np->n_lineno;
	if( n_index == NULL && n_tag == NULL ){
		RM_errormsg( 1,
			"fix_stref: index = or tag = require for stref()." );
	}

	np1 = mk_call_strid( n_tag, n_index, np->n_left->n_sym );

	/* build the 3 parms to stref	*/
	if( n_len == NULL ){
		v_expr.v_type = T_INT;
		v_expr.v_value.v_ival = -1;
		np3 = RM_node( SYM_INT, &v_expr, 0, 0 );
	}else
		np3 = n_len;
	np2 = RM_node( SYM_LIST, 0, np3, NULL );
	if( n_pos == NULL ){
		v_expr.v_type = T_INT;
		v_expr.v_value.v_ival = 1;
		np3 = RM_node( SYM_INT, &v_expr, 0, 0 );
	}else
		np3 = n_pos;
	np2 = RM_node( SYM_LIST, 0, np3, np2 );
	np1 = RM_node( SYM_LIST, 0, np1, np2 );
	np->n_right = np1;
}

static	NODE_T	*mk_call_strid( n_tag, n_index, strel )
NODE_T	*n_tag;
NODE_T	*n_index;
int	strel;
{
	NODE_T	*np1, *np2, *np3;
	VALUE_T	v_expr;
	int	k_tag, k_index;
	char	*v_tag;
	int	v_index;
	int	d, d_tag, d_index;
	STREL_T	*stp;

	k_tag = 0;
	v_tag = NULL;
	d_tag = UNDEF;
	k_index = 0;
	v_index = UNDEF;
	d_index = UNDEF;

	if( n_tag != NULL ){
		if( n_tag->n_sym == SYM_STRING ){
			k_tag = 1;
			v_tag = n_tag->n_val.v_value.v_pval;
		}
	}else{
		k_tag = 1;
		v_tag = NULL;
	}

	if( n_index != NULL ){
		if( n_index->n_sym == SYM_INT ){
			k_index = 1;
			v_index = n_index->n_val.v_value.v_ival;
		}
	}else{
		k_index = 1;
		v_index = UNDEF;
	}

	if( k_tag && k_index ){
		if( v_tag != NULL ){
			stp = rm_descr;
			for( d = 0; d < rm_n_descr; d++, stp++ ){
				if( stp->s_type == strel ){
					if( !strcmp( stp->s_tag, v_tag ) ){
						d_tag = d;
						break;
					}
				}
			}
			rm_emsg_lineno = n_tag->n_lineno;
			sprintf( emsg,
				"mk_call_strid: no such tag: '%s'.", v_tag );
			RM_errormsg( 1, emsg );
		}
		if( v_index != UNDEF ){
			rm_emsg_lineno = n_index->n_lineno;
			if( v_index < 0 || v_index >= rm_n_descr ){
				sprintf( emsg,
			"mk_call_strid: index must be between 1 and %d.",
					rm_n_descr );
				RM_errormsg( 1, emsg );
			}else
				d_index = v_index;
			if( rm_descr[ d_index - 1 ].s_type != strel ){
				sprintf( emsg,
		"mk_call_strid: strel with index= %d has wrong type.",
					v_index );
				RM_errormsg( 1, emsg );
			}
		}
		if( d_tag == UNDEF && d_index == UNDEF ){
			rm_emsg_lineno = n_tag->n_lineno;
			RM_errormsg( 1,
		"mk_call_strid: tag and index both have invalid values." );
		}else if( d_tag != UNDEF && d_index == UNDEF ){
			v_expr.v_type = T_INT;
			v_expr.v_value.v_ival = d_tag;
			np1 = RM_node( SYM_INT, &v_expr, 0, 0 );
			return( np1 );
		}else if( d_tag == UNDEF && d_index != UNDEF ){
			v_expr.v_type = T_INT;
			v_expr.v_value.v_ival = d_index - 1;
			np1 = RM_node( SYM_INT, &v_expr, 0, 0 );
			return( np1 );
		}else if( d_tag == d_index - 1 ){
			v_expr.v_type = T_INT;
			v_expr.v_value.v_ival = d_index;
			np1 = RM_node( SYM_INT, &v_expr, 0, 0 );
			return( np1 );
		}else{
			rm_emsg_lineno = n_tag->n_lineno;
			RM_errormsg( 1,
		"mk_call_strid: tag and index values are inconsistant." );
		}
	}

	if( n_tag == NULL ){
		v_expr.v_type = T_STRING;
		v_expr.v_value.v_pval = "";
		n_tag = RM_node( SYM_STRING, &v_expr, 0, 0 );
	}
	np2 = RM_node( SYM_LIST, 0, n_tag, NULL );

	if( n_index == NULL ){
		v_expr.v_type = T_INT;
		v_expr.v_value.v_ival = UNDEF;
		n_index = RM_node( SYM_INT, &v_expr, 0, 0 );
	}
		np3 = n_index;
	np2 = RM_node( SYM_LIST, 0, n_index, np2 );

	v_expr.v_type = T_INT;
	v_expr.v_value.v_ival = strel;
	np3 = RM_node( SYM_INT, &v_expr, 0, 0 );
	np2 = RM_node( SYM_LIST, 0, np3, np2 );

	v_expr.v_type = T_STRING;
	v_expr.v_value.v_pval = "STRID";
	np3 = RM_node( SYM_IDENT, &v_expr, 0, 0 );
	np1 = RM_node( SYM_CALL, 0, np3, np2 );
	return( np1 );
}

static	void	fix_call( np )
NODE_T	*np;
{
	int	sc, pcnt;
	NODE_T	*np1;

	sc = is_syscall( np );
	switch( sc ){
	case SC_LENGTH :
		break;
	case SC_MISMATCHES :
	case SC_MISPAIRS :
	case SC_PAIRED :
		for( pcnt = 0, np1 = np->n_right; np1; np1 = np1->n_right )
			pcnt++;
		if( pcnt != 1 ){
			rm_emsg_lineno = np->n_lineno;
			sprintf( emsg,
				"fix_call: function '%s' has only 1 parameter.",
				 scnames[ sc ] );
			RM_errormsg( 1, emsg );
		}
		np1 = np->n_right;
		np1 = np1->n_left;
		np1 = np1->n_right;
		np->n_right = np1;
		break;
	case SC_STRID :
		rm_emsg_lineno = np->n_lineno;
		RM_errormsg( 1, "fix_call: STRID can not be called by user." );
		break;
	default :
		rm_emsg_lineno = np->n_lineno;
		RM_errormsg( 1, "fix_call: unknown syscall." );
		break;
	}
}

static	void	do_fcl( ip )
INST_T	*ip;
{
	char	*cp;
	int	len;

	if( !strcmp( ip->i_val.v_value.v_pval, "length" ) ){
		cp = mem[ sp ].v_value.v_pval;
		len = strlen( cp );
		free( cp );
		mem[ mp ].v_type = T_INT;
		mem[ mp ].v_value.v_ival = len;
		sp = mp;
		mp = mem[ mp ].v_value.v_ival;
	}
}

static	void	do_scl( ip )
INST_T	*ip;
{
	char	*cp;
	int	idx, pos, len;
	STREL_T	*stp;

	switch( ip->i_val.v_value.v_ival ){
	case SC_LENGTH :
		cp = mem[ sp ].v_value.v_pval;
		len = strlen( cp );
		free( cp );
		sp = mp;
		mp = mem[ mp ].v_value.v_ival;
		mem[ sp ].v_type = T_INT;
		mem[ sp ].v_value.v_ival = len;
		break; 
	case SC_MISMATCHES :
		idx = mem[ sp - 2 ].v_value.v_ival;
		if( idx < 0 || idx >= rm_n_descr ){
			rm_emsg_lineno = UNDEF;
			sprintf( emsg, 
		"do_scl: mismatches: descr index must be between 1 and %d.",
				rm_n_descr );
			RM_errormsg( 1, emsg );
		}
		sp -= 2;
		stp = &rm_descr[ idx ];
		sp = mp;
		mp = mem[ mp ].v_value.v_ival;
		mem[ sp ].v_type = T_INT;
		mem[ sp ].v_value.v_ival = stp->s_n_mismatches;
		break;
	case SC_MISPAIRS :
		idx = mem[ sp - 2 ].v_value.v_ival;
		if( idx < 0 || idx >= rm_n_descr ){
			rm_emsg_lineno = UNDEF;
			sprintf( emsg, 
		"do_scl: mispairs: descr index must be between 1 and %d.",
				rm_n_descr );
			RM_errormsg( 1, emsg );
		}
		sp -= 2;
		stp = &rm_descr[ idx ];
		sp = mp;
		mp = mem[ mp ].v_value.v_ival;
		mem[ sp ].v_type = T_INT;
		mem[ sp ].v_value.v_ival = stp->s_n_mispairs;
		break;
	case SC_PAIRED :
		idx = mem[ sp - 2 ].v_value.v_ival;
		if( idx < 0 || idx >= rm_n_descr ){
			rm_emsg_lineno = UNDEF;
			sprintf( emsg, 
		"do_scl: paired: descr index must be between 1 and %d.",
				rm_n_descr );
			RM_errormsg( 1, emsg );
		}
		stp = &rm_descr[ idx ];
		
		pos = mem[ sp - 1 ].v_value.v_ival;
		if( pos < 1 || pos > stp->s_matchlen ){
			rm_emsg_lineno = UNDEF;
			sprintf( emsg, 
		"do_scl: paired: pos must be between 1 and %d.",
				stp->s_matchlen );
			RM_errormsg( 1, emsg );
		}
		pos--;

		len = mem[ sp ].v_value.v_ival;
		if( len == 0 ){
			rm_emsg_lineno = UNDEF;
			sprintf( emsg, "do_scl: paired: len must be > 0." );
			RM_errormsg( 1, emsg );
		}else if( len < 0 )
			len = stp->s_matchlen - pos;
		else
			len = MIN( stp->s_matchlen - pos, len );
		sp -= 2;
		sp = mp;
		mp = mem[ mp ].v_value.v_ival;
		mem[ sp ].v_type = T_INT;
		mem[ sp ].v_value.v_ival = paired( stp, pos, len );
		break;

	case SC_STRID :
		break;

	default :
		rm_emsg_lineno = UNDEF;
		RM_errormsg( 1, "do_scl: undefined syscall." );
		break;
	}
}

static	int	paired( stp, pos, len )
STREL_T	*stp;
int	pos;
int	len;
{
	STREL_T	*stp1, *stp2, *stp3, *stp4;
	int	i, mlen;
	int	p1, p2, p3, p4;
	int	b1, b2, b3, b4;

	mlen = stp->s_matchlen;
	switch( stp->s_n_mates ){
	case 1 : 
		if( stp->s_index < stp->s_mates[ 0 ]->s_index )
			stp1 = stp;
		else
			stp1 = stp->s_mates[ 0 ];
		stp2 = stp1->s_mates[ 0 ];
		p1 = stp1->s_matchoff;
		p2 = stp2->s_matchoff + mlen - 1;
		for( i = 0; i < len; i++ ){
			b1 = sc_sbuf[ p1 + pos + i ];
			b2 = sc_sbuf[ p2 - pos - i ];
			if( !RM_paired( stp1->s_pairset, b1, b2 ) )
				return( 0 );
		}
		return( 1 );
		break;
	case 2 :
		if( stp->s_index < stp->s_mates[ 0 ]->s_index )
			stp1 = stp;
		else
			stp1 = stp->s_mates[ 0 ];
		stp2 = stp1->s_mates[ 0 ];
		stp3 = stp1->s_mates[ 1 ];
		p1 = stp1->s_matchoff;
		p2 = stp2->s_matchoff + mlen - 1;
		p3 = stp3->s_matchoff;
		for( i = 0; i < len; i++ ){
			b1 = sc_sbuf[ p1 + pos + i ];
			b2 = sc_sbuf[ p2 - pos - i ];
			b3 = sc_sbuf[ p3 + pos + i ];
			if( !RM_triple( stp1->s_pairset, b1, b2, b3 ) )
				return( 0 );
		}
		return( 1 );
		break;
	case 3 :
		if( stp->s_index < stp->s_mates[ 0 ]->s_index )
			stp1 = stp;
		else
			stp1 = stp->s_mates[ 0 ];
		stp2 = stp1->s_mates[ 0 ];
		stp3 = stp1->s_mates[ 1 ];
		stp4 = stp1->s_mates[ 2 ];
		p1 = stp1->s_matchoff;
		p2 = stp2->s_matchoff + mlen - 1;
		p3 = stp3->s_matchoff;
		p4 = stp4->s_matchoff + mlen - 1;
		for( i = 0; i < len; i++ ){
			b1 = sc_sbuf[ p1 + pos + i ];
			b2 = sc_sbuf[ p2 - pos - i ];
			b3 = sc_sbuf[ p3 + pos + i ];
			b4 = sc_sbuf[ p3 - pos - i ];
			if( !RM_quad( stp1->s_pairset, b1, b2, b3, b4 ) )
				return( 0 );
		}
		return( 1 );
		break;
	default :
		rm_emsg_lineno = stp->s_lineno;
		RM_errormsg( 1, "paired() does not accept descr type 'ss'." );
		break;
	}
}

static	void	do_strf( ip )
INST_T	*ip;
{
	int	index;
	int	pos;
	int	len;
	STREL_T	*stp;
	char	*cp;

	len   = mem[ sp     ].v_value.v_ival;
	pos   = mem[ sp - 1 ].v_value.v_ival;
	index = mem[ sp - 2 ].v_value.v_ival;
	if( index < 0 || index >= rm_n_descr ){
		rm_emsg_lineno = UNDEF;
		sprintf( emsg, "do_strf: no such descriptor %d.", index );
		RM_errormsg( 1, emsg );
	}
	stp = &rm_descr[ index ];
	if( pos < 1 ){
		rm_emsg_lineno = UNDEF;
		RM_errormsg( 1, "do_strf: pos must be > 0." );
	}else if( pos > stp->s_matchlen ){
		rm_emsg_lineno = UNDEF;
		sprintf( emsg, "do_strf: pos must be <= %d.\n",
			stp->s_matchlen );
		RM_errormsg( 1, emsg );
	}
	pos--;
	if( len == 0 ){ 
		rm_emsg_lineno = UNDEF;
		RM_errormsg( 1, "do_strf: len must be > 0." );
	}else if( len == UNDEF )	/* rest of string */
		len = stp->s_matchlen - pos;
	else
		len = MIN( stp->s_matchlen - pos, len );
	cp = ( char * )malloc( ( len + 1 ) * sizeof( char ) );
	if( cp == NULL ){
		rm_emsg_lineno = UNDEF;
		RM_errormsg( 1, "do_strf: can't allocate cp." );
		exit( 1 );
	}
	strncpy( cp, &sc_sbuf[ stp->s_matchoff + pos ], len );
	cp[ len ] = '\0';
	sp -= 2; 
	mem[ sp ].v_type = T_STRING;
	mem[ sp ].v_value.v_pval = cp; 
}

static	void	do_lda( ip )
INST_T	*ip;
{
	VALUE_T	*v_top;
	IDENT_T	*idp;

	sp++;
	v_top = &mem[ sp ];
	idp = RM_find_id( ip->i_val.v_value.v_pval );
	if( idp == NULL ){
		idp = RM_enter_id( ip->i_val.v_value.v_pval, T_UNDEF, C_VAR,
			S_GLOBAL, 1, NULL );
		v_top->v_type = T_UNDEF;
		v_top->v_value.v_pval = idp;
	}else if( !idp->i_reinit ){
		rm_emsg_lineno = UNDEF;
		sprintf( emsg, "do_lda: variable '%s' is readonly.", 
			idp->i_name );
		RM_errormsg( 1, emsg );
	}else{
		v_top->v_type = T_IDENT;
		v_top->v_value.v_pval = idp;
	}
}

static	void	do_lod( ip )
INST_T	*ip;
{
	VALUE_T	*v_top;
	IDENT_T	*idp;
	char	*cp;

	sp++;
	v_top = &mem[ sp ];
	idp = RM_find_id( ip->i_val.v_value.v_pval );
	if( idp == NULL ){
		rm_emsg_lineno = UNDEF;
		sprintf( emsg, "do_lod: variable '%s' is undefined.",
			idp->i_name );
		RM_errormsg( 1, emsg );
	}else{
		switch( idp->i_type ){
		case T_UNDEF :
			rm_emsg_lineno = UNDEF;
			sprintf( emsg,
				"do_lod: variable '%s' is undefined.",
				idp->i_name );
			RM_errormsg( 1, emsg );
			break;
		case T_INT :
			v_top->v_type = T_INT;
			v_top->v_value.v_ival = idp->i_val.v_value.v_ival;
			break;
		case T_FLOAT :
			v_top->v_type = T_FLOAT;
			v_top->v_value.v_fval = idp->i_val.v_value.v_fval;
			break;
		case T_STRING :
			cp = ( char * )
				malloc( strlen(idp->i_val.v_value.v_pval) + 1 );
			if( cp == NULL ){
				rm_emsg_lineno = UNDEF;
				RM_errormsg( 1, "do_lod: can't allocate cp." );
			}
			strcpy( cp, idp->i_val.v_value.v_pval );
			v_top->v_type = T_STRING;
			v_top->v_value.v_pval = cp;
			break;
		case T_PAIR :
			break;
		case T_POS :
			break;
		case T_IDENT :
			break;
		}
	}
}

static	void	do_ldc( ip )
INST_T	*ip;
{
	VALUE_T	*v_top;
	char	*cp;

	sp++;
	v_top = &mem[ sp ];
	switch( ip->i_val.v_type ){
	case T_INT :
		v_top->v_type = T_INT;
		v_top->v_value.v_ival = ip->i_val.v_value.v_ival;
		break;
	case T_FLOAT :
		v_top->v_type = T_FLOAT;
		v_top->v_value.v_fval = ip->i_val.v_value.v_fval;
		break;
	case T_STRING :
		v_top->v_type = T_STRING;
		cp = ( char * )malloc( strlen( ip->i_val.v_value.v_pval ) + 1 );
		if( cp == NULL ){
			rm_emsg_lineno = UNDEF;
			RM_errormsg( 1, "do_ldc: can't allocate cp." );
		}
		strcpy( cp, ip->i_val.v_value.v_pval );
		v_top->v_value.v_pval = cp;
		break;
	case	T_POS :
		v_top->v_type = T_POS;
		v_top->v_value.v_pval = NULL;
		break;
	default :
		rm_emsg_lineno = UNDEF;
		RM_errormsg( 1, "do_ldc: type mismatch." );
		break;
	}
}

static	void	do_sto()
{
	VALUE_T	*v_tm1, *v_top;
	int	t_tm1, t_top;
	IDENT_T	*idp;
	char	*cp;

	v_top = &mem[ sp ];
	t_top = v_top->v_type;
	sp--;
	v_tm1 = &mem[ sp ];
	idp = v_tm1->v_value.v_pval;
	t_tm1 = idp->i_type;
	
	switch( T_IJ( t_tm1, t_top ) ){
	case T_IJ( T_UNDEF, T_INT ):
		v_tm1->v_type = T_INT;
		idp->i_type = T_INT;
		idp->i_val.v_type = T_INT;
		idp->i_val.v_value.v_ival = v_top->v_value.v_ival;
		break;
	case T_IJ( T_UNDEF, T_FLOAT ):
		v_tm1->v_type = T_FLOAT;
		idp->i_type = T_INT;
		idp->i_val.v_type = T_INT;
		idp->i_val.v_value.v_ival = v_top->v_value.v_ival;
		break;
	case T_IJ( T_UNDEF, T_STRING ):
		cp = ( char * )malloc( strlen( v_top->v_value.v_pval ) + 1 );
		if( cp == NULL ){
			rm_emsg_lineno = UNDEF;
			RM_errormsg( 1, "do_sto: can't allocate new string." );
		}
		strcpy( cp, v_top->v_value.v_pval );
		idp->i_type = T_STRING;
		idp->i_val.v_type = T_STRING;
		idp->i_val.v_value.v_pval = cp;
		break;
	case T_IJ( T_UNDEF, T_PAIR ):
		break;
	case T_IJ( T_INT, T_INT ) :
		idp->i_val.v_value.v_ival = v_top->v_value.v_ival;
		break;
	case T_IJ( T_INT, T_FLOAT ) :
		idp->i_val.v_value.v_ival = v_top->v_value.v_fval;
		break;
	case T_IJ( T_INT, T_POS ) :
		break;
	case T_IJ( T_FLOAT, T_INT ) :
		idp->i_val.v_value.v_fval = v_top->v_value.v_ival;
		break;
	case T_IJ( T_FLOAT, T_FLOAT ) :
		idp->i_val.v_value.v_fval = v_top->v_value.v_fval;
		break;
	case T_IJ( T_STRING, T_STRING ) :
		cp = ( char * )malloc( strlen( v_top->v_value.v_pval ) + 1 );
		if( cp == NULL ){
			rm_emsg_lineno = UNDEF;
			RM_errormsg( 1, "do_sto: can't allocate new string." );
		}
		strcpy( cp, v_top->v_value.v_pval );
		free( idp->i_val.v_value.v_pval );
		idp->i_val.v_value.v_pval = cp;
		break;
	case T_IJ( T_PAIR, T_PAIR ) :
		break;
	default :
		rm_emsg_lineno = UNDEF;
		RM_errormsg( 1, "do_sto: type mismatch." );
		break;
	}
}

static	void	do_and( ip )
INST_T	*ip;
{
	VALUE_T	*v_top;
	int	t_top, rv;

	v_top = &mem[ sp ];
	t_top = v_top->v_type;

	switch( t_top ){
	case T_INT :
		rv = v_top->v_value.v_ival = v_top->v_value.v_ival != 0;
		break;
	case T_FLOAT :
		rv = v_top->v_value.v_ival = v_top->v_value.v_fval != 0.0;
		break;
	case T_STRING :
		rv = v_top->v_value.v_ival =
			*( char * )v_top->v_value.v_pval != '\0';
		break;
	default :
		rm_emsg_lineno = UNDEF;
		RM_errormsg( 1, "do_and: type mismatch." );
		break;
	}
	if( !rv )
		pc = ip->i_val.v_value.v_ival;
}

static	void	do_ior( ip )
INST_T	*ip;
{
	VALUE_T	*v_top;
	int	t_top, rv;

	v_top = &mem[ sp ];
	t_top = v_top->v_type;

	switch( t_top ){
	case T_INT :
		rv = v_top->v_value.v_ival = v_top->v_value.v_ival != 0;
		break;
	case T_FLOAT :
		rv = v_top->v_value.v_ival = v_top->v_value.v_fval != 0.0;
		break;
	case T_STRING :
		rv = v_top->v_value.v_ival =
			*( char * )v_top->v_value.v_pval != '\0';
		break;
	default :
		rm_emsg_lineno = UNDEF;
		RM_errormsg( 1, "do_ior: type mismatch." );
		break;
	}
	if( rv )
		pc = ip->i_val.v_value.v_ival;
}

static	void	do_not()
{
	VALUE_T	*v_top;
	int	t_top;

	v_top = &mem[ sp ];
	t_top = v_top->v_type;

	switch( t_top ){
	case T_INT :
		v_top->v_value.v_ival = !( v_top->v_value.v_ival != 0 );
		break;
	case T_FLOAT :
		v_top->v_value.v_ival = !( v_top->v_value.v_fval != 0.0 );
		break;
	case T_STRING :
		v_top->v_value.v_ival =
			!( *( char * )v_top->v_value.v_pval != '\0' );
		break;
	default :
		rm_emsg_lineno = UNDEF;
		RM_errormsg( 1, "do_not: type mismatch." );
		break;
	}
}

static	void	do_mat()
{

}

static	void	do_ins()
{

}

static	void	do_gtr()
{
	VALUE_T	*v_tm1, *v_top;
	int	t_tm1, t_top;
	char	*s_tm1, *s_top;

	v_top = &mem[ sp ];
	t_top = v_top->v_type;
	sp--;
	v_tm1 = &mem[ sp ];
	t_tm1 = v_tm1->v_type;
	v_tm1->v_type = T_INT;
	
	switch( T_IJ( t_tm1, t_top ) ){
	case T_IJ( T_INT, T_INT ) :
		v_tm1->v_value.v_ival =
			v_tm1->v_value.v_ival > v_top->v_value.v_ival;
		break;
	case T_IJ( T_INT, T_FLOAT ) :
		v_tm1->v_value.v_ival =
			v_tm1->v_value.v_ival > v_top->v_value.v_fval;
		break;
	case T_IJ( T_FLOAT, T_INT ) :
		v_tm1->v_value.v_ival =
			v_tm1->v_value.v_fval > v_top->v_value.v_ival;
		break;
	case T_IJ( T_FLOAT, T_FLOAT ) :
		v_tm1->v_value.v_ival =
			v_tm1->v_value.v_fval > v_top->v_value.v_fval;
		break;
	case T_IJ( T_STRING, T_STRING ) :
		s_tm1 = v_tm1->v_value.v_pval;
		s_top = v_top->v_value.v_pval;
		v_tm1->v_value.v_ival = strcmp( s_tm1, s_top ) > 0;
		free( s_top );
		free( s_tm1 );
		break;
	default :
		rm_emsg_lineno = UNDEF;
		RM_errormsg( 1, "do_gtr: type mismatch." );
		break;
	}
}

static	void	do_geq()
{
	VALUE_T	*v_tm1, *v_top;
	int	t_tm1, t_top;
	char	*s_tm1, *s_top;

	v_top = &mem[ sp ];
	t_top = v_top->v_type;
	sp--;
	v_tm1 = &mem[ sp ];
	t_tm1 = v_tm1->v_type;
	v_tm1->v_type = T_INT;
	
	switch( T_IJ( t_tm1, t_top ) ){
	case T_IJ( T_INT, T_INT ) :
		v_tm1->v_value.v_ival =
			v_tm1->v_value.v_ival >= v_top->v_value.v_ival;
		break;
	case T_IJ( T_INT, T_FLOAT ) :
		v_tm1->v_value.v_ival =
			v_tm1->v_value.v_ival >= v_top->v_value.v_fval;
		break;
	case T_IJ( T_FLOAT, T_INT ) :
		v_tm1->v_value.v_ival =
			v_tm1->v_value.v_fval >= v_top->v_value.v_ival;
		break;
	case T_IJ( T_FLOAT, T_FLOAT ) :
		v_tm1->v_value.v_ival =
			v_tm1->v_value.v_fval >= v_top->v_value.v_fval;
		break;
	case T_IJ( T_STRING, T_STRING ) :
		s_tm1 = v_tm1->v_value.v_pval;
		s_top = v_top->v_value.v_pval;
		v_tm1->v_value.v_ival = strcmp( s_tm1, s_top ) >= 0;
		free( s_top );
		free( s_tm1 );
		break;
	default :
		rm_emsg_lineno = UNDEF;
		RM_errormsg( 1, "do_geq: type mismatch." );
		exit( 1 );
		break;
	}
}

static	void	do_equ()
{
	VALUE_T	*v_tm1, *v_top;
	int	t_tm1, t_top;
	char	*s_tm1, *s_top;

	v_top = &mem[ sp ];
	t_top = v_top->v_type;
	sp--;
	v_tm1 = &mem[ sp ];
	t_tm1 = v_tm1->v_type;
	v_tm1->v_type = T_INT;
	
	switch( T_IJ( t_tm1, t_top ) ){
	case T_IJ( T_INT, T_INT ) :
		v_tm1->v_value.v_ival =
			v_tm1->v_value.v_ival == v_top->v_value.v_ival;
		break;
	case T_IJ( T_INT, T_FLOAT ) :
		v_tm1->v_value.v_ival =
			v_tm1->v_value.v_ival == v_top->v_value.v_fval;
		break;
	case T_IJ( T_FLOAT, T_INT ) :
		v_tm1->v_value.v_ival =
			v_tm1->v_value.v_fval == v_top->v_value.v_ival;
		break;
	case T_IJ( T_FLOAT, T_FLOAT ) :
		v_tm1->v_value.v_ival =
			v_tm1->v_value.v_fval == v_top->v_value.v_fval;
		break;
	case T_IJ( T_STRING, T_STRING ) :
		s_tm1 = v_tm1->v_value.v_pval;
		s_top = v_top->v_value.v_pval;
		v_tm1->v_value.v_ival = strcmp( s_tm1, s_top ) == 0;
		free( s_top );
		free( s_tm1 );
		break;
	default :
		rm_emsg_lineno = UNDEF;
		RM_errormsg( 1, "do_equ: type mismatch." );
		break;
	}
}

static	void	do_neq()
{
	VALUE_T	*v_tm1, *v_top;
	int	t_tm1, t_top;
	char	*s_tm1, *s_top;

	v_top = &mem[ sp ];
	t_top = v_top->v_type;
	sp--;
	v_tm1 = &mem[ sp ];
	t_tm1 = v_tm1->v_type;
	v_tm1->v_type = T_INT;
	
	switch( T_IJ( t_tm1, t_top ) ){
	case T_IJ( T_INT, T_INT ) :
		v_tm1->v_value.v_ival =
			v_tm1->v_value.v_ival != v_top->v_value.v_ival;
		break;
	case T_IJ( T_INT, T_FLOAT ) :
		v_tm1->v_value.v_ival =
			v_tm1->v_value.v_ival != v_top->v_value.v_fval;
		break;
	case T_IJ( T_FLOAT, T_INT ) :
		v_tm1->v_value.v_ival =
			v_tm1->v_value.v_fval != v_top->v_value.v_ival;
		break;
	case T_IJ( T_FLOAT, T_FLOAT ) :
		v_tm1->v_value.v_ival =
			v_tm1->v_value.v_fval != v_top->v_value.v_fval;
		break;
	case T_IJ( T_STRING, T_STRING ) :
		s_tm1 = v_tm1->v_value.v_pval;
		s_top = v_top->v_value.v_pval;
		v_tm1->v_value.v_ival = strcmp( s_tm1, s_top ) != 0;
		free( s_top );
		free( s_tm1 );
		break;
	default :
		rm_emsg_lineno = UNDEF;
		RM_errormsg( 1, "do_neq: type mismatch." );
		break;
	}
}

static	void	do_leq()
{
	VALUE_T	*v_tm1, *v_top;
	int	t_tm1, t_top;
	char	*s_tm1, *s_top;

	v_top = &mem[ sp ];
	t_top = v_top->v_type;
	sp--;
	v_tm1 = &mem[ sp ];
	t_tm1 = v_tm1->v_type;
	v_tm1->v_type = T_INT;
	
	switch( T_IJ( t_tm1, t_top ) ){
	case T_IJ( T_INT, T_INT ) :
		v_tm1->v_value.v_ival =
			v_tm1->v_value.v_ival <= v_top->v_value.v_ival;
		break;
	case T_IJ( T_INT, T_FLOAT ) :
		v_tm1->v_value.v_ival =
			v_tm1->v_value.v_ival <= v_top->v_value.v_fval;
		break;
	case T_IJ( T_FLOAT, T_INT ) :
		v_tm1->v_value.v_ival =
			v_tm1->v_value.v_fval <= v_top->v_value.v_ival;
		break;
	case T_IJ( T_FLOAT, T_FLOAT ) :
		v_tm1->v_value.v_ival =
			v_tm1->v_value.v_fval <= v_top->v_value.v_fval;
		break;
	case T_IJ( T_STRING, T_STRING ) :
		s_tm1 = v_tm1->v_value.v_pval;
		s_top = v_top->v_value.v_pval;
		v_tm1->v_value.v_ival = strcmp( s_tm1, s_top ) <= 0;
		free( s_top );
		free( s_tm1 );
		break;
	default :
		rm_emsg_lineno = UNDEF;
		RM_errormsg( 1, "do_leq: type mismatch." );
		break;
	}
}

static	void	do_les()
{
	VALUE_T	*v_tm1, *v_top;
	int	t_tm1, t_top;
	char	*s_tm1, *s_top;

	v_top = &mem[ sp ];
	t_top = v_top->v_type;
	sp--;
	v_tm1 = &mem[ sp ];
	t_tm1 = v_tm1->v_type;
	v_tm1->v_type = T_INT;
	
	switch( T_IJ( t_tm1, t_top ) ){
	case T_IJ( T_INT, T_INT ) :
		v_tm1->v_value.v_ival =
			v_tm1->v_value.v_ival < v_top->v_value.v_ival;
		break;
	case T_IJ( T_INT, T_FLOAT ) :
		v_tm1->v_value.v_ival =
			v_tm1->v_value.v_ival < v_top->v_value.v_fval;
		break;
	case T_IJ( T_FLOAT, T_INT ) :
		v_tm1->v_value.v_ival =
			v_tm1->v_value.v_fval < v_top->v_value.v_ival;
		break;
	case T_IJ( T_FLOAT, T_FLOAT ) :
		v_tm1->v_value.v_ival =
			v_tm1->v_value.v_fval < v_top->v_value.v_fval;
		break;
	case T_IJ( T_STRING, T_STRING ) :
		s_tm1 = v_tm1->v_value.v_pval;
		s_top = v_top->v_value.v_pval;
		v_tm1->v_value.v_ival = strcmp( s_tm1, s_top ) < 0;
		free( s_top );
		free( s_tm1 );
		break;
	default :
		rm_emsg_lineno = UNDEF;
		RM_errormsg( 1, "do_les: type mismatch." );
		break;
	}
}

static	void	do_add()
{
	VALUE_T	*v_tm1, *v_top;
	int	t_tm1, t_top;

	v_top = &mem[ sp ];
	t_top = v_top->v_type;
	sp--;
	v_tm1 = &mem[ sp ];
	t_tm1 = v_tm1->v_type;
	
	switch( T_IJ( t_tm1, t_top ) ){
	case T_IJ( T_INT, T_INT ) :
		v_tm1->v_value.v_ival += v_top->v_value.v_ival;
		break;
	case T_IJ( T_INT, T_FLOAT ) :
		v_tm1->v_value.v_ival += v_top->v_value.v_fval;
		break;
	case T_IJ( T_INT, T_POS ) :
		break;
	case T_IJ( T_FLOAT, T_INT ) :
		v_tm1->v_value.v_fval += v_top->v_value.v_ival;
		break;
	case T_IJ( T_FLOAT, T_FLOAT ) :
		v_tm1->v_value.v_fval += v_top->v_value.v_fval;
		break;
	case T_IJ( T_STRING, T_STRING ) :
		break;
	case T_IJ( T_PAIR, T_PAIR ) :
		break;
	case T_IJ( T_POS, T_INT ) :
		break;
	default :
		rm_emsg_lineno = UNDEF;
		RM_errormsg( 1, "do_add: type mismatch." );
		break;
	}
}

static	void	do_sub()
{
	VALUE_T	*v_tm1, *v_top;
	int	t_tm1, t_top;

	v_top = &mem[ sp ];
	t_top = v_top->v_type;
	sp--;
	v_tm1 = &mem[ sp ];
	t_tm1 = v_tm1->v_type;
	
	switch( T_IJ( t_tm1, t_top ) ){
	case T_IJ( T_INT, T_INT ) :
		v_tm1->v_value.v_ival -= v_top->v_value.v_ival;
		break;
	case T_IJ( T_INT, T_FLOAT ) :
		v_tm1->v_value.v_ival -= v_top->v_value.v_fval;
		break;
	case T_IJ( T_INT, T_POS ) :
		break;
	case T_IJ( T_FLOAT, T_INT ) :
		v_tm1->v_value.v_fval -= v_top->v_value.v_ival;
		break;
	case T_IJ( T_FLOAT, T_FLOAT ) :
		v_tm1->v_value.v_fval -= v_top->v_value.v_fval;
		break;
	case T_IJ( T_PAIR, T_PAIR ) :
		break;
	case T_IJ( T_POS, T_INT ) :
		break;
	default :
		rm_emsg_lineno = UNDEF;
		RM_errormsg( 1, "do_sub: type mismatch." );
		break;
	}
}

static	void	do_mul()
{
	VALUE_T	*v_tm1, *v_top;
	int	t_tm1, t_top;

	v_top = &mem[ sp ];
	t_top = v_top->v_type;
	sp--;
	v_tm1 = &mem[ sp ];
	t_tm1 = v_tm1->v_type;
	
	switch( T_IJ( t_tm1, t_top ) ){
	case T_IJ( T_INT, T_INT ) :
		v_tm1->v_value.v_ival *= v_top->v_value.v_ival;
		break;
	case T_IJ( T_INT, T_FLOAT ) :
		v_tm1->v_value.v_ival *= v_top->v_value.v_fval;
		break;
	case T_IJ( T_FLOAT, T_INT ) :
		v_tm1->v_value.v_fval *= v_top->v_value.v_ival;
		break;
	case T_IJ( T_FLOAT, T_FLOAT ) :
		v_tm1->v_value.v_fval *= v_top->v_value.v_fval;
		break;
	default :
		rm_emsg_lineno = UNDEF;
		RM_errormsg( 1, "do_mul: type mismatch." );
		break;
	}
}

static	void	do_div()
{
	VALUE_T	*v_tm1, *v_top;
	int	t_tm1, t_top;

	v_top = &mem[ sp ];
	t_top = v_top->v_type;
	sp--;
	v_tm1 = &mem[ sp ];
	t_tm1 = v_tm1->v_type;
	
	switch( T_IJ( t_tm1, t_top ) ){
	case T_IJ( T_INT, T_INT ) :
		v_tm1->v_value.v_ival /= v_top->v_value.v_ival;
		break;
	case T_IJ( T_INT, T_FLOAT ) :
		v_tm1->v_value.v_ival /= v_top->v_value.v_fval;
		break;
	case T_IJ( T_FLOAT, T_INT ) :
		v_tm1->v_value.v_fval /= v_top->v_value.v_ival;
		break;
	case T_IJ( T_FLOAT, T_FLOAT ) :
		v_tm1->v_value.v_fval /= v_top->v_value.v_fval;
		break;
	default :
		rm_emsg_lineno = UNDEF;
		RM_errormsg( 1, "do_div: type mismatch." );
		break;
	}
}

static	void	do_mod()
{
	VALUE_T	*v_tm1, *v_top;
	int	t_tm1, t_top;

	v_top = &mem[ sp ];
	t_top = v_top->v_type;
	sp--;
	v_tm1 = &mem[ sp ];
	t_tm1 = v_tm1->v_type;
	
	switch( T_IJ( t_tm1, t_top ) ){
	case T_IJ( T_INT, T_INT ) :
		v_tm1->v_value.v_ival %= v_top->v_value.v_ival;
		break;
	default :
		rm_emsg_lineno = UNDEF;
		RM_errormsg( 1, "do_mod: type mismatch." );
		break;
	}
}

static	void	do_neg()
{
	VALUE_T	*v_top;
	int	t_top;

	v_top = &mem[ sp ];
	t_top = v_top->v_type;
	
	switch( t_top ){
	case T_INT :
		v_top->v_value.v_ival = -v_top->v_value.v_ival;
		break;
	case T_FLOAT :
		v_top->v_value.v_fval = -v_top->v_value.v_fval;
		break;
	default :
		rm_emsg_lineno = UNDEF;
		RM_errormsg( 1, "do_neg: type mismatch." );
		break;
	}
}

static	void	do_prst()
{

}

static	void	do_bpr()
{

}

static	void	do_i_pp()
{
	VALUE_T	*v_top;
	int	t_top;
	IDENT_T	*ip;

	v_top = &mem[ sp ];
	ip = v_top->v_value.v_pval;
	t_top = ip->i_type;
	
	switch( t_top ){
	case T_UNDEF :
		rm_emsg_lineno = UNDEF;
		sprintf( emsg, "do_i_pp: variable '%s' is undefined.",
			ip->i_name );
		RM_errormsg( 1, emsg );
		break;
	case T_INT :
		v_top->v_value.v_ival = ( ip->i_val.v_value.v_ival )++;
		break;
	default :
		rm_emsg_lineno = UNDEF;
		RM_errormsg( 1, "do_i_pp: type mismatch." );
		break;
	}
}

static	void	do_pp_i()
{
	VALUE_T	*v_top;
	int	t_top;
	IDENT_T	*ip;

	v_top = &mem[ sp ];
	t_top = v_top->v_type;
	ip = v_top->v_value.v_pval;
	
	switch( t_top ){
	case T_UNDEF :
		rm_emsg_lineno = UNDEF;
		sprintf( emsg, "do_pp_i: variable '%s' is undefined.",
			ip->i_name );
		RM_errormsg( 1, emsg );
		break;
	case T_INT :
		v_top->v_value.v_ival = ++( ip->i_val.v_value.v_ival );
		break;
	default :
		rm_emsg_lineno = UNDEF;
		RM_errormsg( 1, "do_pp_i: type mismatch." );
		break;
	}
}

static	void	do_i_mm()
{
	VALUE_T	*v_top;
	int	t_top;
	IDENT_T	*ip;

	v_top = &mem[ sp ];
	t_top = v_top->v_type;
	ip = v_top->v_value.v_pval;
	
	switch( t_top ){
	case T_UNDEF :
		rm_emsg_lineno = UNDEF;
		sprintf( emsg, "do_i_mm: variable '%s' is undefined.",
			ip->i_name );
		RM_errormsg( 1, emsg );
		break;
	case T_INT :
		v_top->v_value.v_ival = ( ip->i_val.v_value.v_ival )--;
		break;
	default :
		rm_emsg_lineno = UNDEF;
		RM_errormsg( 1, "do_i_mm: type mismatch." );
		exit( 1 );
		break;
	}
}

static	void	do_mm_i()
{
	VALUE_T	*v_top;
	int	t_top;
	IDENT_T	*ip;

	v_top = &mem[ sp ];
	t_top = v_top->v_type;
	ip = v_top->v_value.v_pval;
	
	switch( t_top ){
	case T_UNDEF :
		rm_emsg_lineno = UNDEF;
		sprintf( emsg, "do_mm_i: variable '%s' is undefined.",
			ip->i_name );
		RM_errormsg( 1, emsg );
		break;
	case T_INT :
		v_top->v_value.v_ival = --( ip->i_val.v_value.v_ival );
		break;
	default :
		rm_emsg_lineno = UNDEF;
		RM_errormsg( 1, "do_mm_i: type mismatch." );
		break;
	}
}

static	void	mk_stref_name( sym, name )
int	sym;
char	name[];
{

	switch( sym ){
	case SYM_SS :
		strcpy( name, "ss" );
		break;

	case SYM_H5 :
		strcpy( name, "h5" );
		break;
	case SYM_H3 :
		strcpy( name, "h3" );
		break;

	case SYM_P5 :
		strcpy( name, "p5" );
		break;
	case SYM_P3 :
		strcpy( name, "p3" );
		break;

	case SYM_T1 :
		strcpy( name, "t1" );
		break;
	case SYM_T2 :
		strcpy( name, "t2" );
		break;
	case SYM_T3 :
		strcpy( name, "t3" );
		break;

	case SYM_Q1 :
		strcpy( name, "q1" );
		break;
	case SYM_Q2 :
		strcpy( name, "q2" );
		break;
	case SYM_Q3 :
		strcpy( name, "q3" );
		break;
	case SYM_Q4 :
		strcpy( name, "q4" );
		break;

	default :
		strcpy( name, "" );
		break;
	}
}

static	void	addnode( lval, np, l_andor )
int	lval;
NODE_T	*np;
int	l_andor;
{
	POS_T	*posp;
	VALUE_T	v_node;
	int	sc;

	rm_emsg_lineno = np->n_lineno;
	switch( np->n_sym ){

	case SYM_CALL :
		if( ( sc = is_syscall( np ) ) != UNDEF ){
			v_node.v_type = T_INT;
			v_node.v_value.v_ival = sc;
			addinst( OP_SCL, &v_node );
		}else
			addinst( OP_FCL, &np->n_val );
		break;
	case SYM_LIST :
		break;
	case SYM_STREF :
		break;

	case SYM_PARMS :
		break;
	case SYM_DESCR :
		break;
	case SYM_SITES :
		break;

	case SYM_SS :
	case SYM_H5 :
	case SYM_H3 :
	case SYM_P5 :
	case SYM_P3 :
	case SYM_T1 :
	case SYM_T2 :
	case SYM_T3 :
	case SYM_Q1 :
	case SYM_Q2 :
	case SYM_Q3 :
	case SYM_Q4 :
		break;

	case SYM_ELSE :
		break;
	case SYM_FOR :
		break;
	case SYM_IF :
		break;
	case SYM_WHILE :
		break;

	case SYM_IDENT :
		if( lval ){
			addinst( OP_LDA, &np->n_val );
		}else{
			addinst( OP_LOD, &np->n_val );
		}
		break;
	case SYM_INT :
		addinst( OP_LDC, &np->n_val );
		break;
	case SYM_FLOAT :
		addinst( OP_LDC, &np->n_val );
		break;
	case SYM_STRING :
		addinst( OP_LDC, &np->n_val );
		break;
	case SYM_DOLLAR :
		v_node.v_type = T_POS;
		v_node.v_value.v_pval = NULL;
		addinst( OP_LDC, &v_node );
		break;

	case SYM_ASSIGN :
		addinst( OP_STO, NULL );
		break;

	case SYM_PLUS_ASSIGN :
		addinst( OP_ADD, NULL );
		addinst( OP_STO, NULL );
		break;
	case SYM_MINUS_ASSIGN :
		addinst( OP_SUB, NULL );
		addinst( OP_STO, NULL );
		break;
	case SYM_PERCENT_ASSIGN :
		addinst( OP_MOD, NULL );
		addinst( OP_STO, NULL );
		break;
	case SYM_STAR_ASSIGN :
		addinst( OP_MUL, NULL );
		addinst( OP_STO, NULL );
		break;
	case SYM_SLASH_ASSIGN :
		addinst( OP_DIV, NULL );
		addinst( OP_STO, NULL );
		break;

	case SYM_AND :
		v_lab.v_type = T_INT;
		v_lab.v_value.v_ival = l_andor;
		addinst( OP_AND, &v_lab );
		break;
	case SYM_OR :
		v_lab.v_type = T_INT;
		v_lab.v_value.v_ival = l_andor;
		addinst( OP_IOR, &v_lab );
		break;
	case SYM_NOT :
		addinst( OP_NOT, NULL );
		break;

	case SYM_EQUAL :
		addinst( OP_EQU, NULL );
		break;
	case SYM_NOT_EQUAL :
		addinst( OP_NEQ, NULL );
		break;
	case SYM_GREATER :
		addinst( OP_GTR, NULL );
		break;
	case SYM_GREATER_EQUAL :
		addinst( OP_GEQ, NULL );
		break;
	case SYM_LESS :
		addinst( OP_LES, NULL );
		break;
	case SYM_LESS_EQUAL :
		addinst( OP_LEQ, NULL );
		break;

	case SYM_MATCH :
		addinst( OP_MAT, NULL );
		break;
	case SYM_DONT_MATCH :
		addinst( OP_MAT, NULL );
		addinst( OP_NOT, NULL );
		break;
	case SYM_IN :
		addinst( OP_INS, NULL );
		break;

	case SYM_PLUS :
		addinst( OP_ADD, NULL );
		break;
	case SYM_MINUS :
		addinst( OP_SUB, NULL );
		break;
	case SYM_PERCENT :
		addinst( OP_MOD, NULL );
		break;
	case SYM_STAR :
		addinst( OP_MUL, NULL );
		break;
	case SYM_SLASH :
		addinst( OP_DIV, NULL );
		break;
	case SYM_NEGATE :
		addinst( OP_NEG, NULL );
		break;

	case SYM_COLON :
		addinst( OP_BPR, NULL );
		break;

	case SYM_MINUS_MINUS :
		if( np->n_left ){
			addinst( OP_I_MM, NULL );
		}else{
			addinst( OP_MM_I, NULL );
		}
		break;
	case SYM_PLUS_PLUS :
		if( np->n_left ){
			addinst( OP_I_PP, NULL );
		}else{
			addinst( OP_PP_I, NULL );
		}
		break;

	case SYM_LPAREN :
		break;
	case SYM_RPAREN :
		break;
	case SYM_LCURLY :
		break;
	case SYM_RCURLY :
		break;
	case SYM_COMMA :
		break;
	case SYM_SEMICOLON :
		break;
	case SYM_ERROR :
		RM_errormsg( 1, "addnode: SYM_ERROR." );
		break;
	default :
		sprintf( emsg, "addnode: Unknown symbol %d\n", np->n_sym );
		RM_errormsg( 1, emsg );
		break;
	}
}

static	void	addinst( op, vp )
int	op;
VALUE_T	*vp;
{
	INST_T	*ip;
	char	*sp;

	if( pc >= PROG_SIZE ){
		RM_errormsg( 1, "addinst: program size exceeded." );
	}else{
		ip = &prog[ l_prog ];
		l_prog++;
		ip->i_op = op;
		if( vp == NULL ){
			ip->i_val.v_type = T_UNDEF;
			ip->i_val.v_value.v_ival = 0;
		}else{
			ip->i_val.v_type = vp->v_type;
			if( vp->v_type == T_INT )
				ip->i_val.v_value.v_ival = vp->v_value.v_ival;
			else if( vp->v_type = T_FLOAT )
				ip->i_val.v_value.v_fval = vp->v_value.v_fval;
			else if( vp->v_type==T_STRING || vp->v_type==T_IDENT ){
				sp = ( char * )malloc( 
					strlen( vp->v_value.v_pval ) + 1 );
				if( sp == NULL ){
					RM_errormsg( 1,
						"addinst: can't allocate sp." );
				}
				strcpy( sp, vp->v_value.v_pval );
				ip->i_val.v_value.v_pval = sp;
			}
		}
	}
}

static	void	dumpinst( fp, i, ip )
FILE	*fp;
INST_T	*ip;
{
	VALUE_T	*vp;
	char	name[ 20 ];
	
	if( ip->i_op < 0 || ip->i_op >= N_OP ){
		rm_emsg_lineno = UNDEF;
		sprintf( emsg, "dumpinst: bad op %d\n", ip->i_op );
		RM_errormsg( 1, emsg );
	}

	fprintf( fp, "%5d ", i );
	fprintf( fp, "  %s", opnames[ ip->i_op ] );
	switch( ip->i_op ){
	case OP_NOOP :
	case OP_ACPT :
	case OP_RJCT :
	case OP_MRK :
	case OP_CLS :
		break;

	case OP_FCL :
	case OP_SCL :
		break;
	case OP_STRF :
		break;

	case OP_LDA :
	case OP_LOD :
		break;

	case OP_LDC :
		break;

	case OP_STO :
		break;

	case OP_AND :
	case OP_IOR :
		break;

	case OP_NOT :
		break;

	case OP_MAT :
	case OP_INS :
	case OP_GTR :
	case OP_GEQ :
	case OP_EQU :
	case OP_NEQ :
	case OP_LEQ :
	case OP_LES :
		break;

	case OP_ADD :
	case OP_SUB :
	case OP_MUL :
	case OP_DIV :
	case OP_MOD :
		break;

	case OP_NEG :
		break;

	case OP_PRST :
	case OP_BPR :
		break;

	case OP_I_PP :
	case OP_PP_I :
	case OP_I_MM :
	case OP_MM_I :
		break;

	case OP_FJP :
	case OP_JMP :
		break;
	}
	vp = &ip->i_val;
	if( ip->i_op == OP_LDA ){
		fprintf( fp, " %s", vp->v_value.v_pval );
	}else if( ip->i_op == OP_SCL ){
		fprintf( fp, " %s", scnames[ vp->v_value.v_ival ] );
	}else if( vp->v_type == T_INT ){
		fprintf( fp, " %d", vp->v_value.v_ival );
	}else if( vp->v_type == T_FLOAT ) 
		fprintf( fp, " %f", vp->v_value.v_fval );
	else if( vp->v_type == T_STRING )
		fprintf( fp, " \"%s\"", vp->v_value.v_pval );
	else if( vp->v_type == T_POS )
		fprintf( fp, " $" );
	else if( vp->v_type == T_IDENT )
		fprintf( fp, " %s", vp->v_value.v_pval );
	fprintf( fp, "\n" );
}

static	void	dumpstk( fp, msg )
FILE	*fp;
char	msg[];
{
	int	i;
	VALUE_T	*vp;

	fprintf( fp, "%s: sp = %5d, mp = %5d\n", msg, sp, mp );
	for( vp = mem, i = 0; i <= sp; i++, vp++ ){
		fprintf( fp, "  mem[%4d]: ", i );
		switch( vp->v_type ){
		case T_UNDEF :
			fprintf( fp, "U " );
			break;
		case T_INT :
			fprintf( fp, "I %6d", vp->v_value.v_ival );
			break;
		case T_FLOAT :
			fprintf( fp, "F %8.3f", vp->v_value.v_fval );
			break;
		case T_STRING :
			fprintf( fp, "S \"%s\"", vp->v_value.v_pval );
			break;
		case T_PAIR :
			fprintf( fp, "Pr" );
			break;
		case T_POS :
			fprintf( fp, "Ps" );
			break;
		case T_IDENT :
			fprintf( fp, "Id" );
			break;
		default :
			fprintf( fp, "? " );
			break;
		}
		fprintf( fp, "\n" );
	}
}
