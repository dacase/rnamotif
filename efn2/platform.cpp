//SGI platform code for RNAstructure
# define bool int
# define true 1
# define false 0
# define TRUE 1
# define FALSE 0
//# define try /* not supported on SGI */
//# define catch /* not supported on SGI */
//# define (xalloc) /* not defined on SGI */
//#define floor ffloor
//#define floor /*floor*/
#define sgifix strcat(line," ");
#define binary in

#include <math.h>

int min(int i, int j)
{

	if (i>j) return j;
	return i;
}

int max (int i,int j)
{

	if (i<j) return j;
	return i;
}

int pow10(int i)
{
	int j, n;

	if (i==0) return 1;
	j = 1;

	for (n=1;n<=i;n++) {
		j = 10*j;
	}

	return j;
}

void itoa (int x, char *ch, int i) 
{
	float y;

	y = float (x);
	gcvt(y,6,ch);
}
