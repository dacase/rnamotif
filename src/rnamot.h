#ifndef	__RNAMOT__
#define	__RNAMOT__

#define	UNDEF	(-1)
#define	LASTVAL	(-2)

#define	T_UNDEF		0
#define	T_INT		1
#define	T_FLOAT		2
#define	T_STRING	3
#define	T_PAIR		4

#define	C_UNDEF		0
#define	C_LIT		1
#define	C_VAR		2
#define	C_EXPR		3

typedef	struct	value_t	{
	int	v_type;
	union	{
		int	v_ival;
		float	v_fval;
		char	*v_cval;
		char	*v_pval;
	} v_value;
} VALUE_T;

typedef	struct	ident_t	{
	char	*i_name;
	int	i_type;
	int	i_class;
	VALUE_T	i_value;
} IDENT_T;

typedef	struct	node_t	{
	int	n_sym;
	int	n_type;
	int	n_class;
	VALUE_T	n_val;
	struct	node_t	*n_left;
	struct	node_t	*n_right;
} NODE_T;

typedef	struct	strel_t	{
	int	s_type;
	char	s_index;	/* index into descr array */
	char	*s_tag;
	struct	strel_t	*s_next;
	struct	strel_t	*s_pairs;
	int	s_minlen;
	int	s_maxlen;
	char	*s_seq;
	int	s_mismatch;
	int	s_mispair;
	char	*s_pairdata;
	char	*s_sites;
} STREL_T;

#endif
