#ifndef	__RNAMOT__
#define	__RNAMOT__

typedef	struct	value_t	{
	int	v_sym;
	union	{
		int	v_ival;
		float	v_fval;
		char	*v_cval;
		char	*v_pval;
	} v_value;
} VALUE_T;

typedef	struct	strel_t	{
	int	s_type;
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
