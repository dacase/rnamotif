%%

#.*		;

[\ \t\f\n]+	;

ss		return( SYM_SINGLE );
h5		return( SYM_HELIX5 );
h3		return( SYM_HELIX3 );
p5		return( SYM_PARALLEL5 );
p3		return( SYM_PARALLEL3 );
t1		reteun( SYM_TRIPLEX1 );
t2		reteun( SYM_TRIPLEX2 );
t3		reteun( SYM_TRIPLEX3 );
q1		reteun( SYM_QUAD1 );
q2		reteun( SYM_QUAD2 );
q3		reteun( SYM_QUAD3 );
q4		reteun( SYM_QUAD4 );

[a-zA-Z][_a-zA-Z0-9]*	return( SYM_IDENT );
[0-9]+		return( SYM_INT );
\"[^"\n]*	{ if( yytext[ yyleng - 1 ] == '\\' )
			yymore();
		  else{
			yytext[ yyleng ] = input();
			yytext[ yyleng ] = '\0';
			strcpy( yytext, &yytext[ 1 ] );
			return( SYM_STRING );
		  }
		}

\(		return( SYM_LPAREN );
\)		return( SYM_RPAREN );
\,		return( SYM_COMMA );
\-		return( SYM_MINUS );
\$		return( SYM_DOLLAR );
:		return( SYM_COLON );

.		return( SYM_ERROR );

%%

int	yywrap()
{

	return( 1 );
}
