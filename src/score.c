#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "rnamot.h"
#include "y.tab.h"

#define	FMTLIST		"bBdiouxXfeEgGcCsSpn%"
#define	FMTFLAGS	"'-+ #0"

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
extern	int	rm_b2bc[];
extern	int	rm_efninit;
extern	char	rm_efndatadir[];
extern	int	rm_efndataok;
extern	int	*rm_bcseq;
extern	int	*rm_basepr;
extern	int	*rm_hstnum;
extern	int	rm_l_base;
extern	int	rm_efnusestdbp;
extern	PAIRSET_T	*rm_efnstdbp;

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

static	int	sc_comp;
static	int	sc_slen;
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
#define	OP_I_PP		29	/* use then incr (i++)	*/
#define	OP_PP_I		30	/* incr then use (++i)	*/
#define	OP_I_MM		31	/* use then decr (i--)	*/
#define	OP_MM_I		32	/* decr then use (--i)	*/
#define	OP_FJP		33	/* False Jump		*/
#define	OP_JMP		34	/* Jump			*/
#define	N_OP		35

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
	"incp",
	"pinc",
	"decp",
	"pdec",
	"fjp",
	"jmp" 
};

#define	SC_STRID	0
#define	SC_EFN		1
#define	SC_LENGTH	2
#define	SC_MISMATCHES	3
#define	SC_MISPAIRS	4
#define	SC_PAIRED	5
#define	SC_SPRINTF	6
#define	N_SC		7	

static	char	*scnames[ N_SC ] = {
	"STRID",
	"efn",
	"length",
	"mismatches",
	"mispairs",
	"paired",
	"sprintf"
};

typedef	struct	inst_t	{
	int	i_lineno;
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
#define	ESTK_SIZE	20
static	int	estk[ ESTK_SIZE ];
static	int	esp;		/* element stack pointer*/

static	char	emsg[ 256 ];

#define	SPRINTFBUF_SIZE	10000
static	char	sprintfbuf[ SPRINTFBUF_SIZE ];
static	char	*sbp;

void	RM_action( NODE_T * );
void	RM_endaction( void );
void	RM_if( NODE_T * );
void	RM_else( void );
void	RM_endelse( void );
void	RM_endif( void );
void	RM_forinit( NODE_T * );
void	RM_fortest( NODE_T * );
void	RM_forincr( NODE_T * );
void	RM_endfor( void );
void	RM_while( NODE_T * );
void	RM_endwhile( void );
void	RM_break( void );
void	RM_continue( void );
void	RM_accept( void );
void	RM_reject( void );
void	RM_mark( void );
void	RM_clear( void );
void	RM_expr( int, NODE_T * );
void	RM_linkscore( void );
void	RM_dumpscore( FILE * );
int	RM_score( int, int, char [] );

static	void	fixexpr( NODE_T * );
static	void	genexpr( int, NODE_T * );
static	int	is_syscall( NODE_T * );
static	void	fix_kw_stref( NODE_T * );
static	void	fix_ix_stref( NODE_T * );
static	NODE_T	*mk_call_strid( int, NODE_T * );
static	void	fix_call( NODE_T * );
static	void	do_fcl( INST_T * );
static	void	do_scl( INST_T * );
static	int	paired( STREL_T *, int, int );
static	int	strid( int, VALUE_T * );
static	int	do_sprintf( INST_T * );
static	int	do_fmt( INST_T *, char *, int, int * );
static	void	do_strf( INST_T * );
static	void	do_lda( INST_T * );
static	void	do_lod( INST_T * );
static	void	do_ldc( INST_T * );
static	void	do_sto( INST_T * );
static	void	do_and( INST_T * );
static	void	do_ior( INST_T * );
static	void	do_not( INST_T * );
static	void	do_mat( INST_T * );
static	void	do_ins( INST_T * );
static	void	do_gtr( INST_T * );
static	void	do_geq( INST_T * );
static	void	do_equ( INST_T * );
static	void	do_neq( INST_T * );
static	void	do_leq( INST_T * );
static	void	do_les( INST_T * );
static	void	do_add( INST_T * );
static	void	do_sub( INST_T * );
static	void	do_mul( INST_T * );
static	void	do_div( INST_T * );
static	void	do_mod( INST_T * );
static	void	do_neg( INST_T * );
static	void	do_i_pp( INST_T * );
static	void	do_pp_i( INST_T * );
static	void	do_i_mm( INST_T * );
static	void	do_mm_i( INST_T * );

static	int	setupefn( INST_T *, int, int, int, int );
static	int	setbp( STREL_T *, int, int, int, int, int [] );
static	void	mk_stref_name( int, char []);
static	void	addnode( int, NODE_T *, int );
static	void	addinst( int, int, VALUE_T * );
static	void	dumpinst( FILE *, int, INST_T * );
static	void	dumpstk( FILE *, char [] );

char	*getenv();

#define	TM_MEM_SIZE	1000
typedef	struct	tm_mem_t	{
	int	t_freed;
	char	*t_id;
	int	t_pc;
	void	*t_addr;
} TM_MEM_T;
static	TM_MEM_T	tm_mem[ TM_MEM_SIZE ];
static	int	tm_n_mem;

static	char	*tm_malloc( int, char [] );
static	void	tm_free( void * );
static	void	tm_report( void );

void	RM_action( NODE_T *np )
{

	RM_mark();
	RM_expr( 0, np );
	actlab = nextlab;
	nextlab++;
	v_lab.v_type = T_INT;
	v_lab.v_value.v_ival = actlab;
	addinst( np->n_lineno, OP_FJP, &v_lab );
}

void	RM_endaction( void )
{

	labtab[ actlab ] = l_prog;
}

void	RM_if( NODE_T *np )
{

	RM_mark();
	RM_expr( 0, np );
	ifstk[ ifstkp ] = nextlab;
	ifstkp++;
	nextlab += 2;
	v_lab.v_type = T_INT;
	v_lab.v_value.v_ival = ifstk[ ifstkp - 1 ];
	addinst( np->n_lineno, OP_FJP, &v_lab );
}

void	RM_else( void )
{

	v_lab.v_type = T_INT;
	v_lab.v_value.v_ival = ifstk[ ifstkp - 1 ] + 1;
	addinst( UNDEF, OP_JMP, &v_lab );
	labtab[ ifstk[ ifstkp - 1 ] ] = l_prog;
}

void	RM_endelse( void )
{

	labtab[ ifstk[ ifstkp - 1 ] + 1 ] = l_prog;
	ifstkp--;
}

void	RM_endif( void )
{

	labtab[ ifstk[ ifstkp - 1 ] ] = l_prog;
	ifstkp--;
}

void	RM_forinit( NODE_T *np )
{

	loopstk[ loopstkp ] = nextlab;
	loopstkp++;
	nextlab += 3;
	RM_mark();
	RM_expr( 0, np );
	RM_clear();
}

void	RM_fortest( NODE_T *np )
{

	labtab[ loopstk[ loopstkp - 1 ] ] = l_prog;
	RM_mark();
	RM_expr( 0, np );
	v_lab.v_type = T_INT;
	v_lab.v_value.v_ival = loopstk[ loopstkp - 1 ] + 2;
	addinst( np->n_lineno, OP_FJP, &v_lab );
}

void	RM_forincr( NODE_T *np )
{

	loopincrstk[ loopstkp - 1 ] = np;
}

void	RM_endfor( void )
{

	labtab[ loopstk[ loopstkp - 1 ] + 1 ] = l_prog;
	RM_mark();
	RM_expr( 0, loopincrstk[ loopstkp - 1 ] );
	RM_clear();
	v_lab.v_type = T_INT;
	v_lab.v_value.v_ival = loopstk[ loopstkp - 1 ];
	addinst( UNDEF, OP_JMP, &v_lab );
	labtab[ loopstk[ loopstkp - 1 ] + 2 ] = l_prog;
	loopstkp--;
}

void	RM_while( NODE_T *np )
{

	loopstk[ loopstkp ] = nextlab;
	loopstkp++;
	nextlab += 3;
	labtab[ loopstk[ loopstkp -1 ] ] = l_prog;
	RM_mark();
	RM_expr( 0, np );
	v_lab.v_type = T_INT;
	v_lab.v_value.v_ival = loopstk[ loopstkp - 1 ] + 2;
	addinst( np->n_lineno, OP_FJP, &v_lab );
}

void	RM_endwhile( void )
{

	v_lab.v_type = T_INT;
	v_lab.v_value.v_ival = loopstk[ loopstkp - 1 ];
	addinst( UNDEF, OP_JMP, &v_lab );
	labtab[ loopstk[ loopstkp - 1 ] + 2 ] = l_prog;
	loopstkp--;
}

void	RM_break( void )
{

	v_lab.v_type = T_INT;
	v_lab.v_value.v_ival = loopstk[ loopstkp - 1 ] + 1;
	addinst( UNDEF, OP_JMP, &v_lab );
}

void	RM_continue( void )
{

	v_lab.v_type = T_INT;
	v_lab.v_value.v_ival = loopstk[ loopstkp - 1 ];
	addinst( UNDEF, OP_JMP, &v_lab );
}

void	RM_accept( void )
{

	addinst( UNDEF, OP_ACPT, NULL );
}

void	RM_reject( void )
{

	addinst( UNDEF, OP_RJCT, NULL );
}

void	RM_mark( void )
{

	addinst( UNDEF, OP_MRK, NULL );
}

void	RM_clear( void )
{

	addinst( UNDEF, OP_CLS, 0 );
}

void	RM_expr( int lval, NODE_T *np )
{

	fixexpr( np );
	genexpr( lval, np );
}

void	RM_linkscore( void )
{
	int	i;
	INST_T	*ip;
	IDENT_T	*idp;
	VALUE_T	v_svars;

	for( ip = prog, i = 0; i < l_prog; i++, ip++ ){
		if( ip->i_op == OP_FJP || ip->i_op == OP_JMP ){
			ip->i_val.v_value.v_ival =
				labtab[ ip->i_val.v_value.v_ival ];
		}else if( ip->i_op == OP_IOR || ip->i_op == OP_AND ){
			ip->i_val.v_value.v_ival =
				labtab[ ip->i_val.v_value.v_ival ];
		}
	}
	v_svars.v_type = T_INT;
	v_svars.v_value.v_ival = rm_n_descr;
	RM_enter_id( "NSE", T_INT, C_VAR, S_GLOBAL, 1, &v_svars );
}

void	RM_dumpscore( FILE *fp )
{
	INST_T	*ip;
	int	i;

	fprintf( fp, "SCORE: %4d inst.\n", l_prog );
	for( ip = prog, i = 0; i < l_prog; i++, ip++ ){
		dumpinst( fp, i, ip );
	}
}

int	RM_score( int comp, int slen, char sbuf[] )
{
	INST_T	*ip;
	int	rval;
	
	if( l_prog <= 0 )
		return( 1 );

	sc_comp = comp;
	sc_slen = slen;
	sc_sbuf = sbuf;
	esp = sp = mp = -1;
	for( rval = 0, pc = 0; ; ){
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
			rval = 1;
			goto SCORED;
			break;
		case OP_RJCT :
			goto SCORED;
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
			do_sto( ip );
			break;

		case OP_AND :
			do_and( ip );
			break;
		case OP_IOR :
			do_ior( ip );
			break;
		case OP_NOT :
			do_not( ip );
			break;

		case OP_MAT :
			do_mat( ip );
			break;
		case OP_INS :
			do_ins( ip );
			break;
		case OP_GTR :
			do_gtr( ip );
			break;
		case OP_GEQ :
			do_geq( ip );
			break;
		case OP_EQU :
			do_equ( ip );
			break;
		case OP_NEQ :
			do_neq( ip );
			break;
		case OP_LEQ :
			do_leq( ip );
			break;
		case OP_LES :
			do_les( ip );
			break;

		case OP_ADD :
			do_add( ip );
			break;
		case OP_SUB :
			do_sub( ip );
			break;
		case OP_MUL :
			do_mul( ip );
			break;
		case OP_DIV :
			do_div( ip );
			break;
		case OP_MOD :
			do_mod( ip );
			break;
		case OP_NEG :
			do_neg( ip );
			break;

		case OP_I_PP :
			do_i_pp( ip );
			break;
		case OP_PP_I :
			do_pp_i( ip );
			break;
		case OP_I_MM :
			do_i_mm( ip );
			break;
		case OP_MM_I :
			do_mm_i( ip );
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
			rm_emsg_lineno = ip->i_lineno;
			sprintf( emsg, "RM_score: unknown op %d.", ip->i_op );
			RM_errormsg( 1, emsg );
			break;
		}

/*
dumpstk( stdout, "after op " );
*/

	}

SCORED : ;

#ifdef MEMDEBUG
	tm_report();
#endif
	
	return( rval );
}

static	void	fixexpr( NODE_T *np )
{

	if( np ){
		fixexpr( np->n_left );
		fixexpr( np->n_right );
		if( np->n_sym == SYM_KW_STREF ){
			fix_kw_stref( np );
		}else if( np->n_sym == SYM_IX_STREF ){
			fix_ix_stref( np );
		}else if( np->n_sym == SYM_CALL ){
			fix_call( np );
		}
	}
}

static	void	genexpr( int lval, NODE_T *np )
{
	int	l_andor;

	if( np ){
		if( np->n_sym == SYM_CALL ){
			addinst( np->n_lineno, OP_MRK, NULL );
		}else if( np->n_sym == SYM_IN ){
			addinst( np->n_lineno, OP_MRK, NULL );
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

		if( np->n_sym == SYM_KW_STREF || np->n_sym == SYM_IX_STREF ){
			addinst( np->n_lineno, OP_STRF, NULL );
		}
	}
}

static	int	is_syscall( NODE_T *np )
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

static	void	fix_kw_stref( NODE_T *np )
{
	int	sel;
	NODE_T	*n_id, *n_index, *n_tag, *n_pos, *n_len;
	NODE_T	*np1, *np2, *np3;
	char	*ip;
	VALUE_T	v_expr;

	sel = np->n_left->n_sym;
	n_id = n_index = n_tag = n_pos = n_len = NULL;
	for( np1 = np->n_right; np1; np1 = np1->n_right ){
		np2 = np1->n_left;
		np3 = np2->n_left;
		if( np3->n_sym == SYM_IDENT ){
			ip = np3->n_val.v_value.v_pval;
			if( !strcmp( ip, "index" ) ){
				if( n_index != NULL ){
					rm_emsg_lineno = n_index->n_lineno;
					RM_errormsg( 1,
	"fix_kw_stref: index parameter may not appear more than once." );
				}else
					n_index = np2->n_right;
			}else if( !strcmp( ip, "tag" ) ){
				if( n_tag != NULL ){
					rm_emsg_lineno = n_tag->n_lineno;
					RM_errormsg( 1,
	"fix_kw_stref: tag parameter may not appear more than once." );
				}else
					n_tag = np2->n_right;
			}else if( !strcmp( ip, "pos" ) ){
				if( n_pos != NULL ){
					rm_emsg_lineno = n_pos->n_lineno;
					RM_errormsg( 1,
		"fix_kw_stref: pos parameter may not appear more than once." );
				}else
					n_pos = np2->n_right;
			}else if( !strcmp( ip, "len" ) ){
				if( n_len != NULL ){
					rm_emsg_lineno = n_len->n_lineno;
					RM_errormsg( 1,
		"fix_kw_stref: len parameter may not appear more than once." );
				}else
					n_len = np2->n_right;
			}else{
				rm_emsg_lineno = np3->n_lineno;
				sprintf( emsg,
				"fix_kw_stref: unknown parameter: '%s'.",
					ip );
				RM_errormsg( 1, emsg );
			}
		}
	}

	rm_emsg_lineno = np->n_lineno;
	if( n_index == NULL && n_tag == NULL ||
		n_index != NULL && n_tag != NULL )
	{
		rm_emsg_lineno = np->n_lineno;
		RM_errormsg( 1,
		"fix_kw_stref: one of index= or tag= required for stref()." );
	}else
		n_id = n_index != NULL ? n_index : n_tag;

	np1 = mk_call_strid( sel, n_id );

	/* build the 3 parms to stref	*/
	if( n_len == NULL ){
		v_expr.v_type = T_INT;
		v_expr.v_value.v_ival = UNDEF;
		np3 = RM_node( SYM_INT, &v_expr, 0, 0 );
	}else
		np3 = n_len;
	np2 = RM_node( SYM_LIST, 0, np3, NULL );
	if( n_pos == NULL ){
		v_expr.v_type = T_INT;
/*
		v_expr.v_value.v_ival = 1;
*/
		v_expr.v_value.v_ival = UNDEF;
		np3 = RM_node( SYM_INT, &v_expr, 0, 0 );
	}else
		np3 = n_pos;
	np2 = RM_node( SYM_LIST, 0, np3, np2 );
	np1 = RM_node( SYM_LIST, 0, np1, np2 );
	np->n_right = np1;
}

static	void	fix_ix_stref( NODE_T *np )
{
	int	sel;
	NODE_T	*n_id, *n_pos, *n_len;
	NODE_T	*np1, *np2, *np3;
	VALUE_T	v_expr;

	sel = np->n_left->n_sym;
	n_id = n_pos = n_len = NULL;
	np1 = np->n_right;
	np2 = np1->n_left; 

	n_id = np2;
	if( np1->n_right != NULL ){
		np1 = np1->n_right;
		n_pos = np1->n_left; 
		if( np1->n_right != NULL ){
			np1 = np1->n_right;
			n_len = np1->n_left; 
		}
	}

	np1 = mk_call_strid( sel, n_id );

	/* build the 3 parms to stref	*/
	if( n_len == NULL ){
		v_expr.v_type = T_INT;
		v_expr.v_value.v_ival = UNDEF;
		np3 = RM_node( SYM_INT, &v_expr, 0, 0 );
	}else
		np3 = n_len;
	np2 = RM_node( SYM_LIST, 0, np3, NULL );
	if( n_pos == NULL ){
		v_expr.v_type = T_INT;
		v_expr.v_value.v_ival = UNDEF;
		np3 = RM_node( SYM_INT, &v_expr, 0, 0 );
	}else
		np3 = n_pos;
	np2 = RM_node( SYM_LIST, 0, np3, np2 );
	np1 = RM_node( SYM_LIST, 0, np1, np2 );
	np->n_right = np1;
}

static	NODE_T	*mk_call_strid( int strel, NODE_T *n_id )
{
	NODE_T	*np1, *np2, *np3;
	VALUE_T	v_expr;

	np2 = RM_node( SYM_LIST, 0, n_id, NULL );

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

static	void	fix_call( NODE_T *np )
{
	int	sc, pcnt;
	NODE_T	*np1, *np2, *np3;

	sc = is_syscall( np );
	switch( sc ){
	case SC_STRID :
		rm_emsg_lineno = np->n_lineno;
		RM_errormsg( 1, "fix_call: STRID can not be called by user." );
		break;
	case SC_EFN :
		for( pcnt = 0, np1 = np->n_right; np1; np1 = np1->n_right )
			pcnt++;
		if( pcnt != 2 ){
			rm_emsg_lineno = np->n_lineno;
			sprintf( emsg,
				"fix_call: function '%s' has 2 parameters.",
				 scnames[ sc ] );
			RM_errormsg( 1, emsg );
		}
		np1 = np->n_right;
		np2 = np1->n_right;

		np1 = np1->n_left;
		np2 = np2->n_left;

		if( np1->n_sym != SYM_KW_STREF && np1->n_sym != SYM_IX_STREF ){
			rm_emsg_lineno = np1->n_lineno;
			sprintf( emsg,
			"fix_call: function '%s' takes only strel arguments.",
				scnames[ sc ] );
			RM_errormsg( 1, emsg );
		}else{
			np1 = np1->n_right;
			for( np3 = np1; np3->n_right ; np3 = np3->n_right )
				;
		}
		if( np2->n_sym != SYM_KW_STREF && np2->n_sym != SYM_IX_STREF ){
			rm_emsg_lineno = np2->n_lineno;
			sprintf( emsg,
			"fix_call: function '%s' takes only strel arguments.",
				scnames[ sc ] );
			RM_errormsg( 1, emsg );
		}else
			np2 = np2->n_right;

		np3->n_right = np2;
		np->n_right = np1;
		break;
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
				"fix_call: function '%s' has 1 parameter.",
				 scnames[ sc ] );
			RM_errormsg( 1, emsg );
		}
		np1 = np->n_right;
		np1 = np1->n_left;
		np1 = np1->n_right;
		np->n_right = np1;
		break;
	case SC_SPRINTF :
		break;
	default :
		rm_emsg_lineno = np->n_lineno;
		RM_errormsg( 1, "fix_call: unknown syscall." );
		break;
	}
}

static	void	do_fcl( INST_T *ip )
{
	char	*cp;
	int	len;

	if( !strcmp( ip->i_val.v_value.v_pval, "length" ) ){
		cp = mem[ sp ].v_value.v_pval;
		len = strlen( cp );
		tm_free( cp );
		mem[ mp ].v_type = T_INT;
		mem[ mp ].v_value.v_ival = len;
		sp = mp;
		mp = mem[ mp ].v_value.v_ival;
	}
}

static	void	do_scl( INST_T *ip )
{
	char	*cp;
	VALUE_T	*v_id;
	int	i, stype, idx, pos, len;
	int	idx2, pos2, len2;
	STREL_T	*stp, *stp2;
	IDENT_T	*idp;
	float	rval;

	switch( ip->i_val.v_value.v_ival ){
	case SC_STRID :
		v_id = &mem[ sp  ];
		stype = mem[ sp - 1 ].v_value.v_ival;
		sp = mp;
		mp = mem[ mp ].v_value.v_ival;
		mem[ sp ].v_type = T_INT;
		idx = mem[ sp ].v_value.v_ival = strid( stype, v_id );
		if( v_id->v_type == T_STRING )
			tm_free( v_id->v_value.v_pval );
		esp++;
		estk[ esp ] = idx;
		break;

	case SC_EFN :
		if( !rm_efninit ){
			rm_efninit = 1;
			idp = RM_find_id( "windowsize" );
			if( RM_allocefnds( idp->i_val.v_value.v_ival ) )
				rm_efndataok = 0;
			else{
				idp = RM_find_id( "efn_datadir" );
				cp = ( char * )idp->i_val.v_value.v_pval;
				if( cp == NULL || *cp == '\0' ){
					if( ( cp = getenv( "EFNDATA" ) ) )
						strcpy( rm_efndatadir, cp );
				}else
					strcpy( rm_efndatadir, cp );
				idp = RM_find_id( "efn_usestdbp" );
				rm_efnusestdbp = idp->i_val.v_value.v_ival;
				rm_efndataok = RM_getefndata();
			}
		}

		idx  = mem[ sp - 5 ].v_value.v_ival;
		if( idx < 0 || idx >= rm_n_descr ){
			rm_emsg_lineno = UNDEF;
			sprintf( emsg, 
		"do_scl: efn: 1st descr index must be between 1 and %d.",
				rm_n_descr );
			RM_errormsg( 1, emsg );
		}
		stp = &rm_descr[ idx ];
		pos = mem[ sp - 4 ].v_value.v_ival;
		if( pos == UNDEF )
			pos = 1;
		else if( pos < 0 ){
			rm_emsg_lineno = ip->i_lineno;
			RM_errormsg( 1, "efn: pos1 must be > 0." );
		}else if( stp->s_matchlen == 0 ){
			rm_emsg_lineno = ip->i_lineno;
			RM_errormsg( 1,
	"do_scl: efn: descr1 must have match len > 0." );
		}else if( pos > stp->s_matchlen ){
			rm_emsg_lineno = ip->i_lineno;
			sprintf( emsg, "do_scl: efn: pos1 must be <= %d.",
				stp->s_matchlen );
			RM_errormsg( 1, emsg );
		}
		pos--;
		len  = mem[ sp - 3 ].v_value.v_ival;

		idx2 = mem[ sp - 2 ].v_value.v_ival;
		if( idx2 < 0 || idx2 >= rm_n_descr ){
			rm_emsg_lineno = ip->i_lineno;
			sprintf( emsg, 
		"do_scl: efn: 2nd descr index must be between 1 and %d.",
				rm_n_descr );
			RM_errormsg( 1, emsg );
		}else if( idx >= idx2 ){
			rm_emsg_lineno = ip->i_lineno;
			RM_errormsg( 1, 
		"do_scl: efn: 2nd descr index must follow 1st descr.",
				rm_n_descr );
			RM_errormsg( 1, emsg );
		}
		stp2 = &rm_descr[ idx2 ];
		pos2 = mem[ sp - 1 ].v_value.v_ival;
		if( pos2 == UNDEF )
			pos2 = stp2->s_matchlen;
		else if( pos2 < 0 ){
			rm_emsg_lineno = ip->i_lineno;
			RM_errormsg( 1, "efn: pos2 must be > 0." );
		}else if( stp2->s_matchlen == 0 ){
			rm_emsg_lineno = ip->i_lineno;
			RM_errormsg( 1,
	"do_scl: efn: descr2 must have match len > 0." );
		}else if( pos > stp2->s_matchlen ){
			rm_emsg_lineno = ip->i_lineno;
			sprintf( emsg, "do_scl: efn: pos2 must be <= %d.",
				stp2->s_matchlen );
			RM_errormsg( 1, emsg );
		}
		pos2--;
		len2 = mem[ sp     ].v_value.v_ival;
		if( setupefn( ip, idx, pos, idx2, pos2 ) ){
			RM_initst();
			rval = 0.01 * RM_efn( 0, rm_l_base, 1 );
		}else
			rval = EFN_INFINITY;
		sp = mp;
		mp = mem[ mp ].v_value.v_ival;
		mem[ sp ].v_type = T_FLOAT;
		mem[ sp ].v_value.v_dval = rval;
		break;

	case SC_LENGTH :
		cp = mem[ sp ].v_value.v_pval;
		len = strlen( cp );
		tm_free( cp );
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
		sp = mp;
		mp = mem[ mp ].v_value.v_ival;
		mem[ sp ].v_type = T_INT;
		mem[ sp ].v_value.v_ival = paired( stp, pos, len );
		break;

	case SC_SPRINTF :
		do_sprintf( ip );
		cp = ( char * )malloc( strlen( sprintfbuf ) + 1 );
		if( cp == NULL ){
			rm_emsg_lineno = ip->i_lineno;
			RM_errormsg( 1, "do_strf: can't allocate cp1." );
		}
		strcpy( cp, sprintfbuf );
		for( i = sp; i > mp; i-- ){
			if( mem[ i ].v_type == T_STRING )
				tm_free( mem[ i ].v_value.v_pval );
		}
		sp = mp;
		mp = mem[ mp ].v_value.v_ival;
		mem[ sp ].v_type = T_STRING;
		mem[ sp ].v_value.v_pval = cp;
		break;

	default :
		rm_emsg_lineno = UNDEF;
		RM_errormsg( 1, "do_scl: undefined syscall." );
		break;
	}
}

static	int	paired( STREL_T *stp, int pos, int len )
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
			b4 = sc_sbuf[ p4 - pos - i ];
			if( !RM_quad( stp1->s_pairset, b1, b2, b3, b4 ) )
				return( 0 );
		}
		return( 1 );
		break;
	default :
		rm_emsg_lineno = stp->s_lineno;
		RM_errormsg( 1, "paired() does not accept descr type 'ss'." );
		return( 0 );
		break;
	}
}

static	int	strid( int stype, VALUE_T *v_id )
{
	int	s, idx;
	STREL_T	*stp;
	char	*tag, name1[ 20 ], name2[ 20 ];

	if( v_id->v_type == T_INT ){
		idx = v_id->v_value.v_ival;
		if( idx < 1 || idx > rm_n_descr ){
			rm_emsg_lineno = UNDEF;
			sprintf( emsg,
				"strid: index (%d) out of range: 1 .. %d.",
				idx, rm_n_descr );
			RM_errormsg( 1, emsg );
		}
		idx--;
		stp = &rm_descr[ idx ];
		if( stype != SYM_SE ){
			if( stp->s_type != stype ){
				mk_stref_name( stype, name1 );
				mk_stref_name( stp->s_type, name2 );
				rm_emsg_lineno = UNDEF;
				sprintf( emsg,
			"strid: descr type mismatch: is %s should be %s.",
					name1, name2 );
				RM_errormsg( 1, emsg );
			}
		}
	}else if( v_id->v_type == T_STRING ){
		tag = v_id->v_value.v_pval;
		stp = rm_descr;
		for( idx = UNDEF, s = 0; s < rm_n_descr; s++, stp++ ){ 
			if( stp->s_tag == NULL )
				continue;
			else if( !strcmp( stp->s_tag, tag ) ){
				if( stp->s_type == stype ){
					idx = s;
					break;
				}else if(stp->s_type==SYM_SS && stype==SYM_SE){ 
					idx = s;
					break;
/*
				}else{
					mk_stref_name( stype, name1 );
					mk_stref_name( stp->s_type, name2 );
					rm_emsg_lineno = UNDEF;
					sprintf( emsg,
				"strid: ambiguous descr reference: %s vs %s.",
						name1, name2 );
					RM_errormsg( 1, emsg );
*/
				}
			}
		}
		if( idx == UNDEF ){
			rm_emsg_lineno = UNDEF;
			sprintf( emsg, "strid: no such descr '%s'.", tag );
			RM_errormsg( 1, emsg );
		}
	}
	return( idx );
}

static	int	do_sprintf( INST_T *ip )
{
	int	c_arg, n_args;
	char	*fstr;
	char	fmt[ 256 ];
	char	*fp, *pp, *epp;
	int	rval = 0;

	n_args = sp - mp;
	fstr = mem[ mp + 1 ].v_value.v_pval;
	sbp = sprintfbuf;
	for( c_arg = 0, fp = fstr; pp = strchr( fp, '%' ); ){
		strncpy( sbp, fp, pp - fp );
		sbp[ pp - fp ] = '\0';
		sbp += strlen( sbp );
		epp = strpbrk( &pp[ 1 ], FMTLIST );
		strncpy( fmt, pp, epp - pp + 1 );
		fmt[ epp - pp + 1 ] = '\0';
		if( rval = do_fmt( ip, fmt, n_args, &c_arg ) )
			break;
		fp = epp + 1;
	}
	strcpy( sbp, fp );
	return( rval );
}

static	int	do_fmt( INST_T *ip, char *fmt, int n_args, int *c_arg )
{
	int	l_fmt, nprt;
	int	type;
	char	work[ 256 ];
	char	posp[ 20 ], flags[ 20 ], width[ 20 ], prec[ 20 ], size[ 20 ];
	char	*efp, *sp;
	char	*dot, *star, *dollar;
	char	*wp;
	int	u_arg, r_arg;
	int	w_ind, u_wid, r_wid, p_ind, u_prec, r_prec;
	int	i_argc, u_argc;
	VALUE_T	*v_arg, *v_wid, *v_prec;
	int	rval = 0;

	nprt = 0;
	w_ind = 0; u_wid = 0;
	p_ind = 0; u_prec = 0; 
	i_argc = 0;

	l_fmt = strlen( fmt );
	efp = &fmt[ l_fmt - 1 ];
	type = *efp--;

	sp = size;
	if( *efp == 'h' || *efp == 'l' || *efp == 'L' )
		*sp++ = *efp--;
	if( *efp == 'l' )
		*sp++ = *efp--;
	*sp = '\0';

	strncpy( work, &fmt[ 1 ], efp - &fmt[ 1 ] + 1 );
	work[ efp - &fmt[ 1 ] + 1 ] = '\0';

	if( dot = strchr( work, '.' ) ){
		strcpy( prec, dot );
		*dot = '\0'; 
		if( prec[ 1 ] == '*' ){
			p_ind = 1;
			if( isdigit( prec[ 2 ] ) )
				u_prec = atoi( &prec[ 2 ] );
			else{
				u_prec = -1;
				i_argc++;
			}
		}
	}else
		*prec = '\0';

	if( star = strchr( work, '*' ) ){
		w_ind = 1;
		strcpy( width, star );
		*star = '\0';
		if( isdigit( width[ 1 ] ) ) 
			u_wid = atoi( &width[ 1 ] );
		else{
			u_wid = -1;
			i_argc++;
		}
		if( dollar = strchr( work, '$' ) ){
			dollar++;
			strncpy( posp, work, dollar - work );
			posp[ dollar - work ] = '\0'; 
			strcpy( flags, dollar );
		}else{
			*posp = '\0';
			strcpy( flags, work );
		}
	}else if( dollar = strchr( work, '$' ) ){
		dollar++;
		strncpy( posp, work, dollar - work );
		posp[ dollar - work ] = '\0'; 
		wp = dollar + strspn( dollar, FMTFLAGS );
		strncpy( flags, dollar, wp - dollar );
		flags[ wp - dollar ] = '\0';
		strcpy( width, wp );
	}else{
		*posp = '\0';
		wp = work + strspn( work, FMTFLAGS );
		strncpy( flags, work, wp - work );
		flags[ wp - work ] = '\0';
		strcpy( width, wp );
	}

	u_arg = *posp ? atoi( posp ) : -1 - i_argc;

	if( w_ind && p_ind ){
		if( u_wid == -1 && u_prec == -1 )
			u_prec = -2;
	}

	v_arg = NULL;
	r_arg = u_arg < 0 ? *c_arg - u_arg : u_arg;
	if( r_arg < 1 || r_arg >= n_args ){
		rm_emsg_lineno = ip->i_lineno;
		sprintf( emsg, "do_fmt: No such argument (%d).", r_arg );
		RM_errormsg( 0, emsg );
		rval = 1;
		goto DONE;
	}else
		v_arg = &mem[ mp + r_arg + 1 ];

	v_wid = NULL;
	r_wid = u_wid < 0 ? *c_arg - u_wid : u_wid;
	if( r_wid < 0 || r_wid >= n_args ){
		rm_emsg_lineno = ip->i_lineno;
		sprintf( emsg, "do_fmt: No such width argument (%d).", r_wid );
		RM_errormsg( 0, emsg );
		rval = 1;
		goto DONE;
	}else if( r_wid != 0 ){
		v_wid = &mem[ mp + r_wid + 1 ];
		if( v_wid->v_type != T_INT ){
			rm_emsg_lineno = ip->i_lineno;
			RM_errormsg( 0, "do_fmt: Ind. width must be int." );
			rval = 1;
			goto DONE;
		}
	}

	v_prec = NULL;
	r_prec = u_prec < 0 ? *c_arg - u_prec : u_prec;
	if( r_prec < 0 || r_prec >= n_args ){
		rm_emsg_lineno = ip->i_lineno;
		sprintf( emsg,
			"do_fmt: No such prec. argument (%d).", r_prec );
		RM_errormsg( 0, emsg );
		rval = 1;
		goto DONE;
	}else if( r_prec != 0 ){
		v_wid = &mem[ mp + r_prec + 1 ];
		if( v_prec->v_type != T_INT ){
			rm_emsg_lineno = ip->i_lineno;
			RM_errormsg( 0, "do_fmt: Ind. prec. must be int." );
			rval = 1;
			goto DONE;
		}
	}

	u_argc = 1;
	u_argc += r_wid != 0 ? 1 : 0;
	u_argc += r_prec != 0 ? 1 : 0;

	*c_arg += *posp ? i_argc : i_argc + 1;

	switch( type ){
	case 'b' :
	case 'B' :
	case 'e' :
	case 'E' :
	case 'f' :
	case 'g' :
	case 'G' :
		if( v_arg->v_type != T_FLOAT ){
			rm_emsg_lineno = ip->i_lineno;
			sprintf( emsg,
				"do_fmt: '%c' format requires float arg.",
				type );
			RM_errormsg( 0, emsg );
			rval = 1;
			goto DONE;
		}	
		switch( u_argc ){
		case 1 :
			nprt = sprintf( sbp, fmt, v_arg->v_value.v_dval );
			break;
		case 2 :
			nprt = sprintf( sbp, fmt,
				v_wid ? v_wid->v_value.v_ival
					: v_prec->v_value.v_ival,
				v_arg->v_value.v_dval );
			break;
		case 3 :
			nprt = sprintf( sbp, fmt,
				v_wid->v_value.v_ival, v_prec->v_value.v_ival,
				v_arg->v_value.v_dval );
			break;
		default :
			break;
		}
		break;
	case 'd' :
	case 'i' :
		if( v_arg->v_type != T_INT ){
			rm_emsg_lineno = ip->i_lineno;
			sprintf( emsg,
				"do_fmt: '%c' format requires int arg.",
				type );
			RM_errormsg( 0, emsg );
			rval = 1;
			goto DONE;
		}	
		switch( u_argc ){
		case 1 :
			nprt = sprintf( sbp, fmt, v_arg->v_value.v_ival );
			break;
		case 2 :
			nprt = sprintf( sbp, fmt,
				v_wid ? v_wid->v_value.v_ival
					: v_prec->v_value.v_ival,
				v_arg->v_value.v_ival );
			break;
		case 3 :
			nprt = sprintf( sbp, fmt,
				v_wid->v_value.v_ival, v_prec->v_value.v_ival,
				v_arg->v_value.v_ival );
			break;
		default :
			break;
		}
		break;
	case 'o' :
	case 'u' :
	case 'x' :
	case 'X' :
		if( v_arg->v_type != T_INT ){
			rm_emsg_lineno = ip->i_lineno;
			sprintf( emsg,
				"do_fmt: '%c' format requires int arg.",
				type );
			RM_errormsg( 0, emsg );
			rval = 1;
			goto DONE;
		}	
		switch( u_argc ){
		case 1 :
			nprt = sprintf( sbp, fmt,
				( unsigned )v_arg->v_value.v_ival );
			break;
		case 2 :
			nprt = sprintf( sbp, fmt,
				v_wid ? v_wid->v_value.v_ival
					: v_prec->v_value.v_ival,
				( unsigned )v_arg->v_value.v_ival );
			break;
		case 3 :
			nprt = sprintf( sbp, fmt,
				v_wid->v_value.v_ival, v_prec->v_value.v_ival,
				( unsigned )v_arg->v_value.v_ival );
			break;
		default :
			break;
		}
		break;
	case 's' :
	case 'S' :
		if( v_arg->v_type != T_STRING ){
			rm_emsg_lineno = ip->i_lineno;
			sprintf( emsg,
				"do_fmt: '%c' format requires string/seq arg.",
				type );
			RM_errormsg( 0, emsg );
			rval = 1;
			goto DONE;
		}	
		switch( u_argc ){
		case 1 :
			nprt = sprintf( sbp, fmt,
				v_arg->v_value.v_pval );
			break;
		case 2 :
			nprt = sprintf( sbp, fmt,
				v_wid ? v_wid->v_value.v_ival
					: v_prec->v_value.v_ival,
				v_arg->v_value.v_pval );
			break;
		case 3 :
			nprt = sprintf( sbp, fmt,
				v_wid->v_value.v_ival, v_prec->v_value.v_ival,
				v_arg->v_value.v_pval );
			break;
		default :
			break;
		}
		break;
	case 'n' :
		if( v_arg->v_type == T_INT )
			v_arg->v_value.v_ival = nprt;
		else if( v_arg->v_type == T_FLOAT )
			v_arg->v_value.v_dval = nprt;
		else{
			rm_emsg_lineno = ip->i_lineno;
			sprintf( emsg, "do_fmt: '%c' format requires int arg.",
				type );
			RM_errormsg( 0, emsg );
			rval = 1;
			goto DONE;
		}
		break;
	case '%' :
		*sbp = '%';
		sbp[ 1 ] = '\0';
		nprt++;
		break;
	case 'c' :
	case 'C' :
	case 'p' :
	default :
		rm_emsg_lineno = ip->i_lineno;
		sprintf( emsg, "do_fmt: '%c' unsupported format.", type );
		RM_errormsg( 0, emsg );
		rval = 1;
		goto DONE;
		break;
	}

DONE : ;

	sbp += nprt;
	*sbp = '\0';

	return( rval );
}

static	void	do_strf( INST_T *ip )
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
		rm_emsg_lineno = ip->i_lineno;;
		sprintf( emsg, "do_strf: no such descriptor %d.", index );
		RM_errormsg( 1, emsg );
	}
	stp = &rm_descr[ index ];
	if( pos == UNDEF )
		pos = 1;
	else if( pos < 0 ){
		rm_emsg_lineno = ip->i_lineno;
		RM_errormsg( 1, "do_strf: pos must be > 0." );
	}else if( stp->s_matchlen == 0 ){
		pos = 1;	
	}else if( pos > stp->s_matchlen ){
		rm_emsg_lineno = ip->i_lineno;
		sprintf( emsg, "do_strf: pos must be <= %d.\n",
			stp->s_matchlen );
		RM_errormsg( 1, emsg );
	}
	pos--;
	if( len == 0 ){ 
		rm_emsg_lineno = ip->i_lineno;
		RM_errormsg( 1, "do_strf: len must be > 0." );
	}else if( len == UNDEF )	/* rest of string */
		len = stp->s_matchlen - pos;
	else
		len = MIN( stp->s_matchlen - pos, len );
	cp = ( char * )tm_malloc( ( len + 1 ) * sizeof( char ), "do_strf" );
	if( cp == NULL ){
		rm_emsg_lineno = ip->i_lineno;
		RM_errormsg( 1, "do_strf: can't allocate cp." );
		exit( 1 );
	}
	strncpy( cp, &sc_sbuf[ stp->s_matchoff + pos ], len );
	cp[ len ] = '\0';
	sp -= 2; 
	mem[ sp ].v_type = T_STRING;
	mem[ sp ].v_value.v_pval = cp; 
	esp--;
}

static	void	do_lda( INST_T *ip )
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
		rm_emsg_lineno = ip->i_lineno;
		sprintf( emsg, "do_lda: variable '%s' is readonly.", 
			idp->i_name );
		RM_errormsg( 1, emsg );
	}else{
		v_top->v_type = T_IDENT;
		v_top->v_value.v_pval = idp;
	}
}

static	void	do_lod( INST_T *ip )
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
			ip->i_val.v_value.v_pval );
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
			v_top->v_value.v_dval = idp->i_val.v_value.v_dval;
			break;
		case T_STRING :
			cp = ( char * )tm_malloc(
				strlen(idp->i_val.v_value.v_pval)+1, "do_lod" );
			if( cp == NULL ){
				rm_emsg_lineno = UNDEF;
				RM_errormsg( 1, "do_lod: can't allocate cp." );
			}
			strcpy( cp, idp->i_val.v_value.v_pval );
			v_top->v_type = T_STRING;
			v_top->v_value.v_pval = cp;
			break;
		default :
			rm_emsg_lineno = ip->i_lineno;
			RM_errormsg( 1, "do_lod: type mismatch." );
			break;
		}
	}
}

static	void	do_ldc( INST_T *ip )
{
	VALUE_T	*v_top;
	char	*cp;
	int	idx;

	sp++;
	v_top = &mem[ sp ];
	switch( ip->i_val.v_type ){
	case T_INT :
		v_top->v_type = T_INT;
		v_top->v_value.v_ival = ip->i_val.v_value.v_ival;
		break;
	case T_FLOAT :
		v_top->v_type = T_FLOAT;
		v_top->v_value.v_dval = ip->i_val.v_value.v_dval;
		break;
	case T_STRING :
		v_top->v_type = T_STRING;
		cp = ( char * )tm_malloc(
			strlen( ip->i_val.v_value.v_pval ) + 1, "do_ldc" );
		if( cp == NULL ){
			rm_emsg_lineno = ip->i_lineno;
			RM_errormsg( 1, "do_ldc: can't allocate cp." );
		}
		strcpy( cp, ip->i_val.v_value.v_pval );
		v_top->v_value.v_pval = cp;
		break;
	case	T_POS :
		v_top->v_type = T_INT;
		idx = estk[ esp ];
		v_top->v_value.v_ival = rm_descr[ idx ].s_matchlen;
		
		break;
	case 	T_PAIRSET :
		v_top->v_type = T_PAIRSET;
		v_top->v_value.v_pval = ip->i_val.v_value.v_pval;
		break;
	default :
		rm_emsg_lineno = ip->i_lineno;
		RM_errormsg( 1, "do_ldc: type mismatch." );
		break;
	}
}

static	void	do_sto( INST_T *ip )
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
		idp->i_type = T_FLOAT;
		idp->i_val.v_type = T_FLOAT;
		idp->i_val.v_value.v_dval = v_top->v_value.v_dval;
		break;
	case T_IJ( T_UNDEF, T_STRING ):
		cp = ( char * )tm_malloc(
			strlen( v_top->v_value.v_pval ) + 1, "do_sto: US" );
		if( cp == NULL ){
			rm_emsg_lineno = UNDEF;
			RM_errormsg( 1, "do_sto: can't allocate new string." );
		}
		strcpy( cp, v_top->v_value.v_pval );
		idp->i_type = T_STRING;
		idp->i_val.v_type = T_STRING;
		idp->i_val.v_value.v_pval = cp;
		tm_free( v_top->v_value.v_pval );
		break;
	case T_IJ( T_INT, T_INT ) :
		idp->i_val.v_value.v_ival = v_top->v_value.v_ival;
		break;
	case T_IJ( T_INT, T_FLOAT ) :
		idp->i_val.v_value.v_ival = v_top->v_value.v_dval;
		break;
	case T_IJ( T_FLOAT, T_INT ) :
		idp->i_val.v_value.v_dval = v_top->v_value.v_ival;
		break;
	case T_IJ( T_FLOAT, T_FLOAT ) :
		idp->i_val.v_value.v_dval = v_top->v_value.v_dval;
		break;
	case T_IJ( T_STRING, T_STRING ) :
		cp = ( char * )tm_malloc(
			strlen( v_top->v_value.v_pval ) + 1, "do_sto: SS" );
		if( cp == NULL ){
			rm_emsg_lineno = UNDEF;
			RM_errormsg( 1, "do_sto: can't allocate new string." );
		}
		strcpy( cp, v_top->v_value.v_pval );
		tm_free( idp->i_val.v_value.v_pval );
		idp->i_val.v_value.v_pval = cp;
		tm_free( v_top->v_value.v_pval );
		break;
	default :
		rm_emsg_lineno = ip->i_lineno;
		RM_errormsg( 1, "do_sto: type mismatch." );
		break;
	}
}

static	void	do_and( INST_T *ip )
{
	VALUE_T	*v_top;
	int	t_top, rv;
	char	*cp;

	v_top = &mem[ sp ];
	t_top = v_top->v_type;

	switch( t_top ){
	case T_INT :
		rv = v_top->v_value.v_ival = v_top->v_value.v_ival != 0;
		break;
	case T_FLOAT :
		rv = v_top->v_value.v_ival = v_top->v_value.v_dval != 0.0;
		break;
	case T_STRING :
/*
		rv = v_top->v_value.v_ival =
			*( char * )v_top->v_value.v_pval != '\0';
*/
		cp = ( char * )v_top->v_value.v_pval;
		rv = v_top->v_value.v_ival =
			*( char * )v_top->v_value.v_pval != '\0';
		tm_free( cp );
		break;
	default :
		rm_emsg_lineno = ip->i_lineno;
		RM_errormsg( 1, "do_and: type mismatch." );
		break;
	}
	if( !rv )
		pc = ip->i_val.v_value.v_ival;
}

static	void	do_ior( INST_T *ip )
{
	VALUE_T	*v_top;
	int	t_top, rv;
	char	*cp;

	v_top = &mem[ sp ];
	t_top = v_top->v_type;

	switch( t_top ){
	case T_INT :
		rv = v_top->v_value.v_ival = v_top->v_value.v_ival != 0;
		break;
	case T_FLOAT :
		rv = v_top->v_value.v_ival = v_top->v_value.v_dval != 0.0;
		break;
	case T_STRING :
/*
		rv = v_top->v_value.v_ival =
			*( char * )v_top->v_value.v_pval != '\0';
*/
		cp = ( char * )v_top->v_value.v_pval;
		rv = v_top->v_value.v_ival =
			*( char * )v_top->v_value.v_pval != '\0';
		tm_free( cp );
		break;
	default :
		rm_emsg_lineno = ip->i_lineno;
		RM_errormsg( 1, "do_ior: type mismatch." );
		break;
	}
	if( rv )
		pc = ip->i_val.v_value.v_ival;
}

static	void	do_not( INST_T *ip )
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
		v_top->v_value.v_ival = !( v_top->v_value.v_dval != 0.0 );
		break;
	case T_STRING :
		v_top->v_value.v_ival =
			!( *( char * )v_top->v_value.v_pval != '\0' );
		break;
	default :
		rm_emsg_lineno = ip->i_lineno;
		RM_errormsg( 1, "do_not: type mismatch." );
		break;
	}
}

static	void	do_mat( INST_T *ip )
{
	VALUE_T	*v_tm1, *v_top;
	int	t_tm1, t_top;
	char	*s_tm1, *s_top;
#define	EXPBUF_SIZE	256
	static	char	expbuf[ EXPBUF_SIZE ];

	v_top = &mem[ sp ];
	t_top = v_top->v_type;
	sp--;
	v_tm1 = &mem[ sp ];
	t_tm1 = v_tm1->v_type;
	v_tm1->v_type = T_INT;

	switch( T_IJ( t_tm1, t_top ) ){
	case T_IJ( T_STRING, T_STRING ) :
		s_tm1 = v_tm1->v_value.v_pval;
		s_top = v_top->v_value.v_pval;
		compile( s_top, expbuf, &expbuf[ EXPBUF_SIZE ], '\0' );
		v_tm1->v_value.v_ival = step( s_tm1, expbuf );
		tm_free( s_top );
		tm_free( s_tm1 );
		break;
	default :
		rm_emsg_lineno = ip->i_lineno;
		RM_errormsg( 1, "do_mat: type mismatch." );
		break;
	}
}

static	void	do_ins( INST_T *ip )
{
	VALUE_T	*v_bases[ 4 ], *v_top;
	char	*s_bases[ 4 ];
	int	l0, l_bases[ 4 ];
	int	i, n_bases;
	int	rv;
	PAIRSET_T	*ps_top;

	n_bases = sp - mp - 1;
	if( n_bases < 2 || n_bases > 4 ){
		rm_emsg_lineno = ip->i_lineno;
		sprintf( emsg, "do_ins: pair has %d bases, requires %d-%d.",
			n_bases, 2, 4 );
		RM_errormsg( 1, emsg );
	}

	v_top = &mem[ sp ];
	if( v_top->v_type != T_PAIRSET ){
		rm_emsg_lineno = ip->i_lineno;
		RM_errormsg( 1,
			"do_ins: rhs of \"in\" must have type pairset." );
	}
	ps_top = v_top->v_value.v_pval;
	for( l0 = UNDEF, i = 0; i < n_bases; i++ ){
		v_bases[ i ] = &mem[ mp + 1 + i ];
		if( v_bases[ i ]->v_type != T_STRING ){
			rm_emsg_lineno = ip->i_lineno;
			RM_errormsg( 1,
				"do_ins: pair elements must be type string." );
		}
		l_bases[ i ] = strlen( v_bases[ i ]->v_value.v_pval );
		if( l0 == UNDEF )
			l0 = l_bases[ i ]; 
		else if( l_bases[ i ] != l0 ){
			rm_emsg_lineno = ip->i_lineno;
			RM_errormsg( 1,
		"do_ins: all pair elements must have the same length." );
		}
		s_bases[ i ] = v_bases[ i ]->v_value.v_pval;
	}

	switch( n_bases ){
	case 2 :
		for( rv = 1, i = 0; i < l0; i++ ){
			if( !RM_paired( ps_top, s_bases[0][i], s_bases[1][i] ) )
			{
				rv = 0;
				break;
			}
		}
		break;
	case 3 :
		for( rv = 1, i = 0; i < l0; i++ ){
			if( !RM_triple( ps_top,
				s_bases[0][i], s_bases[1][i], s_bases[2][i] ) )
			{
				rv = 0;
				break;
			}
		}
		break;
	case 4 :
		for( rv = 1, i = 0; i < l0; i++ ){
			if( !RM_quad( ps_top,
				s_bases[0][i], s_bases[1][i],
				s_bases[2][i], s_bases[3][i] ) )
			{
				rv = 0;
				break;
			}
		}
		break;
	}

	for( i = 0; i < n_bases; i++ ){
		tm_free( mem[ mp + 1 + i ].v_value.v_pval );
	}

	sp = mp;
	mp = mem[ mp ].v_value.v_ival;
	mem[ sp ].v_type = T_INT;
	mem[ sp ].v_value.v_ival = rv;
}

static	void	do_gtr( INST_T *ip )
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
			v_tm1->v_value.v_ival > v_top->v_value.v_dval;
		break;
	case T_IJ( T_FLOAT, T_INT ) :
		v_tm1->v_value.v_ival =
			v_tm1->v_value.v_dval > v_top->v_value.v_ival;
		break;
	case T_IJ( T_FLOAT, T_FLOAT ) :
		v_tm1->v_value.v_ival =
			v_tm1->v_value.v_dval > v_top->v_value.v_dval;
		break;
	case T_IJ( T_STRING, T_STRING ) :
		s_tm1 = v_tm1->v_value.v_pval;
		s_top = v_top->v_value.v_pval;
		v_tm1->v_value.v_ival = strcmp( s_tm1, s_top ) > 0;
		tm_free( s_top );
		tm_free( s_tm1 );
		break;
	default :
		rm_emsg_lineno = ip->i_lineno;
		RM_errormsg( 1, "do_gtr: type mismatch." );
		break;
	}
}

static	void	do_geq( INST_T *ip )
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
			v_tm1->v_value.v_ival >= v_top->v_value.v_dval;
		break;
	case T_IJ( T_FLOAT, T_INT ) :
		v_tm1->v_value.v_ival =
			v_tm1->v_value.v_dval >= v_top->v_value.v_ival;
		break;
	case T_IJ( T_FLOAT, T_FLOAT ) :
		v_tm1->v_value.v_ival =
			v_tm1->v_value.v_dval >= v_top->v_value.v_dval;
		break;
	case T_IJ( T_STRING, T_STRING ) :
		s_tm1 = v_tm1->v_value.v_pval;
		s_top = v_top->v_value.v_pval;
		v_tm1->v_value.v_ival = strcmp( s_tm1, s_top ) >= 0;
		tm_free( s_top );
		tm_free( s_tm1 );
		break;
	default :
		rm_emsg_lineno = ip->i_lineno;
		RM_errormsg( 1, "do_geq: type mismatch." );
		exit( 1 );
		break;
	}
}

static	void	do_equ( INST_T *ip )
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
			v_tm1->v_value.v_ival == v_top->v_value.v_dval;
		break;
	case T_IJ( T_FLOAT, T_INT ) :
		v_tm1->v_value.v_ival =
			v_tm1->v_value.v_dval == v_top->v_value.v_ival;
		break;
	case T_IJ( T_FLOAT, T_FLOAT ) :
		v_tm1->v_value.v_ival =
			v_tm1->v_value.v_dval == v_top->v_value.v_dval;
		break;
	case T_IJ( T_STRING, T_STRING ) :
		s_tm1 = v_tm1->v_value.v_pval;
		s_top = v_top->v_value.v_pval;
		v_tm1->v_value.v_ival = strcmp( s_tm1, s_top ) == 0;
		tm_free( s_top );
		tm_free( s_tm1 );
		break;
	default :
		rm_emsg_lineno = ip->i_lineno;
		RM_errormsg( 1, "do_equ: type mismatch." );
		break;
	}
}

static	void	do_neq( INST_T *ip )
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
			v_tm1->v_value.v_ival != v_top->v_value.v_dval;
		break;
	case T_IJ( T_FLOAT, T_INT ) :
		v_tm1->v_value.v_ival =
			v_tm1->v_value.v_dval != v_top->v_value.v_ival;
		break;
	case T_IJ( T_FLOAT, T_FLOAT ) :
		v_tm1->v_value.v_ival =
			v_tm1->v_value.v_dval != v_top->v_value.v_dval;
		break;
	case T_IJ( T_STRING, T_STRING ) :
		s_tm1 = v_tm1->v_value.v_pval;
		s_top = v_top->v_value.v_pval;
		v_tm1->v_value.v_ival = strcmp( s_tm1, s_top ) != 0;
		tm_free( s_top );
		tm_free( s_tm1 );
		break;
	default :
		rm_emsg_lineno = ip->i_lineno;
		RM_errormsg( 1, "do_neq: type mismatch." );
		break;
	}
}

static	void	do_leq( INST_T *ip )
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
			v_tm1->v_value.v_ival <= v_top->v_value.v_dval;
		break;
	case T_IJ( T_FLOAT, T_INT ) :
		v_tm1->v_value.v_ival =
			v_tm1->v_value.v_dval <= v_top->v_value.v_ival;
		break;
	case T_IJ( T_FLOAT, T_FLOAT ) :
		v_tm1->v_value.v_ival =
			v_tm1->v_value.v_dval <= v_top->v_value.v_dval;
		break;
	case T_IJ( T_STRING, T_STRING ) :
		s_tm1 = v_tm1->v_value.v_pval;
		s_top = v_top->v_value.v_pval;
		v_tm1->v_value.v_ival = strcmp( s_tm1, s_top ) <= 0;
		tm_free( s_top );
		tm_free( s_tm1 );
		break;
	default :
		rm_emsg_lineno = ip->i_lineno;
		RM_errormsg( 1, "do_leq: type mismatch." );
		break;
	}
}

static	void	do_les( INST_T *ip )
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
			v_tm1->v_value.v_ival < v_top->v_value.v_dval;
		break;
	case T_IJ( T_FLOAT, T_INT ) :
		v_tm1->v_value.v_ival =
			v_tm1->v_value.v_dval < v_top->v_value.v_ival;
		break;
	case T_IJ( T_FLOAT, T_FLOAT ) :
		v_tm1->v_value.v_ival =
			v_tm1->v_value.v_dval < v_top->v_value.v_dval;
		break;
	case T_IJ( T_STRING, T_STRING ) :
		s_tm1 = v_tm1->v_value.v_pval;
		s_top = v_top->v_value.v_pval;
		v_tm1->v_value.v_ival = strcmp( s_tm1, s_top ) < 0;
		tm_free( s_top );
		tm_free( s_tm1 );
		break;
	default :
		rm_emsg_lineno = ip->i_lineno;
		RM_errormsg( 1, "do_les: type mismatch." );
		break;
	}
}

static	void	do_add( INST_T *ip )
{
	VALUE_T	*v_tm1, *v_top;
	int	t_tm1, t_top;
	char	*s_tm1, *s_top;
	char	*cp;

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
		v_tm1->v_value.v_ival += v_top->v_value.v_dval;
		break;
	case T_IJ( T_FLOAT, T_INT ) :
		v_tm1->v_value.v_dval += v_top->v_value.v_ival;
		break;
	case T_IJ( T_FLOAT, T_FLOAT ) :
		v_tm1->v_value.v_dval += v_top->v_value.v_dval;
		break;
	case T_IJ( T_STRING, T_STRING ) :
		s_tm1 = v_tm1->v_value.v_pval;
		s_top = v_top->v_value.v_pval;
		cp = ( char * )tm_malloc(
			strlen( s_tm1 ) + strlen( s_top ) + 1, "do_add" );
		if( cp == NULL ){
			rm_emsg_lineno = UNDEF;
			RM_errormsg( 1, "do_add: can't allocate new string." );
		}
		strcpy( cp, s_tm1 );
		strcat( cp, s_top );
		v_tm1->v_value.v_pval = cp;
		tm_free( s_top );
		tm_free( s_tm1 );
		break;
	default :
		rm_emsg_lineno = ip->i_lineno;
		RM_errormsg( 1, "do_add: type mismatch." );
		break;
	}
}

static	void	do_sub( INST_T *ip )
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
		v_tm1->v_value.v_ival -= v_top->v_value.v_dval;
		break;
	case T_IJ( T_FLOAT, T_INT ) :
		v_tm1->v_value.v_dval -= v_top->v_value.v_ival;
		break;
	case T_IJ( T_FLOAT, T_FLOAT ) :
		v_tm1->v_value.v_dval -= v_top->v_value.v_dval;
		break;
	default :
		rm_emsg_lineno = ip->i_lineno;
		RM_errormsg( 1, "do_sub: type mismatch." );
		break;
	}
}

static	void	do_mul( INST_T *ip )
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
		v_tm1->v_value.v_ival *= v_top->v_value.v_dval;
		break;
	case T_IJ( T_FLOAT, T_INT ) :
		v_tm1->v_value.v_dval *= v_top->v_value.v_ival;
		break;
	case T_IJ( T_FLOAT, T_FLOAT ) :
		v_tm1->v_value.v_dval *= v_top->v_value.v_dval;
		break;
	default :
		rm_emsg_lineno = ip->i_lineno;
		RM_errormsg( 1, "do_mul: type mismatch." );
		break;
	}
}

static	void	do_div( INST_T *ip )
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
		v_tm1->v_value.v_ival /= v_top->v_value.v_dval;
		break;
	case T_IJ( T_FLOAT, T_INT ) :
		v_tm1->v_value.v_dval /= v_top->v_value.v_ival;
		break;
	case T_IJ( T_FLOAT, T_FLOAT ) :
		v_tm1->v_value.v_dval /= v_top->v_value.v_dval;
		break;
	default :
		rm_emsg_lineno = ip->i_lineno;
		RM_errormsg( 1, "do_div: type mismatch." );
		break;
	}
}

static	void	do_mod( INST_T *ip )
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
		rm_emsg_lineno = ip->i_lineno;
		RM_errormsg( 1, "do_mod: type mismatch." );
		break;
	}
}

static	void	do_neg( INST_T *ip )
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
		v_top->v_value.v_dval = -v_top->v_value.v_dval;
		break;
	default :
		rm_emsg_lineno = ip->i_lineno;
		RM_errormsg( 1, "do_neg: type mismatch." );
		break;
	}
}

static	void	do_i_pp( INST_T *ip )
{
	VALUE_T	*v_top;
	int	t_top;
	IDENT_T	*idp;

	v_top = &mem[ sp ];
	idp = v_top->v_value.v_pval;
	t_top = idp->i_type;
	
	switch( t_top ){
	case T_UNDEF :
		rm_emsg_lineno = ip->i_lineno;
		sprintf( emsg, "do_i_pp: variable '%s' is undefined.",
			idp->i_name );
		RM_errormsg( 1, emsg );
		break;
	case T_INT :
		v_top->v_value.v_ival = ( idp->i_val.v_value.v_ival )++;
		break;
	default :
		rm_emsg_lineno = ip->i_lineno;
		RM_errormsg( 1, "do_i_pp: type mismatch." );
		break;
	}
}

static	void	do_pp_i( INST_T *ip )
{
	VALUE_T	*v_top;
	int	t_top;
	IDENT_T	*idp;

	v_top = &mem[ sp ];
	idp = v_top->v_value.v_pval;
	t_top = idp->i_type;
	
	switch( t_top ){
	case T_UNDEF :
		rm_emsg_lineno = ip->i_lineno;
		sprintf( emsg, "do_pp_i: variable '%s' is undefined.",
			idp->i_name );
		RM_errormsg( 1, emsg );
		break;
	case T_INT :
		v_top->v_value.v_ival = ++( idp->i_val.v_value.v_ival );
		break;
	default :
		rm_emsg_lineno = ip->i_lineno;
		RM_errormsg( 1, "do_pp_i: type mismatch." );
		break;
	}
}

static	void	do_i_mm( INST_T *ip )
{
	VALUE_T	*v_top;
	int	t_top;
	IDENT_T	*idp;

	v_top = &mem[ sp ];
	idp = v_top->v_value.v_pval;
	t_top = idp->i_type;
	
	switch( t_top ){
	case T_UNDEF :
		rm_emsg_lineno = ip->i_lineno;
		sprintf( emsg, "do_i_mm: variable '%s' is undefined.",
			idp->i_name );
		RM_errormsg( 1, emsg );
		break;
	case T_INT :
		v_top->v_value.v_ival = ( idp->i_val.v_value.v_ival )--;
		break;
	default :
		rm_emsg_lineno = ip->i_lineno;
		RM_errormsg( 1, "do_i_mm: type mismatch." );
		exit( 1 );
		break;
	}
}

static	void	do_mm_i( INST_T *ip )
{
	VALUE_T	*v_top;
	int	t_top;
	IDENT_T	*idp;

	v_top = &mem[ sp ];
	idp = v_top->v_value.v_pval;
	t_top = idp->i_type;
	
	switch( t_top ){
	case T_UNDEF :
		rm_emsg_lineno = ip->i_lineno;
		sprintf( emsg, "do_mm_i: variable '%s' is undefined.",
			idp->i_name );
		RM_errormsg( 1, emsg );
		break;
	case T_INT :
		v_top->v_value.v_ival = --( idp->i_val.v_value.v_ival );
		break;
	default :
		rm_emsg_lineno = ip->i_lineno;
		RM_errormsg( 1, "do_mm_i: type mismatch." );
		break;
	}
}

static	int	setupefn( INST_T *ip, int idx, int pos, int idx2, int pos2 )
{
	int	i, p, inc;
	int	off, off5, len;
	STREL_T	*stp, *stp5, *stp3;
	char	*bp;

	stp5 = &rm_descr[ idx ];
	off5 = stp5->s_matchoff;
	stp3 = &rm_descr[ idx2 ];
	for( len = 0, stp = stp5, i = idx; i <= idx2; i++, stp++ ){
		len += stp->s_matchlen;
	}
	len -= pos;
	len -= stp3->s_matchlen - ( pos2 + 1 );
	if( sc_comp ){
		off = sc_slen - stp5->s_matchoff + pos;
		inc = -1;
	}else{
		off = stp5->s_matchoff + 1 + pos;
		inc = 1;
	}

	i = 0;
	bp = &sc_sbuf[ stp5->s_matchoff + pos ];
	for( p = pos; p < stp5->s_matchlen; p++, i++ ){
		rm_bcseq[ i ] = rm_b2bc[ *bp++ ];
		if( stp5->s_type == SYM_H5 ){
			if( !setbp( stp5, p, i, off5, len, rm_basepr ) ){
			}
		}else if( stp5->s_type == SYM_H3 ){	/* treat as ss() */
			rm_basepr[ i ] = UNDEF;
		}else if( stp5->s_type == SYM_SS ){
			rm_basepr[ i ] = UNDEF;
		}else{
			rm_emsg_lineno = ip->i_lineno;
			RM_errormsg( 1, 
			"setupefn: efn() only works on h5/ss/h3 elements." );
			return( 0 );
		} 
		rm_hstnum[ i ] = off + i * inc;
	}
	for( stp = &rm_descr[ stp5->s_index + 1 ]; stp < stp3; stp++ ){
		bp = &sc_sbuf[ stp->s_matchoff ];
		for( p = 0; p < stp->s_matchlen; p++, i++ ){
			rm_bcseq[ i ] = rm_b2bc[ *bp++ ];
			if( stp->s_type == SYM_H5 ){
				if( !setbp( stp, p, i, off5, len, rm_basepr ) ){
				}
			}else if( stp->s_type == SYM_H3 ){
				if( !setbp( stp, p, i, off5, len, rm_basepr ) ){
				}
			}else if( stp->s_type == SYM_SS ){
				rm_basepr[ i ] = UNDEF;
			}else{
				rm_emsg_lineno = ip->i_lineno;
				RM_errormsg( 1, 
			"setupefn: efn() only works on h5/ss/h3 elements." );
				return( 0 );
			}
			rm_hstnum[ i ] = off + i * inc;
		}
	}
	bp = &sc_sbuf[ stp3->s_matchoff ];
	for( p = 0; p <= pos2; p++, i++ ){
		rm_bcseq[ i ] = rm_b2bc[ *bp++ ];
		if( stp3->s_type == SYM_H5 ){	/* treat as ss() */
			rm_basepr[ i ] = UNDEF;
		}else if( stp3->s_type == SYM_H3 ){
			if( !setbp( stp3, p, i, off5, len, rm_basepr ) ){
			}
		}else if( stp3->s_type == SYM_SS ){
			rm_basepr[ i ] = UNDEF;
		}else{
			rm_emsg_lineno = ip->i_lineno;
			RM_errormsg( 1, 
			"setupefn: efn() only works on h5/ss/h3 elements." );
			return( 0 );
		} 
		rm_hstnum[ i ] = off + i * inc;
	}
	rm_l_base = len - 1;

	return( 1 );
}

static	int	setbp( STREL_T *stp, int p, int i, int off, int len,
	int basepr[] )
{
	STREL_T	*stp1;
	int	p1;
	int	bp, bp1;
	int	b, b1;
	PAIRSET_T	*ps;

	bp = p + stp->s_matchoff - off;
	b = sc_sbuf[ p + stp->s_matchoff ];

	stp1 = stp->s_mates[ 0 ];
	p1 = stp1->s_matchlen - p - 1;
	bp1 = p1 + stp1->s_matchoff - off;
	b1 = sc_sbuf[ p1 + stp1->s_matchoff ];

	if( bp1 < 0 || bp1 >= len ){
fprintf( stderr, "out of bounds bp: %4d.%4d\n", bp, bp1 ); 
		return( 0 );
	}else{
		ps = rm_efnusestdbp ? rm_efnstdbp : stp->s_pairset;
		basepr[ i ] = RM_paired( ps, b, b1 ) ? bp1 : UNDEF;
	}

	return( 1 );
}

static	void	mk_stref_name( int sym, char name[] )
{

	switch( sym ){
	case SYM_SE :
		strcpy( name, "se" );
		break;

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
		sprintf( name, " ?%d? ", sym );
		break;
	}
}

static	void	addnode( int lval, NODE_T *np, int l_andor )
{
	VALUE_T	v_node;
	int	sc;

	rm_emsg_lineno = np->n_lineno;
	switch( np->n_sym ){

	case SYM_CALL :
		if( ( sc = is_syscall( np ) ) != UNDEF ){
			v_node.v_type = T_INT;
			v_node.v_value.v_ival = sc;
			addinst( np->n_lineno, OP_SCL, &v_node );
		}else
			addinst( np->n_lineno, OP_FCL, &np->n_val );
		break;
	case SYM_LIST :
		break;
	case SYM_KW_STREF :
	case SYM_IX_STREF :
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
	case SYM_SE :
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
			addinst( np->n_lineno, OP_LDA, &np->n_val );
		}else{
			addinst( np->n_lineno, OP_LOD, &np->n_val );
		}
		break;
	case SYM_INT :
		addinst( np->n_lineno, OP_LDC, &np->n_val );
		break;
	case SYM_FLOAT :
		addinst( np->n_lineno, OP_LDC, &np->n_val );
		break;
	case SYM_STRING :
		addinst( np->n_lineno, OP_LDC, &np->n_val );
		break;
	case SYM_DOLLAR :
		v_node.v_type = T_POS;
		v_node.v_value.v_pval = NULL;
		addinst( np->n_lineno, OP_LDC, &v_node );
		break;
	case SYM_PAIRSET :
		addinst( np->n_lineno, OP_LDC, &np->n_val );
		break;

	case SYM_ASSIGN :
		addinst( np->n_lineno, OP_STO, NULL );
		break;

	case SYM_PLUS_ASSIGN :
		addinst( np->n_lineno, OP_ADD, NULL );
		addinst( np->n_lineno, OP_STO, NULL );
		break;
	case SYM_MINUS_ASSIGN :
		addinst( np->n_lineno, OP_SUB, NULL );
		addinst( np->n_lineno, OP_STO, NULL );
		break;
	case SYM_PERCENT_ASSIGN :
		addinst( np->n_lineno, OP_MOD, NULL );
		addinst( np->n_lineno, OP_STO, NULL );
		break;
	case SYM_STAR_ASSIGN :
		addinst( np->n_lineno, OP_MUL, NULL );
		addinst( np->n_lineno, OP_STO, NULL );
		break;
	case SYM_SLASH_ASSIGN :
		addinst( np->n_lineno, OP_DIV, NULL );
		addinst( np->n_lineno, OP_STO, NULL );
		break;

	case SYM_AND :
		v_lab.v_type = T_INT;
		v_lab.v_value.v_ival = l_andor;
		addinst( np->n_lineno, OP_AND, &v_lab );
		break;
	case SYM_OR :
		v_lab.v_type = T_INT;
		v_lab.v_value.v_ival = l_andor;
		addinst( np->n_lineno, OP_IOR, &v_lab );
		break;
	case SYM_NOT :
		addinst( np->n_lineno, OP_NOT, NULL );
		break;

	case SYM_EQUAL :
		addinst( np->n_lineno, OP_EQU, NULL );
		break;
	case SYM_NOT_EQUAL :
		addinst( np->n_lineno, OP_NEQ, NULL );
		break;
	case SYM_GREATER :
		addinst( np->n_lineno, OP_GTR, NULL );
		break;
	case SYM_GREATER_EQUAL :
		addinst( np->n_lineno, OP_GEQ, NULL );
		break;
	case SYM_LESS :
		addinst( np->n_lineno, OP_LES, NULL );
		break;
	case SYM_LESS_EQUAL :
		addinst( np->n_lineno, OP_LEQ, NULL );
		break;

	case SYM_MATCH :
		addinst( np->n_lineno, OP_MAT, NULL );
		break;
	case SYM_DONT_MATCH :
		addinst( np->n_lineno, OP_MAT, NULL );
		addinst( np->n_lineno, OP_NOT, NULL );
		break;
	case SYM_IN :
		addinst( np->n_lineno, OP_INS, NULL );
		break;

	case SYM_PLUS :
		addinst( np->n_lineno, OP_ADD, NULL );
		break;
	case SYM_MINUS :
		addinst( np->n_lineno, OP_SUB, NULL );
		break;
	case SYM_PERCENT :
		addinst( np->n_lineno, OP_MOD, NULL );
		break;
	case SYM_STAR :
		addinst( np->n_lineno, OP_MUL, NULL );
		break;
	case SYM_SLASH :
		addinst( np->n_lineno, OP_DIV, NULL );
		break;
	case SYM_NEGATE :
		addinst( np->n_lineno, OP_NEG, NULL );
		break;

	case SYM_COLON :
		break;

	case SYM_MINUS_MINUS :
		if( np->n_left ){
			addinst( np->n_lineno, OP_I_MM, NULL );
		}else{
			addinst( np->n_lineno, OP_MM_I, NULL );
		}
		break;
	case SYM_PLUS_PLUS :
		if( np->n_left ){
			addinst( np->n_lineno, OP_I_PP, NULL );
		}else{
			addinst( np->n_lineno, OP_PP_I, NULL );
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

static	void	addinst( int ln, int op, VALUE_T *vp )
{
	INST_T	*ip;
	char	*sp;

	if( pc >= PROG_SIZE ){
		RM_errormsg( 1, "addinst: program size exceeded." );
	}else{
		ip = &prog[ l_prog ];
		l_prog++;
		ip->i_lineno = ln;
		ip->i_op = op;
		if( vp == NULL ){
			ip->i_val.v_type = T_UNDEF;
			ip->i_val.v_value.v_ival = 0;
		}else{
			ip->i_val.v_type = vp->v_type;
			if( vp->v_type == T_INT )
				ip->i_val.v_value.v_ival = vp->v_value.v_ival;
			else if( vp->v_type == T_FLOAT )
				ip->i_val.v_value.v_dval = vp->v_value.v_dval;
			else if( vp->v_type==T_STRING || vp->v_type==T_IDENT ){
				sp = ( char * )malloc( 
					strlen( vp->v_value.v_pval ) + 1 );
				if( sp == NULL ){
					RM_errormsg( 1,
						"addinst: can't allocate sp." );
				}
				strcpy( sp, vp->v_value.v_pval );
				ip->i_val.v_value.v_pval = sp;
			}else if( vp->v_type == T_PAIRSET ){
				ip->i_val.v_value.v_pval = vp->v_value.v_pval;
			}
		}
	}
}

static	void	dumpinst( FILE *fp, int i, INST_T * ip )
{
	VALUE_T	*vp;
	
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
	if( ip->i_op == OP_LDA || ip->i_op == OP_LOD ){
		fprintf( fp, " %s", vp->v_value.v_pval );
	}else if( ip->i_op == OP_SCL ){
		fprintf( fp, " %s", scnames[ vp->v_value.v_ival ] );
	}else if( vp->v_type == T_INT ){
		fprintf( fp, " %d", vp->v_value.v_ival );
	}else if( vp->v_type == T_FLOAT ) 
		fprintf( fp, " %f", vp->v_value.v_dval );
	else if( vp->v_type == T_STRING )
		fprintf( fp, " \"%s\"", vp->v_value.v_pval );
	else if( vp->v_type == T_POS )
		fprintf( fp, " $" );
	else if( vp->v_type == T_IDENT )
		fprintf( fp, " %s", vp->v_value.v_pval );
	else if( vp->v_type == T_PAIRSET ){
		fprintf( fp, " " );
		RM_dump_pairset( fp, vp->v_value.v_pval );
	}
	fprintf( fp, "\n" );
}

static	void	dumpstk( FILE *fp, char msg[] )
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
			fprintf( fp, "F %8.3lf", vp->v_value.v_dval );
			break;
		case T_STRING :
			fprintf( fp, "S \"%s\"", vp->v_value.v_pval );
			break;
		case T_PAIRSET :
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

static	char	*tm_malloc( int size, char id[] )
{
	char	*ptr;

	ptr = ( char * )malloc( size * sizeof( char ) );
#ifdef	MEMDEBUG
	if( tm_n_mem < TM_MEM_SIZE ){
		tm_mem[ tm_n_mem ].t_freed = 0;
		tm_mem[ tm_n_mem ].t_id = id;
		tm_mem[ tm_n_mem ].t_pc = pc - 1;
		tm_mem[ tm_n_mem ].t_addr = ptr;
		tm_n_mem++;
	}
#endif
	return( ptr );
}

static	void	tm_free( void *ptr )
{
#ifdef	MEMDEBUG
	int	i;
#endif

	free( ptr );
#ifdef	MEMDEBUG
	for( i = 0;i < tm_n_mem; i++ ){
		if( tm_mem[ i ].t_addr == ptr ){
			tm_mem[ i ].t_freed = 1;
		}
	}
#endif
}

static	void	tm_report( void )
{
	int	i;
	static	int	cnt = 0;

	for( i = 0; i < tm_n_mem; i++ ){
		fprintf( stderr, "tm_mem[%4d]:", i );
		fprintf( stderr, " addr = %8p, freed = %d, pc = %4d, id = %s\n",
			tm_mem[i].t_addr, tm_mem[i].t_freed,
			tm_mem[i].t_pc, tm_mem[i].t_id );
	}
	cnt++;
	if( cnt > 1 )
		exit( 1 );
}
