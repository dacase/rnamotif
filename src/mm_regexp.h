#ifndef __MM_REGEXP__
#define	__MM_REGEXP__

char	*mm_regdup( char *, int );
void	mm_seqlen( STREL_T *, int, int *, int *, int * );
void	mm_dumppat( FILE *, char [], char * );
int	mm_step( char *, char *, int, int * );

#endif
