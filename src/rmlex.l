%{

#include "rnamot.h"
#include "y.tab.h"
double	atof();		/* get it it if we ever need reals	*/

VALUE_T	rmval;
static	char	*sp;

%}
%%

#.*		;

[\ \t\f\n]+	;

parm		return( SYM_PARM );
descr		return( SYM_DESCR );
site		return( SYM_SITE );
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

[a-zA-Z][_a-zA-Z0-9]*	\
		{ sp = ( char * )malloc(( yyleng + 1 ) *sizeof( char ));
		  if( sp == NULL ){
			fprintf( stderr,
		"lex.yy.c: FATAL: can't alloc sp for SYM_IDENT.\n" );
			exit( 1 );
		  }
		  strcpy( sp, yytext );
		  rmval.v_type = T_STRING;
		  rmval.v_value.v_cval = sp;
		  return( SYM_IDENT );
		}
[0-9]+		{ rmval.v_type = T_INT;
		  rmval.v_value.v_ival = atoi( yytext );
		  return( SYM_INT );
		}
\"[^"\n]*	{ if( yytext[ yyleng - 1 ] == '\\' )
			yymore();
		  else{
			yytext[ yyleng++ ] = input();
			yytext[ yyleng ] = '\0';
			sp = ( char * )malloc( ( yyleng - 1 )*sizeof( char ) );
			if( sp == NULL ){
				fprintf( stderr,
			"lex.yy.c: FATAL: can't alloc sp for SYM_STRING.\n" );
				exit( 1 );
			}
			strncpy( sp, &yytext[1], yyleng - 2 );
			sp[ yyleng - 2 ] = '\0';
			rmval.v_type = T_STRING;
			rmval.v_value.v_cval = sp;
			return( SYM_STRING );
		  }
		}

=		return( SYM_EQUAL );
\+=		return( SYM_PLUS_EQUAL );
\-=		return( SYM_MINUS_EQUAL );
\+		return( SYM_PLUS );
\-		return( SYM_MINUS );
\$		return( SYM_DOLLAR );
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
