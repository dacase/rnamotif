#! /bin/sh
tr '#' ' ' < $1 |\
awk '$2 ~ /^SYM_/{	printf( "\tcase %s :\n", $2 )
	printf( "\t\tfprintf( fp, \"%s", $2 ) 
	if( $2 == "SYM_IDENT" ){ 
		printf( " = \\\"%%s\\\"\\n\"" ) 
		printf( ", np->n_val.v_value.v_pval" ) 
	}else if( $2 == "SYM_INT" ){ 
		printf( " = %%d\\n\"" ) 
		printf( ", np->n_val.v_value.v_ival" ) 
	}else if( $2 == "SYM_FLOAT" ){ 
		printf( " = %%lg\\n\"" ) 
		printf( ", np->n_val.v_value.v_dval" ) 
	}else if( $2 == "SYM_STRING" ){
		printf( " = \\\"%%s\\\"\\n\"" ) 
		printf( ", np->n_val.v_value.v_pval" ) 
	}else if( $2 == "SYM_PAIRSET" ){
		printf( " = " ) 
		printf( "RM_dump_pairset( fp, np->n_val.v_value.v_pval );" ) 
		printf( "\\n\"" ) 
	}else 
		printf( "\\n\"" ) 
	printf( " );\n" ) 
       	printf( "\t\tbreak;\n" )  } 
 END {  printf( "\tdefault :\n" ) 
	printf( "\t\tfprintf( fp, \"RM_dumpnode: Unknown symbol %%d\\n\"" ) 
	printf( ", np->n_sym );\n" ) 
	printf( "\t\tbreak;\n" ) } '
