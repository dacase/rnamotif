#ifndef	__RNAMOT__
#define	__RNAMOT__

#define	UNDEF	(-1)
#define	LASTVAL	(-2)

#define	T_UNDEF		0
#define	T_INT		1
#define	T_FLOAT		2
#define	T_STRING	3
#define	T_PAIR		4
#define	T_IDENT		5

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

typedef	struct	pair_t	{
	int	p_n_bases;
	char	p_bases[ 4 ];
} PAIR_T;

typedef	struct	pairset_t	{
	int	ps_n_pairs;
	PAIR_T	*ps_pairs;
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

typedef	struct	strel_t	{
	int	s_type;
	char	s_index;	/* index into descr array */
	int	s_lineno;
	char	*s_tag;
	struct	strel_t	*s_next;
	struct	strel_t	*s_mates;
	int	s_minlen;
	int	s_maxlen;
	char	*s_seq;
	int	s_mismatch;
	int	s_mispair;
	PAIRSET_T	*s_pairset;
	char	*s_sites;
} STREL_T;

#endif
