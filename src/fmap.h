#ifndef	__FMAP__
#define	__FMAP__

typedef	struct	fm_entry_t	{
	char	*f_dname;
	int	f_part;
	char	*f_fname;
	char	*f_hosts;
} FM_ENTRY_T;

typedef	struct	fmap_t	{
	char	*f_root;
	char	*f_format;
	int	f_nentries;
	FM_ENTRY_T	*f_entries;
} FMAP_T;

FMAP_T	*FMread_fmap( char * );
FM_ENTRY_T	*FMget_fmentry( FMAP_T *, char *, int );
int     FMmark_active( FMAP_T *, char *, int [] );
void	FMwrite_fmap( FILE *, FMAP_T * );
void	*FMfree_fmap( FMAP_T * );

#endif
