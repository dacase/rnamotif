%token	SYM_PAIRINGS
%token	SYM_PARAMETERS
%token	SYM_DESCRIPTOR
%token	SYM_MISMATCH
%token	SYM_MISPAIR
%token	SYM_PAIR
%token	SYM_POSITIONS

%token	SYM_SINGLE
%token	SYM_HELIX5
%token	SYM_HELIX3
%token	SYM_PARALLEL5
%token	SYM_PARALLEL3
%token	SYM_TRIPLEX1
%token	SYM_TRIPLEX2
%token	SYM_TRIPLEX3
%token	SYM_QUAD1
%token	SYM_QUAD2
%token	SYM_QUAD3
%token	SYM_QUAD4

%token	SYM_IDENT
%token	SYM_INT
%token	SYM_STRING

%token	SYM_EQUAL
%token	SYM_LPAREN
%token	SYM_RPAREN
%token	SYM_LCURLY
%token	SYM_RCURLY
%token	SYM_COMMA
%token	SYM_MINUS
%token	SYM_DOLLAR
%token	SYM_PERIOD
%token	SYM_COLON
%token	SYM_ERROR

%%
program		: pair_part parm_part desc_part pos_part ;

pair_part	: SYM_PAIRINGS pairdef_list
		| ;
pairdef_list	: pairdef
		| pairdef pairdef_list ;
pairdef		: SYM_IDENT SYM_EQUAL pairspec ;
pairspec	: SYM_LCURLY pair_list SYM_RCURLY ;
pair_list	: pair
		| pair SYM_COMMA pair_list ;
pair		: SYM_STRING ;

parm_part	: SYM_PARAMETERS keyval_list 
		| ;
keyval_list	: keyval 
		| keyval keyval_list ;
keyval		: SYM_IDENT SYM_EQUAL val ;
val		: SYM_IDENT
		| SYM_INT
		| SYM_STRING ;

desc_part	: SYM_DESCRIPTOR strel_list ;
strel_list	: strel
		| strel strel_list ;
strel		: strtype strtag SYM_LPAREN strparm_list SYM_RPAREN ;
strtype		: SYM_SINGLE
		| SYM_HELIX5
		| SYM_HELIX3
		| SYM_PARALLEL5
		| SYM_PARALLEL3
		| SYM_TRIPLEX1
		| SYM_TRIPLEX2
		| SYM_TRIPLEX3
		| SYM_QUAD1
		| SYM_QUAD2
		| SYM_QUAD3
		| SYM_QUAD4 ;
strtag		: SYM_PERIOD SYM_IDENT
		| SYM_PERIOD SYM_INT
		| ;
strparm_list	: strparm
		| strparm SYM_COMMA strparm_list ;
strparm		: SYM_INT
		| SYM_STRING
		| strkvparm ;
strkvparm	: SYM_MISPAIR SYM_EQUAL SYM_INT
		| SYM_MISMATCH SYM_EQUAL SYM_INT
		| SYM_PAIR SYM_EQUAL pairspec ;

pos_part	: SYM_POSITIONS posspec_list
		| ;
posspec_list	: posspec
		| posspec posspec_list ;
posspec		: posaddr_list SYM_EQUAL posval_list ;
posaddr_list	: strel
		| strel SYM_COLON posaddr_list ;
posval_list	: pairspec ;
