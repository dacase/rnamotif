#ifndef	__RNAMOT__
#define	__RNAMOT__

#define	UNDEF	(-1)
#define	UNBOUNDED	0x7fffffff

#define	T_UNDEF		0
#define	T_INT		1
#define	T_FLOAT		2
#define	T_STRING	3
#define	T_PAIR		4
#define	T_POS		5
#define	T_IDENT		6

#define	C_UNDEF		0
#define	C_LIT		1
#define	C_VAR		2
#define	C_EXPR		3

#define	S_UNDEF		0
#define	S_GLOBAL	1
#define	S_STREL		2
#define	S_SITE		3

typedef	struct	value_t	{
	int	v_type;
	union	{
		int	v_ival;
		float	v_fval;
		void	*v_pval;
	} v_value;
} VALUE_T;

typedef	struct	ident_t	{
	char	*i_name;
	int	i_type;
	int	i_class;
	int	i_scope;
	VALUE_T	i_val;
} IDENT_T;

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
	void	*ps_mat;
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
	int	p_dindex;
	ADDR_T	p_addr;
} POS_T;

typedef	struct	site_t	{
	struct	site_t	*s_next;
	POS_T	*s_pos;
	int	s_n_pos;
	PAIRSET_T	*s_pairset;
} SITE_T;

typedef	struct	strel_t	{
	int	s_checked;	/* used during linking		*/
	int	s_type;
	int	s_proper;	/* false = pseudoknot		*/
	char	s_index;	/* index into descr array	*/
	int	s_lineno;
	int	s_searchno;	/* index into searches[]	*/
	int	s_matchoff;	/* matched string starts here	*/
	int	s_matchlen;	/* matched string is this long	*/
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
	int	s_mismatch;
	int	s_mispair;
	PAIRSET_T	*s_pairset;
} STREL_T;

typedef	struct	search_t	{
	STREL_T	*s_descr;
	STREL_T	*s_forward;
	STREL_T	*s_backup;
	int	s_zero;
	int	s_dollar;
} SEARCH_T;

#endif
