#if !defined(ALGORITHM_H)
#define ALGORITHM_H

#include "structure.h"
#include "defines.h"
#include "interface.h"


//	Structures:

struct stackstruct
{
	int stk[51][4],sp;
};

struct arraystruct
{
	char array[7][amax][2];
};

void de_allocate (int **v, int i);
void de_allocate (bool **v, int i);

//	dotarray: dot plot information

class dotarray
{
	int **array;
	int store;

	public:

	dotarray(int size);
	int &dot(int i, int j)
	{

      		return array[j][i];
	}
   	~dotarray();
};

//	arrayclass; 2-d arrays of w and v, used by the dynamic algorithm
class arrayclass {
	int Size;

	public:
   	//int **dg;
	int k;
	int **dg;
	int infinite;
	//ofstream out;

	//the constructor allocates the space needed by the arrays
   	arrayclass(int size)
	{

		// out.open("temp.out");
		// k = infinity;
		infinite = infinity;

		Size = size;
		register int i,j;
		dg = new int *[size+1];

   		for (i=0;i<=(size);i++) {
			dg[i] = new int [size+1];
   		}
		for (i=0;i<=size;i++) {
         		for (j=0;j<size+1;j++) {
            			dg[i][j] = infinity;
			}
		}
	}

	//the destructor deallocates the space used
	~arrayclass()
	{

		// out.close();
		int i;
		// de_allocate (dg,Size+2);
		for (i=0;i<=Size;i++) {
			delete[] dg[i];
		}
		delete[] dg;
	}

	// f is an integer function that references the correct element of
	// the array
   	int &f(int i, int j)
	{

		// out << i<<"\t"<<j<<"\n";
		if (i>j) {
			return infinite;
		} else if (i>Size)
			return f(i-Size,j-Size);	// dg[i-Size][j-Size];
		else
			return dg[i][j-i];		// dg[i][j];
	}
};

//	output used by the dynamic algorithm to produce a save file and
//	used by the opensav() to recall the data in a save file
union output
{
	int i;
	unsigned char ch[4];
	float f;
};

//	prototypes:

//	gets thermodynamic data from data files
int opendat (char *loop2,char *stackf,char *tstackh,char *tstacki,
	char *tloop,char *miscloop, char *danglef, char *int22, char *int21,
	char *coax,char *tstackcoax,char *coaxstack,
	char *tstack,char *tstackm, char *triloop, char *int11,
	datatable* data);

//	converts base to a numeric
void tonum(char *base,structure *ct,int count);

//	get name of file to output
void getout (char *energyfile);
										//	energy info
//	reads ct file
void openct(structure *ct,char *ctfile);
//	energy calculator
void efn2(datatable *data,structure *ct, int structnum = 0);
//	push info onto the stack
void push(stackstruct *stack,int a,int b,int c,int d);
//	pull info from the stack
void pull(stackstruct *stack,int *i,int *j,int *open,int *null,int *stz);
//	calculates energy of stacked base pairs
int erg1(int i,int j,int ip,int jp,structure *ct,datatable *data);
//	calculates energy of a bulge/internal loop
int erg2(int i,int j,int ip,int jp,structure *ct,datatable *data,int a,
	int b);
//	calculates energy of a hairpin loop
int erg3(int i,int j,structure *ct,datatable *data,int dbl);
//	calculates energy of a dangling base
int erg4(int i,int j,int ip,int jp,structure *ct,datatable *data,
	bool lfce);
//	calculates end of helix penalty
int penalty(int i,int j,structure *ct,datatable *data);
int penalty2(int i, int j, datatable *data);
void energyout(structure *ct,char *enrgyfile);

//	inputs a sequence from file seqfile
int openseq (structure *ct,char *seqfile);
//	outputs a ct file
void ctout (structure *ct,char *ctoutfile);

//	the dynamic folding algorithm of Zuker
//	cntrl6 = #tracebacks
//	cntrl8 = percent sort
//	cntrl9 = window
void dynamic (structure *ct,datatable *data,int cntrl6,int cntrl8,int cntrl9,
	TProgressDialog* update=0,char* savfile = 0);

//	Swap two variables
inline void swap(int *a,int *b);

//	converts base to a numeric
int tonumi(char *base);

//	line printer output of structure
void linout(structure *ct,char *file);
//	used by linout to write a digit into the line printer output file
void digit (int row,int column,int pos,arraystruct* table);

//	convert a numeric value for a base to the familiar character
char *tobase (int i);

//	this routine resorts the structures
//	predicted by dynamic according to energies calculated by efn2
void sortstructures (structure *ct);

//	function for outputting info in case of an error
void errmsg(int err,int err1);

//	function informs user of progress of fill algorithm
void update (int i);

//	the force functions are used to initialize the arrays used to apply
//	constraints during the folding process
void forcepair(int x,int y,structure *ct,arrayclass *v);
void forcesingle(int x,structure* ct,arrayclass *v);
void forcedbl(int dbl,structure* ct,int **w,bool *v);
void forceinter(int dbl,structure* ct,int **w);
void forceinterefn(int dbl,structure* ct,int **w);

//	filter is used to choose structures to output after efn2
//	this can make the number of structures more reasonable for inspection
//	it takes a structure, ct, which also contains the final output,
//	percent sort, maximum number of structures, and window size
void filter(structure* ct, int percent, int max, int window);

//	force is used to prepare arrays for the function dynamic, used during
//	the fill routines - it coordinates the force...() functions above
void force(structure *ct,arrayclass *v, int **fce, bool *lfce);

//	opens a save file with information filled by fill algorithm
void opensav(char* filename, structure* ct, arrayclass* w, arrayclass* v,
	int *w3, int *w5,int *vmin,datatable *data);

//	uses fill information to make a suboptimal structures
void traceback(structure *ct, datatable *data,
	arrayclass *v, arrayclass *w, int *w3, int *w5, int **fce,
	bool *lfce,int vmin, int cntrl6, int cntrl8, int cntrl9);

//	calculate the values of all the dots in a dot plot
void dotefn2(structure *ct, datatable *data, arrayclass *v, arrayclass *w,
	int *w3, int *w5, int **fce, bool *lfce,int vmin,dotarray *dots,
	TProgressDialog* PD = 0);
void calcpnum(dotarray *dots, int *pnum, int increment, int numofbases,
	TProgressDialog *PD = 0);

//	this function is used to make a save file after the fill algorithm
void savefile(int i, ofstream* sav);

//	read save files
int readfile(ifstream *read);
//	save dot plot info
void savedot(dotarray *dots,structure *ct, char *filename);
//	read a dot plot file
void readdot(dotarray *dots, structure *ct, char *filename);
//	dpalign will align two dot plots and store the info in the array align
void dpalign(dotarray *dots1,dotarray *dots2,
	structure* ct1,structure *ct2,int *align);
//	return the best dot for base i in dots1 and j in dots2
int getbestdot(dotarray *dots1,dotarray *dots2, structure* ct1,
	structure *ct2, int i, int j);
//	energydump will spit out the composite free energies for a traceback
void energydump (structure *ct, arrayclass *v, int n,char *filename);
//	energydump2 will spit out the composite free energies for a traceback
//	-- with the au penalty associated with the correct entity
void energydump2 (structure *ct, datatable *data,
	arrayclass *v, int n,char *filename);
#endif
