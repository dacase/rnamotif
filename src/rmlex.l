%{

#include "rnamot.h"
#include "y.tab.h"
double	atof();		/* get it it if we ever need reals	*/

extern	int	rm_lineno;
extern	VALUE_T	rm_tokval;
static	char	*sp;
static	POS_T	*posp;

%}
%%

#.*		;

\n		rm_lineno++;

[\ \t\f]+	;

parms		return( SYM_PARMS );
descr		return( SYM_DESCR );
sites		return( SYM_SITES );
ss		return( SYM_SS );
h5		return( SYM_H5 );
h3		return( SYM_H3 );
p5		return( SYM_P5 );
p3		return( SYM_P3 );
t1		return( SYM_T1 );
t2		return( SYM_T2 );
t3		return( SYM_T3 );
q1		return( SYM_Q1 );
q2		return( SYM_Q2 );
q3		return( SYM_Q3 );
q4		return( SYM_Q4 );

[a-zA-Z][_a-zA-Z0-9]*	{
		  sp = ( char * )malloc(( yyleng + 1 ) *sizeof( char ));
		  if( sp == NULL ){
			errormsg( 1, 
				"lex.yy.c: can't alloc sp for SYM_IDENT." );
		  }
		  strcpy( sp, yytext );
		  rm_tokval.v_type = T_STRING;
		  rm_tokval.v_value.v_pval = sp;
		  return( SYM_IDENT );
		}
[0-9]+		{ rm_tokval.v_type = T_INT;
		  rm_tokval.v_value.v_ival = atoi( yytext );
		  return( SYM_INT );
		}
\"[^"\n]*	{ if( yytext[ yyleng - 1 ] == '\\' )
			yymore();
		  else{
			yytext[ yyleng++ ] = input();
			yytext[ yyleng ] = '\0';
			sp = ( char * )malloc( ( yyleng - 1 )*sizeof( char ) );
			if( sp == NULL ){
				errormsg( 1,
				"lex.yy.c: can't alloc sp for SYM_STRING." );
			}
			strncpy( sp, &yytext[1], yyleng - 2 );
			sp[ yyleng - 2 ] = '\0';
			rm_tokval.v_type = T_STRING;
			rm_tokval.v_value.v_pval = sp;
			return( SYM_STRING );
		  }
		}

=		return( SYM_ASSIGN );
\+=		return( SYM_PLUS_ASSIGN );
\-=		return( SYM_MINUS_ASSIGN );
\+		return( SYM_PLUS );
\-		return( SYM_MINUS );
\$		{ posp = ( POS_T * )malloc( sizeof( POS_T ) );
		  if( posp == NULL ){
			errormsg( 1,
				"lex.yy.c: can't alloc posp for SYM_DOLLAR." );
		  }
		  posp->p_type = SYM_DOLLAR;
		  posp->p_lineno = rm_lineno;
		  posp->p_tag = NULL;
		  posp->p_l2r = 0;
		  posp->p_offset = 0;
		  rm_tokval.v_type = T_POS;
		  rm_tokval.v_value.v_pval = posp;
		  return( SYM_DOLLAR );
		}
\(		return( SYM_LPAREN );
\)		return( SYM_RPAREN );
\{		return( SYM_LCURLY );
\}		return( SYM_RCURLY );
\,		return( SYM_COMMA );
:		return( SYM_COLON );

.		return( SYM_ERROR );

%%

int	yywrap()
{

	return( 1 );
}
