#ifndef	__DBUTIL__
#define	__DBUTIL__

FILE	*DB_fnext( FILE *, int *, int, char *[] );
int	FN_fgetseq( FILE *, char *, int, char *, int, char * );
int	PIR_fgetseq( FILE *, char *, int, char *, int, char * );
int	GB_fgetseq( FILE *, char *, int, char *, int, char * );

#endif
