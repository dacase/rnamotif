#include <stdio.h>

#include "rmdefs.h"
#include "rnamot.h"
#include "mm_regexp.h"

#define	CBRA	2	/* 0002, \(      */
#define	CCHR	4	/* 0004  ord chr */
#define	CDOT	8	/* 0010  .       */
#define	CCL	12	/* 0014  [...]   */
#define	CXCL	16	/* 0020  [.8bt.] */
#define	CDOL	20	/* 0024  $       */
#define	CCEOF	22	/* 0026  <eof>   */
#define	CKET	24	/* 0030  \)      */
#define	CBRC	28	/* 0034  \<      */
#define	CLET	30	/* 0036  \>      */
#define	CBACK	36	/* 0044  *?      */
#define	NCCL	40	/* 0050  [^...]  */

#define	STAR	01	/* 0001 */
#define	RANGE	03	/* 0003 */

#define	ISTHERE(c)	(ep[(c) >> 3] & bittab[(c) & 07])
static	char	bittab[] = { 1, 2, 4, 8, 16, 32, 64, 128 };

#define	BCNT(c)		(bitcnt[((c)>>4) & 017] + bitcnt[(c) & 017])
static	int	bitcnt[] = { 0, 1, 1, 2, 1, 2, 2, 3, 
			     1, 2, 2, 3, 2, 3, 3, 4 };

extern	char	*loc1, *loc2;
extern	int	circf;

static	int	mm_advance( char *, char *, int, int * );
static	float	mm_classccnt( int, char [], int );
static	int	mm_classmcnt( char [], int );

char	*mm_regdup( char expbuf[], int len )
{
	char	*bp;

	bp = ( char * )malloc( ( len + 1 ) * sizeof( char ) );
	if( bp != NULL ){
		memcpy( bp, expbuf, ( long )len );
		bp[ len ] = CCEOF;
	}
	return( bp );
}

void	mm_seqlen( STREL_T *stp, int getbest, int *minl, int *maxl, int *mmok )
{
	char	*ep;
	int	dol, star, neg;
	int	rng, trng;
	float	b_ecnt, ecnt, ccnt;
	int	b_pos, pos, b_len, b_minl, b_maxl;
	static	STREL_T	b_descr;
	static	char	b_seq[ 2 ];
	static	char	b_expbuf[ ( 256 * RE_BPC ) ];
	int	l_minl, l_maxl, r_minl, r_maxl, b_mmok;

	*b_seq = *stp->s_seq;
	b_seq[ 1 ] = '\0';
	b_descr.s_seq = b_seq;
	b_descr.s_expbuf = b_expbuf;
	*minl = 0;
	*maxl = UNDEF;	/* generally no max length 	*/
	*mmok = 1;	/* allow mismatch unless ...	*/
	dol = 0;
	star = 0;
	b_ecnt = ecnt = 0;	/* # of eff. chars of longest literal	*/
	b_pos = pos = 0;
	b_len = 0;
	for( trng = UNDEF, ep = stp->s_expbuf; *ep != CCEOF; ep++ ){
		neg = 0;
		rng = UNDEF;
		switch( *ep ){
		case CBRA :
			ep++;
			break;
		case CKET :
			break;

		case CBRC :
			*mmok = 0;
			break;

		case CLET :
			*mmok = 0;
			break;

		case CCHR :
		case CCHR | STAR :
		case CCHR | RANGE :
			if( *ep == ( CCHR | STAR ) ){
				star = 1;
				*mmok = 0;
				if( ecnt > b_ecnt ){
					b_ecnt = ecnt;
					b_len = ep - stp->s_expbuf - pos;
					b_pos = pos;
				}
				ecnt = 0.0;
				pos = ep - stp->s_expbuf + 2;
			}else if( *ep == ( CCHR | RANGE ) ){
				*minl += ep[1];
				rng = ep[2]-ep[1];
				if( ep[1] != ep[2] )
					*mmok = 0;
				ecnt += rng;
				ep += 2;
			}else{
				( *minl )++;
				ecnt += 1.0;
			}
			ep++;
			break;
		case CDOT :
		case CDOT | STAR :
		case CDOT | RANGE :
			if( ecnt > b_ecnt ){
				b_ecnt = ecnt;
				b_len = ep - stp->s_expbuf - pos;
				b_pos = pos;
			}
			ecnt = 0;
			if( *ep == ( CDOT | STAR ) ){
				star = 1;
				*mmok = 0;
				pos = ep - stp->s_expbuf + 1;
			}else if( *ep == ( CDOT | RANGE ) ){
				*minl += ep[1];
				rng = ep[2] - ep[1];
				if( ep[1] != ep[2] )
					*mmok = 0;
				pos = ep - stp->s_expbuf + 3;
				ep += 2;
			}else{
				( *minl )++;
				pos = ep - stp->s_expbuf + 1;
			}
			break;
		case NCCL :
		case NCCL | STAR :
		case NCCL | RANGE :
			neg = 1;
		case CCL :
		case CCL | STAR :
		case CCL | RANGE :
			ecnt += ccnt = mm_classccnt( neg, &ep[1], 16 );
			if( *ep == (CCL|STAR) || *ep == (NCCL|STAR)){
				if( ecnt > b_ecnt ){
					b_ecnt = ecnt;
					b_len = ep - stp->s_expbuf - pos;
					b_pos = pos;
				}
				ecnt = 0;
				pos = ep - stp->s_expbuf + 17;
				star = 1;
				*mmok = 0;
			}else if( *ep == (CCL|RANGE) || *ep == (NCCL|RANGE) ){
				*minl += ep[16+1];
				rng = ep[16+2] - ep[16+1];
				if( ep[16+1] != ep[16+2] )
					*mmok = 0;
				ecnt += ( ep[16+1] - 1 ) * ccnt;
				ep += 2;	
			}else{
				( *minl )++;
			}
			ep += 16;
			break;
		case CDOL :
			if( ecnt > b_ecnt ){
				b_ecnt = ecnt;
				b_len = ep - stp->s_expbuf - pos;
				b_pos = pos;
			}
			ecnt = 0;
			dol = 1;
			break;
		default :
			fprintf( stderr, "mm_seqlen: %2d?\n", *ep );
			break;
		}
		if( rng != UNDEF ){
			if( trng == UNDEF )
				trng = rng;
			else
				trng += rng;
		}
	}
	if( *stp->s_seq == '^' && dol && !star )
		*maxl = trng == UNDEF ? *minl : *minl + trng;
	if( ecnt > b_ecnt ){
		b_ecnt = ecnt;
		b_len = ep - stp->s_expbuf - pos;
		b_pos = pos;
	}
	if( getbest ){
		stp->s_bestpat.b_ecnt = b_ecnt;
		stp->s_bestpat.b_pos = b_pos;
		stp->s_bestpat.b_len = b_len;

		memcpy( b_expbuf, stp->s_expbuf, ( long )b_pos );
		b_expbuf[ b_pos ] = CDOL;
		b_expbuf[ b_pos + 1 ] = CCEOF;
		b_descr.s_e_expbuf = &b_expbuf[ b_pos + 1 ];
		*b_seq = *stp->s_seq;
		b_seq[ 1 ] = '\0';
		mm_seqlen( &b_descr, 0, &l_minl, &l_maxl, &b_mmok );
		stp->s_bestpat.b_lminlen = l_minl;
		stp->s_bestpat.b_lmaxlen = l_maxl == UNDEF ? UNBOUNDED : l_maxl;

		memcpy( b_expbuf, &stp->s_expbuf[ b_pos ], ( long )b_len );
		b_expbuf[ b_len ] = CDOL;
		b_expbuf[ b_len + 1 ] = CCEOF;
		*b_seq = '^';
		b_seq[ 1 ] = '\0';
		mm_seqlen( &b_descr, 0, &b_minl, &b_maxl, &b_mmok );
		stp->s_bestpat.b_minlen = b_minl;
		stp->s_bestpat.b_maxlen = b_maxl;

		memcpy( b_expbuf, &stp->s_expbuf[ b_pos + b_len ],
			( long )( ep - &stp->s_expbuf[ b_pos + b_len ] ) );
		b_expbuf[ ep - &stp->s_expbuf[ b_pos + b_len ] ] = CCEOF;
		*b_seq = '^';
		b_seq[ 1 ] = '\0';
		mm_seqlen( &b_descr, 0, &r_minl, &r_maxl, &b_mmok );
		stp->s_bestpat.b_rminlen = r_minl;
		stp->s_bestpat.b_rmaxlen = r_maxl == UNDEF ? UNBOUNDED : r_maxl;
	}
}

static	float	mm_classccnt( int neg, char class[], int s_class )
{
	int	mcnt;
	float	rval;

	mcnt = mm_classmcnt( class, s_class );
	if( neg ){
		if( mcnt >= 3 )
			rval = 1.0;
		else if( mcnt == 2 )
			rval = 0.5;
		else if( mcnt == 1 )
			rval = 0.25;
		else
			rval = 0.0;
	}else if( mcnt >= 4 )
		rval = 0.0;
	else if( mcnt == 3 )
		rval = 0.25;
	else if( mcnt == 2 )
		rval = 0.5;
	else
		rval = 1.0;
	return( rval );
}

static	int	mm_classmcnt( char class[], int s_class )
{
	int	i, c, cnt;
	char	*cc;

	for( cnt = 0, cc = class, i = 0; i < s_class; i++, cc++ ){
		c = BCNT( *cc & 0377 );
		cnt += c;
	}
	return( cnt );
}

void	mm_dumppat( FILE * fp, char expbuf[], char *e_expbuf )
{
	char	*ep;
	int	i;

	for( ep = expbuf; ep < e_expbuf && *ep != CCEOF; ep++ ){
		switch( *ep ){
		case CBRA :
			ep++;
			printf( " \\(%d", *ep );
			break;
		case CKET :
			printf( " \\)" );
			break;

		case CBRC :
			printf( " \\<" );
			break;

		case CLET :
			printf( " \\>" );
			break;

		case CCHR :
		case CCHR | STAR :
		case CCHR | RANGE :
			printf( " %c", ep[1] );
			if( *ep == ( CCHR | STAR ) )
				printf( "*" );
			else if( *ep == ( CCHR | RANGE ) ){
				printf( "\\{%d,%d\\}", ep[2], ep[3] );
				ep += 2;
			}
			ep++;
			break;
		case CDOT :
		case CDOT | STAR :
		case CDOT | RANGE :
			printf( " ." );
			if( *ep == ( CDOT | STAR ) )
				printf( "*" );
			else if( *ep == ( CDOT | RANGE ) ){
				printf( "\\{%d,%d\\}", ep[1], ep[2] );
				ep += 2;
			}
			break;
		case CCL :
		case CCL | STAR :
		case CCL | RANGE :
		case NCCL :
		case NCCL | STAR :
		case NCCL | RANGE :
			if( ( *ep & ~RANGE ) == CCL )
				printf( " [" );
			else
				printf( " [^" );
			for( i = 1; i <= 16; i++ ){
				printf( " %02x", ep[i] & 0xff );
			}
			printf( " ]" );
			if( *ep == (CCL|STAR) || *ep == (NCCL|STAR))
				printf( "*" );
			else if( *ep == (CCL|RANGE) || *ep == (NCCL|RANGE) ){
				printf( "\\{%d,%d\\}", ep[16+1], ep[16+2] );
				ep += 2;	
			}
			ep += 16;
			break;
		case CDOL :
			printf( " $" );
			break;
		default :
			printf( " %2d", *ep );
			break;
		}
	}
	printf( " %s\n", "<eof>" );
}

int	mm_step( char *p1, char *p2, int l_mm, int *n_mm )
{

	loc1 = p1;
	if( circf )
		return( mm_advance( p1, p2, l_mm, n_mm ) );

	do{
		if( mm_advance( p1, p2, l_mm, n_mm ) ){
			loc1 = p1;
			return( 1 );
		}
	}while( *p1++ );
	return( 0 );
}

static	int	mm_advance( char *lp, char *ep, int l_mm, int *n_mm )
{
	int	neg, low;
	int	c;

	for( *n_mm  = 0; ; ){
		neg = 0;
		switch( *ep++ ){

		case CCHR :
			if( *ep++ == *lp++ )
				continue;
			else{
				if( lp[-1] == '\0' )
					return( 0 );
				( *n_mm )++;
				if( *n_mm > l_mm ){
					return( 0 );
				}
			}
			break;

		case CCHR | RANGE :
			c = *ep++;
			low = *ep;
			while( low-- ){
				if(*lp++ != c){
					if( lp[ -1 ] == '\0' )
						return( 0 );
					( *n_mm )++;
					if( *n_mm > l_mm )
						return( 0 ); 
				}
			}
			ep += 2;
			break;

		case CDOT :
			if( *lp++ )
				continue;
			return( 0 );
			break;

		case CDOT | RANGE :
			low = *ep;
			while( low-- ){
				if( *lp++ == '\0' )
					return( 0 );
			}
			ep += 2;
			break;

		case CDOL :
			if( *lp == '\0' )
				continue;
			return( 0 );
			break;

		case CCEOF :
			loc2 = lp;
			return( 1 );
			break;

		case NCCL :
			neg = 1;
		case CCL :
			if( ( c = *lp++ ) == '\0' )
				return( 0 );
			if( ( ( c & 0200 ) == 0 && ISTHERE( c ) ) ^ neg ){
				ep += 16;
				continue;
			}else{
				( *n_mm )++;
				if( *n_mm > l_mm )
					return( 0 );
				ep += 16;
			}
			break;

		case NCCL | RANGE :
			neg = 1;
		case CCL | RANGE :
			low = ep[16];
			while( low-- ){
				if( ( c = *lp++ ) == '\0' )
					return( 0 );
				if((( c & 0200 ) || !ISTHERE(c)) ^ neg ){
					( *n_mm )++;
					if( *n_mm > l_mm )
						return( 0 );
				}
			}
			ep += 18;
			break;

		default :
			break;
		}
	}
	return( 0 );
}
