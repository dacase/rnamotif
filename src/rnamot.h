#ifndef	__RNAMOT__
#define	__RNAMOT__

#define	VERSION	"v3.1.1 2015-may-13"

#define	U_MSG_S	\
"usage: %s [ options ] descr [ fmt ] [ data ]\n\n\
options:\n\
\t-c\t\t\tCompile only, no search\n\
\t-d\t\t\tDump internal data structures\n\
\t-h\t\t\tDump the structure hierarchy\n\
\t-N size\t\t\tSize of longest input. (default=30000000)\n\
\t-On\t\t\tMin #chars, best seq= for opt. (default=2.5)\n\
\t-p\t\t\tDump the score code\n\
\t-s\t\t\tShow builtin variables\n\
\t-v\t\t\tPrint Version Infomation\n\
\t-context\t\tPrint solution context\n\
\t-sh\t\t\tStrict helices: bases surrounding a helix\n\
\t\t\t\tmust not be able to extend that helix\n\
\t-Dvar=expr\t\tSet the value of var to expr\n\
\t-Idir\t\t\tAdd include source directory, dir\n\
\t-xdfname file-name\tPreprocessor output file\n\
\t-pre cmd\t\tmrnamotif only: run cmd db | rnamotif\n\
\t-post cmd\t\tmrnamotif only: run rnamotif | cmd\n\
\t-help\t\t\tPrint this message\n\
\n\
descr:\tUse one:\n\
\t-descr descr-file\tMay have includes; use cmd-line defs\n\
\t-xdescr xdescr-file\tMay not have includes; ignore cmd-line defs\n\
\n\
fmt:\t(Optional) Use one:\n\
\t-fmt fastn\t\tfastn (default)\n\
\t-fmt pir\t\tpir\n\
\t-fmt gb\t\t\tGB flatfile\n\
\n\
data:\t(Optional) Use one:\n\
\tfile1 ...\t\tSerial version; no files search stdin (default)\n\
\t-fmap file-map db1 ...\tmrnamotif only; no dbs search whole map\n"

#define	UNBOUNDED	0x7fffffff
#define	EFN_INFINITY	16000	/* ? */
#define	MAXSLEN		30000000

#define	SID_SIZE	100	/* seq id		*/
#define	SDEF_SIZE	20000	/* seq def		*/
#define	SCORE_SIZE	1000	/* bytes for score	*/
#define	RE_BPC		20	/* bytes for RE element	*/

#define	T_UNDEF		0
#define	T_INT		1
#define	T_FLOAT		2
#define	T_STRING	3
#define	T_PAIRSET	4
#define	T_POS		5
#define	T_IDENT		6
#define	T_HIT		7	/* save match, HOLD, RELEASE stmts	*/
#define	N_TYPE		8
#define	T_IJ(i,j)	((i)*N_TYPE+(j))

#define	C_UNDEF		0
#define	C_LIT		1
#define	C_VAR		2
#define	C_EXPR		3

#define	S_UNDEF		0
#define	S_GLOBAL	1
#define	S_STREL		2
#define	S_SITE		3

	/* context for the parser/lexer	*/
#define	CTX_START	0
#define	CTX_PARMS	1
#define	CTX_DESCR	2
#define	CTX_SITES	3
#define	CTX_SCORE	4

	/* score "programs", BEGIN, main, END	*/
#define	P_BEGIN		0
#define	P_MAIN		1
#define	P_END		2
#define	N_PROG		3

	/* action to take returned by RM_score()	*/
#define	SA_REJECT	0
#define	SA_HOLD		1
#define	SA_ACCEPT	2

typedef	struct	value_t	{
	int	v_type;
	union	{
		int	v_ival;
		double	v_dval;
		void	*v_pval;
	} v_value;
} VALUE_T;

typedef	struct	ident_t	{
	struct	ident_t	*i_left;
	struct	ident_t	*i_right;
	char	*i_name;
	int	i_type;
	int	i_class;
	int	i_scope;
	int	i_reinit;
	VALUE_T	i_val;
} IDENT_T;

typedef	struct	hit_t	{
	char	*h_def;
	char	*h_match;
} HIT_T;

#define	BCODE_A		0
#define	BCODE_C		1
#define	BCODE_G		2
#define	BCODE_T		3
#define	BCODE_N		4
#define N_BCODES	5

typedef	int	BP_MAT_T[ N_BCODES ][ N_BCODES ];
typedef	int	BT_MAT_T[ N_BCODES ][ N_BCODES ][ N_BCODES ];
typedef	int	BQ_MAT_T[ N_BCODES ][ N_BCODES ][ N_BCODES ][ N_BCODES ];

typedef	struct	pair_t	{
	int	p_n_bases;
	char	p_bases[ 4 ];
} PAIR_T;

typedef	struct	pairset_t	{
	int	ps_n_pairs;
	PAIR_T	*ps_pairs;
	void	*ps_mat[ 2 ];	/* 1 for duplex, 2 for 3-plex, 4-plex */
} PAIRSET_T;

typedef	struct	pairlist_t	{
	struct	pairlist_t	*pl_next;
	PAIRSET_T	*pl_pset;
} PAIRLIST_T;

typedef	struct	node_t	{
	int	n_sym;
	int	n_type;
	int	n_class;
	int	n_lineno;
	char	*n_filename;
	VALUE_T	n_val;
	struct	node_t	*n_left;
	struct	node_t	*n_right;
} NODE_T;

typedef	struct	addr_t	{
	int	a_l2r;
	int	a_offset;
} ADDR_T;

typedef	struct	pos_t	{
	int	p_type;
	int	p_lineno;
	char	*p_tag;
	void	*p_descr;
	ADDR_T	p_addr;
} POS_T;

typedef	struct	site_t	{
	struct	site_t	*s_next;
	POS_T	*s_pos;
	int	s_n_pos;
	PAIRSET_T	*s_pairset;
} SITE_T;

/*#define	SA_PROPER	0001	*/
/*#define	SA_5PAIRED	0002	*/
/*#define	SA_3PAIRED	0004	*/
/*#define	SA_DEF_PAIRED	0010	*/
/*#define	SA_5STRICT	0020	*/
/*#define	SA_3STRICT	0040	*/
/*#define	SA_DEF_STRICT	0100	*/
/*#define	SA_N_ATTR	7	*/

#define	SA_PROPER	0
#define	SA_ENDS		1
#define	SA_STRICT	2
#define	SA_N_ATTR	3

#define	SA_5PAIRED	01
#define	SA_3PAIRED	02

#define	SA_5STRICT	01
#define	SA_3STRICT	02

typedef	struct	subpat_t	{
	float	b_ecnt;
	int	b_pos;
	int	b_len;
	int	b_minlen;
	int	b_maxlen;
	int	b_lminlen;
	int	b_lmaxlen;
	int	b_rminlen;
	int	b_rmaxlen;
} BESTPAT_T;

typedef	struct	strel_t	{
	int	s_checked;	/* used during linking		*/
	int	s_type;
	signed char	s_attr[ SA_N_ATTR ];	
	char	s_index;	/* index into descr array	*/
	int	s_lineno;
	int	s_searchno;	/* index into searches[]	*/
	int	s_matchoff;	/* matched string starts here	*/
	int	s_matchlen;	/* matched string is this long	*/
	int	s_n_mismatches;	/* number of mismatches, cur. match	*/
	int	s_n_mispairs;	/* number of mispairs, cur. match	*/
	char	*s_tag;
	struct	strel_t	*s_next;
	struct	strel_t	*s_prev;
	struct	strel_t	*s_inner;
	struct	strel_t	*s_outer;
	struct	strel_t	**s_mates;
	int	s_n_mates;
	struct	strel_t	**s_scopes;
	int	s_n_scopes;
	int	s_scope;
	int	s_minlen;
	int	s_maxlen;
	int	s_minglen;
	int	s_maxglen;
	int	s_minilen;
	int	s_maxilen;
	ADDR_T	s_start;
	ADDR_T	s_stop;
	char	*s_seq;
	char	*s_expbuf;
	char	*s_e_expbuf;
	BESTPAT_T	s_bestpat;
	int	s_mismatch;
	double	s_matchfrac;
	int	s_mispair;
	double	s_pairfrac;
	PAIRSET_T	*s_pairset;
} STREL_T;

typedef	struct	search_t	{
	STREL_T	*s_descr;
	STREL_T	*s_forward;
	STREL_T	*s_backup;
	int	s_zero;
	int	s_dollar;
} SEARCH_T;

typedef	struct	incdir_t	{
	struct	incdir_t	*i_next;
	char	*i_name;
} INCDIR_T;

typedef	struct	args_t	{
	int	a_copt;
	int	a_dopt;
	int	a_hopt;
	int	a_maxslen;
	float	a_o_emin;
	int	a_popt;
	int	a_sopt;
	int	a_vopt;
	int	a_show_context;
	int	a_strict_helices;
	INCDIR_T	*a_idlist;
	char	*a_dfname;
	char	*a_xdfname;
	char	*a_cldefs;
	char	*a_precmd;
	char	*a_postcmd;
	char	*a_dbfmt;
	char	*a_fmfname;
	char	**a_dbfname;
	int	a_n_dbfname;
	int	a_c_dbfname;
} ARGS_T;

int	RM_init( int, char *[] );
void	PARM_add( NODE_T * );
void	PR_open( void );
void	PR_add( NODE_T * );
NODE_T	*PR_close( void );
void	SE_open( int );
void	SE_addval( NODE_T * );
void	SE_close( void );
int	SE_link( int, STREL_T [] );
IDENT_T	*RM_enter_id( char [], int, int, int, int, VALUE_T * );
IDENT_T	*RM_find_id( char [] );
char	*RM_str2seq( char [] );
void	POS_open( int );
void	POS_addval( NODE_T * );
void	POS_close( void );
void	SI_close( NODE_T * );

void	RM_dump_gids( FILE *, IDENT_T *, int );
void	RM_dump_id( FILE *, IDENT_T *, int );
void	RM_dump_pairset( FILE *, PAIRSET_T * );
void	RM_dump_pair( FILE *, PAIR_T * );
void	RM_dump_pairmat( FILE *, PAIRSET_T * );
void	RM_dump_descr( FILE *, STREL_T * );
void	RM_dump_pos( FILE *, int, POS_T * );
void	RM_dump_sites( FILE * );
void	RM_strel_name( STREL_T *, char [] );

int	RM_getefn2data( void );
int	RM_efn2( void );

int	RM_allocefnds( int );
int	RM_getefndata( void );
void	RM_dumpefndata( FILE * );
int	RM_knotted( void );
int	RM_efn( int, int, int );
void	RM_initst( void );

void	RM_errormsg( int, char [] );

int	RM_paired( PAIRSET_T *, int, int );
int	RM_triple( PAIRSET_T *, int, int, int );
int	RM_quad( PAIRSET_T *, int, int, int, int );
int	RM_fm_init( void );
int	RM_find_motif( int, SEARCH_T *[], SITE_T *,
		char [], char [], int, int, char [] );

ARGS_T	*RM_getargs( int, char *[], int );

NODE_T	*RM_node( int, VALUE_T *, NODE_T *, NODE_T * );
void	RM_dumpexpr( FILE *, NODE_T *, int );
void	RM_dumpnode( FILE *, NODE_T *, int );

char	*RM_preprocessor( void );

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
void	RM_break( NODE_T * );
void	RM_continue( NODE_T * );
void	RM_accept( void );
void	RM_reject( void );
void	RM_mark( void );
void	RM_clear( void );
void	RM_expr( int, NODE_T * );
void	RM_linkscore( void );
void	RM_dumpscore( FILE * );
void	RM_setprog( int );
int	RM_score( int, int, char [], IDENT_T ** );

#endif
