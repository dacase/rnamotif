#ifndef	__DBASE__
#define	__DBASE__

#define	DB_STD	0
#define	DB_USER	1
typedef	struct	dbase_t	{
	int	d_type;
	int	d_first;
	int	d_last;
	int	d_current;
	int	*d_iref;
} DBASE_T;

DBASE_T	*DB_open();
DBASE_T	*DB_next();

#endif
