#if !defined(STRUCTURE_H)
#define STRUCTURE_H

#include "defines.h"

//	structure contains all the info read from thermodynamic data files
struct datatable
{
	int	poppen[5];
	int	maxpen;
	int	eparam[11];
	int	dangle[6][6][6][3];
	int	inter[31];
	int	bulge[31];
	int	hairpin[31];
	int	stack[6][6][6][6];
	int	tstkh[6][6][6][6];
	int	tstki[6][6][6][6];
	int	tloop[maxtloop+1][2];
	int	numoftloops;
	int	iloop22[6][6][6][6][6][6][6][6];
	int	iloop21[6][6][6][6][6][6][6];
	int	iloop11[6][6][6][6][6][6];
	int	coax[6][6][6][6];
	int	stackcoax[6][6][6][6];
	int	coaxstack[6][6][6][6],
	int	tstack[6][6][6][6];
	int	tstkm[6][6][6][6];
	int	auend;
	int	gubonus;
	int	cint;
	int	cslope;
	int	c3;
	int	efn2a;
	int	efn2b;
	int	efn2c;
	int	triloop[maxtloop+1][2];
	int	numoftriloops;
	int	init;
	int	gail;
	float prelog;
	datatable();
};

//	structure contains all the info for a structure
struct structure
{
	int	numofbases;
	int	numofstructures;
	int	pair[maxforce][2];
	int	npair;
	int	*numseq;
	int	*hnumber;
	int	**basepr;
	int	ndbl;
	int	dbl[maxforce];
	int	energy[maxstructures+1];
	int	inter[3];
	int	nnopair;
	int	nopair[maxforce];
	int	ngu;
	int	gu[maxgu];
	char 	ctlabel[maxstructures+1][ctheaderlength];
	char	*nucs;
	bool	intermolecular;
	bool	allocated;
	bool	templated;
	bool **tem;
	//int **fce;	//[maxbases+1][2*maxbases]
	structure();
	~structure();
	void	allocate(int size = maxbases);
	void	allocatetem();
/*
	structure is set up to hold many possible structures of the same
	sequence
	numofbases = number of bases in sequence
	numofstructures = number of alternative structures of the sequence
				that is held by structure
	numseq[i] = a numeric that stands for the base in the ith position
				of the sequence,
			A = 1
			C = 2
			G = 3
			U = 4
	basepr[i][j] = base to which the jth base is paired in the ith structure
	force[i] = any information about the way the ith pair is treated when
				folding; eg: forced single, etc
	energy[i] = 10 * the Gibb's free energy of the ith structure, this
				is done so that integer math can be performed
				and the nearest tenth of a kcal/mol can be
				followed
	ctlabel = a string of information for each of the structures
	fce = an array that can keep track of how each i,j pair is being
				forced
	hnumber array stores the historical numbering of a sequence
	nucs is a character array to store the sequence information --
   	this will allow the program to keep T and U from getting confused
*/

};

#endif
