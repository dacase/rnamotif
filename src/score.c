#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "rnamot.h"
#include "y.tab.h"

#define	ISLVAL(s)	\
	((s)==SYM_ASSIGN||(s)==SYM_PLUS_ASSIGN||(s)==SYM_MINUS_ASSIGN||\
	 (s)==SYM_PERCENT_ASSIGN||(s)==SYM_SLASH_ASSIGN||(s)==SYM_STAR_ASSIGN||\
	 (s)==SYM_PLUS_PLUS||(s)==SYM_MINUS_MINUS)

#define	ISRFLX(s)	\
	((s)==SYM_PLUS_ASSIGN||(s)==SYM_MINUS_ASSIGN||\
	 (s)==SYM_PERCENT_ASSIGN||(s)==SYM_SLASH_ASSIGN||(s)==SYM_STAR_ASSIGN)

#define	CFP	stdout

extern	int	rm_lineno;

#define	LABTAB_SIZE	1000
static	int	labtab[ LABTAB_SIZE ];
static	int	nextlab = 1;
static	int	actlab;
static	VALUE_T	v_lab;

static	int	ifstk[ 100 ];
static	int	ifstkp = 0;

static	int	loopstk[ 100 ];
static	NODE_T	*loopincrstk[ 100 ];
static	int	loopstkp = 0;

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
#define	OP_INCP		30	/* use then incr (i++)	*/
#define	OP_PINC		31	/* incr then use (++i)	*/
#define	OP_DECP		32	/* use then decr (i--)	*/
#define	OP_PDEC		33	/* decr then use (--i)	*/
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

void	SC_link();
void	SC_dump();
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
static	void	mk_stref_name();
static	void	addinst();
static	void	dumpinst();

void	SC_link()
{
	int	i;
	INST_T	*ip;

	for( ip = prog, i = 0; i < l_prog; i++, ip++ ){
		if( ip->i_op == OP_FJP || ip->i_op == OP_JMP ){
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

	fprintf( CFP, "  fjp L%d\n", actlab );
}

void	SC_endaction()
{

	labtab[ actlab ] = l_prog;

	fprintf( CFP, "L%d:\n", actlab );
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

	fprintf( CFP, "  fjp L%d\n", ifstk[ ifstkp - 1 ] );
}

void	SC_else()
{

	addinst( OP_JMP, ifstk[ ifstkp - 1 ] + 1 );
	labtab[ ifstk[ ifstkp - 1 ] ] = l_prog;

	fprintf( CFP, "  jmp L%d\n", ifstk[ ifstkp - 1 ] + 1 );
	fprintf( CFP, "L%d:\n", ifstk[ ifstkp - 1 ] );
}

void	SC_endelse()
{

	labtab[ ifstk[ ifstkp - 1 ] + 1 ] = l_prog;

	fprintf( CFP, "L%d:\n", ifstk[ ifstkp - 1 ] + 1 );

	ifstkp--;
}

void	SC_endif()
{

	labtab[ ifstk[ ifstkp - 1 ] ] = l_prog;

	fprintf( CFP, "L%d:\n", ifstk[ ifstkp - 1 ] );

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

	fprintf( CFP, "L%d:\n", loopstk[ loopstkp - 1 ] );

	SC_mark();
	SC_expr( 0, np );
	v_lab.v_type = T_INT;
	v_lab.v_value.v_ival = loopstk[ loopstkp - 1 ] + 2;
	addinst( OP_FJP, &v_lab );

	fprintf( CFP, "  fjp L%d\n", loopstk[ loopstkp - 1 ] + 2 );
}

void	SC_forincr( np )
NODE_T	*np;
{

	loopincrstk[ loopstkp - 1 ] = np;
}

void	SC_endfor()
{

	labtab[ loopstk[ loopstkp - 1 ] + 1 ] = l_prog;

	fprintf( CFP, "L%d:\n", loopstk[ loopstkp - 1 ] + 1 );

	SC_mark();
	SC_expr( 0, loopincrstk[ loopstkp - 1 ] );
	SC_clear();
	v_lab.v_type = T_INT;
	v_lab.v_value.v_ival = loopstk[ loopstkp - 1 ];
	addinst( OP_JMP, &v_lab );
	labtab[ loopstk[ loopstkp - 1 ] + 2 ] = l_prog;

	fprintf( CFP, "  jmp L%d\n", loopstk[ loopstkp - 1 ] );
	fprintf( CFP, "L%d:\n", loopstk[ loopstkp - 1 ] + 2 );

	loopstkp--;
}

void	SC_while( np )
NODE_T	*np;
{

	loopstk[ loopstkp ] = nextlab;
	loopstkp++;
	nextlab += 3;
	labtab[ loopstk[ loopstkp -1 ] ] = l_prog;

	fprintf( CFP, "L%d:\n", loopstk[ loopstkp - 1 ] );

	SC_mark();
	SC_expr( 0, np );
	v_lab.v_type = T_INT;
	v_lab.v_value.v_ival = loopstk[ loopstkp - 1 ] + 2;
	addinst( OP_FJP, &v_lab );
	fprintf( CFP, "  fjp L%d\n", loopstk[ loopstkp - 1 ] + 2 );
}

void	SC_endwhile()
{

	v_lab.v_type = T_INT;
	v_lab.v_value.v_ival = loopstk[ loopstkp - 1 ];
	addinst( OP_JMP, &v_lab );
	labtab[ loopstk[ loopstkp - 1 ] + 2 ] = l_prog;

	fprintf( CFP, "  jmp L%d\n", loopstk[ loopstkp - 1 ] );
	fprintf( CFP, "L%d:\n", loopstk[ loopstkp - 1 ] + 2 );

	loopstkp--;
}

void	SC_break()
{

	v_lab.v_type = T_INT;
	v_lab.v_value.v_ival = loopstk[ loopstkp - 1 ] + 1;
	addinst( OP_JMP, &v_lab );
	fprintf( CFP, "  jmp L%d\n", loopstk[ loopstkp - 1 ] + 1 );
}

void	SC_continue()
{

	v_lab.v_type = T_INT;
	v_lab.v_value.v_ival = loopstk[ loopstkp - 1 ];
	addinst( OP_JMP, &v_lab );
	fprintf( CFP, "  jmp L%d\n", loopstk[ loopstkp - 1 ] );
}

void	SC_accept()
{

	addinst( OP_ACPT, NULL );
	fprintf( CFP, "  acpt\n" );
}

void	SC_reject()
{

	addinst( OP_RJCT, NULL );
	fprintf( CFP, "  rjct\n" );
}

void	SC_mark()
{

	addinst( OP_MRK, NULL );
	fprintf( CFP, "  mrk\n" );
}

void	SC_clear()
{

	addinst( OP_CLS, 0 );
	fprintf( CFP, "  clr\n" );
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
			fprintf( CFP, "  mrk\n" );
		}else if( np->n_sym == SYM_STREF ){
			v_expr.v_type = T_INT;
			v_expr.v_value.v_ival = np->n_left->n_sym;
			addinst( OP_STRF, &v_expr );
			mk_stref_name( np->n_left->n_sym, name );
			fprintf( CFP, "  strf %s\n", name );
		}else if( np->n_sym == SYM_LCURLY ){
			addinst( OP_MRK, NULL );
			fprintf( CFP, "  mrk\n" );
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

			fprintf( CFP, "L%d:\n", l_andor );

		}else{
			SC_expr( 0, np->n_right );
			SC_node( lval, np, 0 );
		}

		if( np->n_sym == SYM_STREF ){
			addinst( OP_STRF, NULL );
			fprintf( CFP, "  strf cls\n" );
		}else if( np->n_sym == SYM_LCURLY ){
			addinst( OP_PRST, NULL );
			fprintf( CFP, "  prst\n" );
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
		addinst( OP_FCL, NULL );
		fprintf( CFP, "  fcl %s\n", np->n_val.v_value.v_pval );
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
			fprintf( CFP, "  lda %s\n", np->n_val.v_value.v_pval );
		}else{
			addinst( OP_LOD, &np->n_val );
			fprintf( CFP, "  lod %s\n", np->n_val.v_value.v_pval );
		}
		break;
	case SYM_INT :
		addinst( OP_LDC, &np->n_val );
		fprintf( CFP, "  ldc %d\n", np->n_val.v_value.v_ival );
		break;
	case SYM_FLOAT :
		addinst( OP_LDC, &np->n_val );
		fprintf( CFP, "  ldcR\n" );
		break;
	case SYM_STRING :
		addinst( OP_LDC, &np->n_val );
		fprintf( CFP, "  ldc \"%s\"\n", np->n_val.v_value.v_pval );
		break;
	case SYM_DOLLAR :
		addinst( OP_LDC, NULL );
		fprintf( CFP, "  lod $\n" );
		break;

	case SYM_ASSIGN :
		addinst( OP_STO, NULL );
		fprintf( CFP, "  sto\n" );
		break;

	case SYM_PLUS_ASSIGN :
		addinst( OP_ADD, NULL );
		addinst( OP_STO, NULL );
		fprintf( CFP, "  add\n" );
		fprintf( CFP, "  sto\n" );
		break;
	case SYM_MINUS_ASSIGN :
		addinst( OP_SUB, NULL );
		addinst( OP_STO, NULL );
		fprintf( CFP, "  sub\n" );
		fprintf( CFP, "  sto\n" );
		break;
	case SYM_PERCENT_ASSIGN :
		addinst( OP_MOD, NULL );
		addinst( OP_STO, NULL );
		fprintf( CFP, "  mod\n" );
		fprintf( CFP, "  sto\n" );
		break;
	case SYM_STAR_ASSIGN :
		addinst( OP_MUL, NULL );
		addinst( OP_STO, NULL );
		fprintf( CFP, "  mul\n" );
		fprintf( CFP, "  sto\n" );
		break;
	case SYM_SLASH_ASSIGN :
		addinst( OP_DIV, NULL );
		addinst( OP_STO, NULL );
		fprintf( CFP, "  div\n" );
		fprintf( CFP, "  sto\n" );
		break;

	case SYM_AND :
		v_lab.v_type = T_INT;
		v_lab.v_value.v_ival = l_andor;
		addinst( OP_AND, &v_lab );
		fprintf( CFP, "  and L%d\n", l_andor );
		break;
	case SYM_OR :
		v_lab.v_type = T_INT;
		v_lab.v_value.v_ival = l_andor;
		addinst( OP_IOR, &v_lab );
		fprintf( CFP, "  ior L%d\n", l_andor );
		break;
	case SYM_NOT :
		addinst( OP_NOT, NULL );
		fprintf( CFP, "  not\n" );
		break;

	case SYM_EQUAL :
		addinst( OP_EQU, NULL );
		fprintf( CFP, "  equ\n" );
		break;
	case SYM_NOT_EQUAL :
		addinst( OP_NEQ, NULL );
		fprintf( CFP, "  neq\n" );
		break;
	case SYM_GREATER :
		addinst( OP_GTR, NULL );
		fprintf( CFP, "  gtr\n" );
		break;
	case SYM_GREATER_EQUAL :
		addinst( OP_GEQ, NULL );
		fprintf( CFP, "  geq\n" );
		break;
	case SYM_LESS :
		addinst( OP_LES, NULL );
		fprintf( CFP, "  les\n" );
		break;
	case SYM_LESS_EQUAL :
		addinst( OP_LEQ, NULL );
		fprintf( CFP, "  leq\n" );
		break;

	case SYM_MATCH :
		addinst( OP_MAT, NULL );
		fprintf( CFP, "  mat\n" );
		break;
	case SYM_DONT_MATCH :
		addinst( OP_MAT, NULL );
		addinst( OP_NOT, NULL );
		fprintf( CFP, "  mat\n" );
		fprintf( CFP, "  not\n" );
		break;
	case SYM_IN :
		addinst( OP_INS, NULL );
		fprintf( CFP, "  ins\n" );
		break;

	case SYM_PLUS :
		addinst( OP_ADD, NULL );
		fprintf( CFP, "  add\n" );
		break;
	case SYM_MINUS :
		addinst( OP_SUB, NULL );
		fprintf( CFP, "  sub\n" );
		break;
	case SYM_PERCENT :
		addinst( OP_MOD, NULL );
		fprintf( CFP, "  mod\n" );
		break;
	case SYM_STAR :
		addinst( OP_MUL, NULL );
		fprintf( CFP, "  mul\n" );
		break;
	case SYM_SLASH :
		addinst( OP_DIV, NULL );
		fprintf( CFP, "  div\n" );
		break;
	case SYM_NEGATE :
		addinst( OP_NEG, NULL );
		fprintf( CFP, "  neg\n" );
		break;

	case SYM_COLON :
		addinst( OP_BPR, NULL );
		fprintf( CFP, "  bpr\n" );
		break;

	case SYM_MINUS_MINUS :
		if( np->n_left ){
			addinst( OP_DECP, NULL );
			fprintf( CFP, "  decp\n" );
		}else{
			addinst( OP_PDEC, NULL );
			fprintf( CFP, "  pdec\n" );
		}
		break;
	case SYM_PLUS_PLUS :
		if( np->n_left ){
			addinst( OP_INCP, NULL );
			fprintf( CFP, "  incp\n" );
		}else{
			addinst( OP_PINC, NULL );
			fprintf( CFP, "  pinc\n" );
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
		fprintf( CFP, "SYM_ERROR\n" );
		break;
	default :
		fprintf( stderr, "SC_node: Unknown symbol %d\n", np->n_sym );
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

void	dumpinst( fp, i, ip )
FILE	*fp;
INST_T	*ip;
{
	VALUE_T	*vp;
	char	name[ 20 ];
	
	if( ip->i_op < 0 || ip->i_op >= N_OP ){
		fprintf( fp, "dumpinst: bad op %d\n", ip->i_op );
		exit( 1 );
	}

	fprintf( fp, "prog[%5d]:  %s", i, opnames[ ip->i_op ] );
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

	case OP_INCP :
	case OP_PINC :
	case OP_DECP :
	case OP_PDEC :
		break;

	case OP_FJP :
	case OP_JMP :
		break;
	}
	vp = &ip->i_val;
	if( vp->v_type == T_INT ){
		if( ip->i_op != OP_STRF )
			fprintf( fp, " %d", vp->v_value.v_ival );
		else{
			mk_stref_name( vp->v_value.v_ival, name );
			fprintf( fp, " %s", name );
		}
	}else if( vp->v_type == T_FLOAT ) 
		fprintf( fp, " %f", vp->v_value.v_fval );
	else if( vp->v_type == T_STRING || vp->v_type == T_IDENT )
		fprintf( fp, " %s", vp->v_value.v_pval );
	fprintf( fp, "\n" );
}
