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
extern	STREL_T	rm_descr[];
extern	int	rm_n_descr;
IDENT_T	*RM_find_id();
IDENT_T	*RM_enter_id();
NODE_T	*node();

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
#define	OP_STRF		6	/* Str. El. Reference	*/
#define	OP_LDA		7	/* Load Address		*/
#define	OP_LOD		8	/* Load Value		*/
#define	OP_LDC		9	/* Load Constant	*/
#define	OP_STO		10	/* Store top of stack	*/
#define	OP_AND		11	/* McCarthy And		*/
#define	OP_IOR		12	/* McCarthy Or		*/
#define	OP_NOT		13	/* Not			*/
#define	OP_MAT		14	/* Match		*/
#define	OP_INS		15	/* In Pairset		*/
#define	OP_GTR		16	/* Greater Than		*/
#define	OP_GEQ		17	/* Greater or Equal	*/
#define	OP_EQU		18	/* Equal		*/
#define	OP_NEQ		19	/* Not Equal		*/
#define	OP_LEQ		20	/* Less or Equal	*/
#define	OP_LES		21	/* Less Than		*/
#define	OP_ADD		22	/* Addition		*/
#define	OP_SUB		23	/* Subtraction		*/
#define	OP_MUL		24	/* Multiplication	*/
#define	OP_DIV		25	/* Division		*/
#define	OP_MOD		26	/* Modulus		*/
#define	OP_NEG		27	/* Negate		*/
#define	OP_PRST		28	/* Make a pairset	*/
#define	OP_BPR		29	/* Make a pair		*/
#define	OP_I_PP		30	/* use then incr (i++)	*/
#define	OP_PP_I		31	/* incr then use (++i)	*/
#define	OP_I_MM		32	/* use then decr (i--)	*/
#define	OP_MM_I		33	/* decr then use (--i)	*/
#define	OP_FJP		34	/* False Jump		*/
#define	OP_JMP		35	/* Jump			*/
#define	N_OP		36

static	char	*opnames[ N_OP ] = {
	"noop",
	"acpt",
	"rjct",
	"mrk",
	"cls",
	"fcl",
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

void	SC_if();
void	SC_else();
void	SC_endelse();
void	SC_endif();
void	SC_forinit();
void	SC_fortest();
void	SC_forincr();
void	SC_endfor();
void	SC_while();
void	SC_endwhile();
void	SC_break();
void	SC_continue();
void	SC_accept();
void	SC_reject();
void	SC_mark();
void	SC_clear();
void	SC_expr();
void	SC_node();
void	SC_dump();
void	SC_link();
int	SC_run();

static	void	fix_stref();
static	NODE_T	*mk_call_strid();
static	void	do_fcl();
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
static	void	addinst();
static	void	dumpinst();
static	void	dumpstk();

void	SC_action( np )
NODE_T	*np;
{

	SC_mark();
	SC_expr( 0, np );
	actlab = nextlab;
	nextlab++;
	v_lab.v_type = T_INT;
	v_lab.v_value.v_ival = actlab;
	addinst( OP_FJP, &v_lab );
}

void	SC_endaction()
{

	labtab[ actlab ] = l_prog;
}

void	SC_if( np )
NODE_T	*np;
{

	SC_mark();
	SC_expr( 0, np );
	ifstk[ ifstkp ] = nextlab;
	ifstkp++;
	nextlab += 2;
	v_lab.v_type = T_INT;
	v_lab.v_value.v_ival = ifstk[ ifstkp - 1 ];
	addinst( OP_FJP, &v_lab );
}

void	SC_else()
{

	addinst( OP_JMP, ifstk[ ifstkp - 1 ] + 1 );
	labtab[ ifstk[ ifstkp - 1 ] ] = l_prog;
}

void	SC_endelse()
{

	labtab[ ifstk[ ifstkp - 1 ] + 1 ] = l_prog;
	ifstkp--;
}

void	SC_endif()
{

	labtab[ ifstk[ ifstkp - 1 ] ] = l_prog;
	ifstkp--;
}

void	SC_forinit( np )
NODE_T	*np;
{

	loopstk[ loopstkp ] = nextlab;
	loopstkp++;
	nextlab += 3;
	SC_mark();
	SC_expr( 0, np );
	SC_clear();
}

void	SC_fortest( np )
NODE_T	*np;
{

	labtab[ loopstk[ loopstkp - 1 ] ] = l_prog;
	SC_mark();
	SC_expr( 0, np );
	v_lab.v_type = T_INT;
	v_lab.v_value.v_ival = loopstk[ loopstkp - 1 ] + 2;
	addinst( OP_FJP, &v_lab );
}

void	SC_forincr( np )
NODE_T	*np;
{

	loopincrstk[ loopstkp - 1 ] = np;
}

void	SC_endfor()
{

	labtab[ loopstk[ loopstkp - 1 ] + 1 ] = l_prog;
	SC_mark();
	SC_expr( 0, loopincrstk[ loopstkp - 1 ] );
	SC_clear();
	v_lab.v_type = T_INT;
	v_lab.v_value.v_ival = loopstk[ loopstkp - 1 ];
	addinst( OP_JMP, &v_lab );
	labtab[ loopstk[ loopstkp - 1 ] + 2 ] = l_prog;
	loopstkp--;
}

void	SC_while( np )
NODE_T	*np;
{

	loopstk[ loopstkp ] = nextlab;
	loopstkp++;
	nextlab += 3;
	labtab[ loopstk[ loopstkp -1 ] ] = l_prog;
	SC_mark();
	SC_expr( 0, np );
	v_lab.v_type = T_INT;
	v_lab.v_value.v_ival = loopstk[ loopstkp - 1 ] + 2;
	addinst( OP_FJP, &v_lab );
}

void	SC_endwhile()
{

	v_lab.v_type = T_INT;
	v_lab.v_value.v_ival = loopstk[ loopstkp - 1 ];
	addinst( OP_JMP, &v_lab );
	labtab[ loopstk[ loopstkp - 1 ] + 2 ] = l_prog;
	loopstkp--;
}

void	SC_break()
{

	v_lab.v_type = T_INT;
	v_lab.v_value.v_ival = loopstk[ loopstkp - 1 ] + 1;
	addinst( OP_JMP, &v_lab );
}

void	SC_continue()
{

	v_lab.v_type = T_INT;
	v_lab.v_value.v_ival = loopstk[ loopstkp - 1 ];
	addinst( OP_JMP, &v_lab );
}

void	SC_accept()
{

	addinst( OP_ACPT, NULL );
}

void	SC_reject()
{

	addinst( OP_RJCT, NULL );
}

void	SC_mark()
{

	addinst( OP_MRK, NULL );
}

void	SC_clear()
{

	addinst( OP_CLS, 0 );
}

void	SC_expr( lval, np )
int	lval;
NODE_T	*np;
{
	char	name[ 20 ];
	int	l_andor;
	VALUE_T	v_expr;

	if( np ){
		if( np->n_sym == SYM_CALL ){
			addinst( OP_MRK, NULL );
		}else if( np->n_sym == SYM_STREF ){
			fix_stref( np );
			v_expr.v_type = T_INT;
			v_expr.v_value.v_ival = np->n_left->n_sym;
		}else if( np->n_sym == SYM_LCURLY ){
			addinst( OP_MRK, NULL );
		}

		SC_expr( ISLVAL( np->n_sym ), np->n_left );

		if( ISRFLX( np->n_sym ) )
			SC_expr( 0, np->n_left );

		if( np->n_sym == SYM_OR || np->n_sym == SYM_AND ){
			l_andor = nextlab;
			nextlab++;
			SC_node( lval, np, l_andor );
			SC_expr( 0, np->n_right );
			labtab[ l_andor ] = l_prog;
		}else{
			SC_expr( 0, np->n_right );
			SC_node( lval, np, 0 );
		}

		if( np->n_sym == SYM_STREF ){
			addinst( OP_STRF, NULL );
		}else if( np->n_sym == SYM_LCURLY ){
			addinst( OP_PRST, NULL );
		}
	}
}

void	SC_node( lval, np, l_andor )
int	lval;
NODE_T	*np;
int	l_andor;
{
	POS_T	*posp;
	VALUE_T	v_node;

	switch( np->n_sym ){

	case SYM_CALL :
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
		fprintf( stderr, "SYM_ERROR\n" );
		break;
	default :
		fprintf( stderr, "SC_node: Unknown symbol %d\n", np->n_sym );
		break;
	}
}

void	SC_link()
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

void	SC_dump( fp )
FILE	*fp;
{
	INST_T	*ip;
	int	i;

	fprintf( fp, "SCORE: %4d inst.\n", l_prog );
	for( ip = prog, i = 0; i < l_prog; i++, ip++ ){
		dumpinst( fp, i, ip );
	}
}

int	SC_run( sbuf )
char	sbuf[];
{
	INST_T	*ip;
	
	if( l_prog <= 0 )
		return( 1 );

	sc_sbuf = sbuf;
	sp = mp = -1;
	for( pc = 0; ; ){
		if( pc < 0 || pc >= l_prog ){
			fprintf( stderr, "run: pc (%d) out of range 0..%d\n",
			pc, l_prog - 1 );
			exit( 1 );
		}
		ip = &prog[ pc ];

/*
fprintf( stdout, "SC_run, pc = %4d, op = %s\n", pc, opnames[ ip->i_op ] );
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
			mp = sp;
			break;
		case OP_CLS :
			sp = mp;
			break;

		case OP_FCL :
			do_fcl( ip );
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
			sp = mp;
			break;
		case OP_JMP :
			pc = ip->i_val.v_value.v_ival;
			break;
		default :
			fprintf( stderr, "run: unknown op %d\n", ip->i_op );
			exit( 1 );
			break;
		}
/*
dumpstk( stdout, "after op" );
*/
	}
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
					fprintf( stderr,
		"fix_stref: index parameter may not appear more than once.\n"
						);
					exit( 1 );
				}else
					n_index = np2->n_right;
			}else if( !strcmp( ip, "tag" ) ){
				if( n_tag != NULL ){
					fprintf( stderr,
		"fix_stref: tag parameter may not appear more than once.\n"
						);
					exit( 1 );
				}else
					n_tag = np2->n_right;
			}else if( !strcmp( ip, "pos" ) ){
				if( n_pos != NULL ){
					fprintf( stderr,
		"fix_stref: pos parameter may not appear more than once.\n"
						);
					exit( 1 );
				}else
					n_pos = np2->n_right;
			}else if( !strcmp( ip, "len" ) ){
				if( n_len != NULL ){
					fprintf( stderr,
		"fix_stref: len parameter may not appear more than once.\n"
						);
					exit( 1 );
				}else
					n_len = np2->n_right;
			}else{
				fprintf( stderr,
					"fix_stref: unknown parameter: '%s'\n",
					ip );
				exit( 1 );
			}
		}
	}

	if( n_index == NULL && n_tag == NULL ){
		fprintf( stderr,
			"fix_stref: index = or tag = require for stref().\n" );
		exit( 1 );
	}

	np1 = mk_call_strid( n_tag, n_index, np->n_left->n_sym );

	/* build the 3 parms to stref	*/
	if( n_len == NULL ){
		v_expr.v_type = T_INT;
		v_expr.v_value.v_ival = -1;
		np3 = node( SYM_INT, &v_expr, 0, 0 );
	}else
		np3 = n_len;
	np2 = node( SYM_LIST, 0, np3, NULL );
	if( n_pos == NULL ){
		v_expr.v_type = T_INT;
		v_expr.v_value.v_ival = 1;
		np3 = node( SYM_INT, &v_expr, 0, 0 );
	}else
		np3 = n_pos;
	np2 = node( SYM_LIST, 0, np3, np2 );
	np1 = node( SYM_LIST, 0, np1, np2 );
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
			fprintf( stderr,
				"mk_call_strid: no such tag: '%s'.\n", v_tag );
			exit( 1 );
		}
		if( v_index != UNDEF ){
			if( v_index < 0 || v_index >= rm_n_descr ){
				fprintf( stderr,
			"mk_call_strid: index must be between 1 and %d\n",
					rm_n_descr );
				exit( 1 );
			}else
				d_index = v_index;
			if( rm_descr[ d_index - 1 ].s_type != strel ){
				fprintf( stderr,
		"mk_call_strid: strel with index= %d has wrong type.\n",
					v_index );
				exit( 1 );
			}
		}
		if( d_tag == UNDEF && d_index == UNDEF ){
			fprintf( stderr,
"mk_call_strid: tag and index both have invalid values.\n" );
			exit( 1 );
		}else if( d_tag != UNDEF && d_index == UNDEF ){
			v_expr.v_type = T_INT;
			v_expr.v_value.v_ival = d_tag;
			np1 = node( SYM_INT, &v_expr, 0, 0 );
			return( np1 );
		}else if( d_tag == UNDEF && d_index != UNDEF ){
			v_expr.v_type = T_INT;
			v_expr.v_value.v_ival = d_index - 1;
			np1 = node( SYM_INT, &v_expr, 0, 0 );
			return( np1 );
		}else if( d_tag == d_index - 1 ){
			v_expr.v_type = T_INT;
			v_expr.v_value.v_ival = d_index;
			np1 = node( SYM_INT, &v_expr, 0, 0 );
			return( np1 );
		}else{
			fprintf( stderr,
		"mk_call_strid: tag and index values are inconsistant.\n" );
			exit( 1 );
		}
	}

	if( n_tag == NULL ){
		v_expr.v_type = T_STRING;
		v_expr.v_value.v_pval = "";
		n_tag = node( SYM_STRING, &v_expr, 0, 0 );
	}
	np2 = node( SYM_LIST, 0, n_tag, NULL );

	if( n_index == NULL ){
		v_expr.v_type = T_INT;
		v_expr.v_value.v_ival = UNDEF;
		n_index = node( SYM_INT, &v_expr, 0, 0 );
	}
		np3 = n_index;
	np2 = node( SYM_LIST, 0, n_index, np2 );

	v_expr.v_type = T_INT;
	v_expr.v_value.v_ival = strel;
	np3 = node( SYM_INT, &v_expr, 0, 0 );
	np2 = node( SYM_LIST, 0, np3, np2 );

	v_expr.v_type = T_STRING;
	v_expr.v_value.v_pval = "strid";
	np3 = node( SYM_IDENT, &v_expr, 0, 0 );
	np1 = node( SYM_CALL, 0, np3, np2 );
	return( np1 );
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
		mem[ sp ].v_type = T_INT;
		mem[ sp ].v_value.v_ival = len;
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
		fprintf( stderr, "do_strf: no such descriptor %d.\n", index );
		exit( 1 );
	}
	stp = &rm_descr[ index ];
	if( pos < 1 ){
		fprintf( stderr, "do_strf: pos must be > 0.\n" );
		exit( 1 );
	}else if( pos > stp->s_matchlen ){
		fprintf( stderr, "do_strf: pos must be <= %d.\n",
			stp->s_matchlen );
		exit( 1 );
	}
	pos--;
	if( len == 0 ){ 
		fprintf( stderr, "do_strf: len must be > 0.\n" );
		exit( 1 );
	}else if( len == UNDEF )	/* rest of string */
		len = stp->s_matchlen - pos;
	else
		len = MIN( stp->s_matchlen - pos, len );
	cp = ( char * )malloc( ( len + 1 ) * sizeof( char ) );
	if( cp == NULL ){
		fprintf( stderr, "do_strf: can't allocate cp.\n" );
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
		fprintf( stderr, "do_lda: variable '%s' is readonly.\n", 
			idp->i_name );
		exit( 1 );
	}else{
/*
		v_top->v_type = idp->i_type;
*/
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
		fprintf( stderr, "do_lod: variable '%s' is undefined.\n",
			idp->i_name );
		exit( 1 );
	}else{
		switch( idp->i_type ){
		case T_UNDEF :
			fprintf( stderr,
				"do_lod: variable '%s' is undefined.\n",
				idp->i_name );
			exit( 1 );
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
				fprintf( stderr,
					"do_lod: can't allocate cp.\n" );
				exit( 1 );
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
			fprintf( stderr, "do_ldc: can't allocate cp.\n" );
			exit( 1 );
		}
		strcpy( cp, ip->i_val.v_value.v_pval );
		v_top->v_value.v_pval = cp;
		break;
	case	T_POS :
		v_top->v_type = T_POS;
		v_top->v_value.v_pval = NULL;
		break;
	default :
		fprintf( stderr, "do_ldc: type mismatch.\n" );
		exit( 1 );
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
			fprintf( stderr,
				"do_sto: can't allocate new string.\n" );
			exit( 1 );
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
			fprintf( stderr,
				"do_sto: can't allocate new string.\n" );
			exit( 1 );
		}
		strcpy( cp, v_top->v_value.v_pval );
		free( idp->i_val.v_value.v_pval );
		idp->i_val.v_value.v_pval = cp;
		break;
	case T_IJ( T_PAIR, T_PAIR ) :
		break;
	default :
		fprintf( stderr, "do_sto: type mismatch.\n" );
		exit( 1 );
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
		fprintf( stderr, "do_and: type mismatch.\n" );
		exit( 1 );
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
		fprintf( stderr, "do_ior: type mismatch.\n" );
		exit( 1 );
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
		v_top->v_value.v_ival = !( v_top->v_value.v_ival == 0 );
		break;
	case T_FLOAT :
		v_top->v_value.v_ival = !( v_top->v_value.v_fval == 0.0 );
		break;
	case T_STRING :
		v_top->v_value.v_ival =
			!( *( char * )v_top->v_value.v_pval == '\0' );
		break;
	default :
		fprintf( stderr, "do_not: type mismatch.\n" );
		exit( 1 );
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
		fprintf( stderr, "do_gtr: type mismatch.\n" );
		exit( 1 );
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
		fprintf( stderr, "do_geq: type mismatch.\n" );
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
		fprintf( stderr, "do_equ: type mismatch.\n" );
		exit( 1 );
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
		fprintf( stderr, "do_neq: type mismatch.\n" );
		exit( 1 );
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
		fprintf( stderr, "do_leq: type mismatch.\n" );
		exit( 1 );
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
		fprintf( stderr, "do_les: type mismatch.\n" );
		exit( 1 );
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
		fprintf( stderr, "do_add: type mismatch.\n" );
		exit( 1 );
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
		fprintf( stderr, "do_sub: type mismatch.\n" );
		exit( 1 );
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
		fprintf( stderr, "do_mul: type mismatch.\n" );
		exit( 1 );
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
		fprintf( stderr, "do_div: type mismatch.\n" );
		exit( 1 );
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
		fprintf( stderr, "do_mod: type mismatch.\n" );
		exit( 1 );
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
		fprintf( stderr, "do_neg: type mismatch.\n" );
		exit( 1 );
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
		fprintf( stderr, "do_i_pp: variable '%s' is undefined.\n",
			ip->i_name );
		exit( 1 );
		break;
	case T_INT :
		v_top->v_value.v_ival = ( ip->i_val.v_value.v_ival )++;
		break;
	default :
		fprintf( stderr, "do_i_pp: type mismatch.\n" );
		exit( 1 );
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
		fprintf( stderr, "do_pp_i: variable '%s' is undefined.\n",
			ip->i_name );
		exit( 1 );
		break;
	case T_INT :
		v_top->v_value.v_ival = ++( ip->i_val.v_value.v_ival );
		break;
	default :
		fprintf( stderr, "do_pp_i: type mismatch.\n" );
		exit( 1 );
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
		fprintf( stderr, "do_i_mm: variable '%s' is undefined.\n",
			ip->i_name );
		exit( 1 );
		break;
	case T_INT :
		v_top->v_value.v_ival = ( ip->i_val.v_value.v_ival )--;
		break;
	default :
		fprintf( stderr, "do_i_mm: type mismatch.\n" );
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
		fprintf( stderr, "do_mm_i: variable '%s' is undefined.\n",
			ip->i_name );
		exit( 1 );
		break;
	case T_INT :
		v_top->v_value.v_ival = --( ip->i_val.v_value.v_ival );
		break;
	default :
		fprintf( stderr, "do_mm_i: type mismatch.\n" );
		exit( 1 );
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

static	void	addinst( op, vp )
int	op;
VALUE_T	*vp;
{
	INST_T	*ip;
	char	*sp;

	if( pc >= PROG_SIZE ){
		errormsg( 1, "addinst: program size exceeded." );
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
					fprintf( stderr,
					"addinst: can't allocate sp.\n" );
					exit( 1 );
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
		fprintf( fp, "dumpinst: bad op %d\n", ip->i_op );
		exit( 1 );
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

	fprintf( fp, "%s\n", msg );
	for( vp = mem, i = 0; i <= sp; i++, vp++ ){
		fprintf( fp, "mem[%4d]: ", i );
		switch( vp->v_type ){
		case T_UNDEF :
			fprintf( fp, "U " );
			break;
		case T_INT :
			fprintf( fp, "I %d", vp->v_value.v_ival );
			break;
		case T_FLOAT :
			fprintf( fp, "F %f", vp->v_value.v_fval );
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
