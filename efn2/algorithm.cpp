/* 	RNA Secondary Structure Prediction, Using the Algorithm of Zuker
	C++ version by David Mathews, copyright 1996, 1997

	Programmed for Isis Pharmaceuticals and the Turner Lab,
	Chemistry Department, University of Rochester
*/

# include <stdio.h>
# include <string.h>
# include <iostream.h>
# include <fstream.h>              
# include <math.h>
# include <stdlib.h>

// platform.cpp contains information specific to the machine
# include "platform.cpp"

#define MAXFIL		100	// maximum length of file names
#define INFINITY	9999999	// an arbitrary value given to INFINITY
#define MAXTLOOP	100	// maximum tetraloops allowed (from tloop)
#define MAXSTRUCTURES	1010	// maximum number of structures in ct file
#define MAXBASES	10000	// maximum number of bases in a structure
#define CTHEADERLENGTH	125	// maximum length of sequence header
#define GA_BONUS	-10	// bonus value for the
				// "almost coaxial stacking" case in efn2
#define AMAX		400	// maximum line length for void linout (below)
#define COL		80	// number of columns in an output file
#define NUMLEN		8	// maximum digits in a number
#define MAXNOPAIR	600	// maximum number of forced single bases

// uncommented out for sgis
#include "structure.cpp"
#include "algorithm.h"

//	Functions:

/*
	Function efn2

	The energy calculator of Zuker
	calculates the free energy of each structural conformation in a
	structure

	structures cannot have pseudoknots

	structnum indicates which structure to calculate the free energy
   	the default, 0, indicates "all" structures
*/

void efn2(datatable *data,structure *ct, int structnum)
{
	int i,j,k,open,null,stz,count,sum,sum1,ip,jp;
	stackstruct stack;
	int **coax,**helix;
	int **fce;
	bool inter,flag;
	int start, stop;
	// int n5,n3;

	// stack = place to keep track of where efn2 is located in a structure
	// inter = indicates whether there is an intermolecular
	// 	interaction involved in a multi-branch loop

	// set stack counter
	stack.sp = 0;

	fce = new int *[ct->numofbases+1];
	for (i=0;i<=ct->numofbases;i++)
   		fce[i] = new int [ct->numofbases + 1-i];

	for (i=0;i<=ct->numofbases;i++) {
   		for (j=i;j<=ct->numofbases;j++) {
      			fce[i][j-i] = 0;
		}
	}

	// if true, indicates an intermolecular folding
	if (ct->intermolecular) {
		for (i=0;i<3;i++) {
			forceinterefn(ct->inter[i],ct,fce);
		}
	}

	if (structnum!=0) {
		start = structnum;
		stop = structnum;
	}else{
		start = 1;
		stop = ct->numofstructures;
	}

	// one structure at a time
	for (count=start;count<=stop;count++){

	    ct->energy[count]=0;
	    // put the whole structure onto the stack
	    push(&stack,1,ct->numofbases,1,0);

	    // loop starts here (I chose the goto statement to speed
	    // operation)
	    // take a substructure off the stack
	    subroutine: pull(&stack,&i,&j,&open,&null,&stz);

	    // cout << "pulled "<<i<<"  energy = "<<ct->energy[count]<<"\n";
	    // cin >> k;

	    while (stz!=1){
		// are i and j paired?
		while (ct->basepr[count][i]==j) {
		    // are i,j and i+1,j-1 stacked?
		    while (ct->basepr[count][i+1]==j-1){
			ct->energy[count] = ct->energy[count] +
				erg1(i,j,i+1,j-1,ct,data);
			i++;
			j--;
		    }
		    sum = 0;
		    k = i + 1;

		    // now efn2 is past the paired region, so 
		    // define the intervening non-paired segment
 
		    while (k<j) {
			if (ct->basepr[count][k]>k){
			    sum++;
			    ip = k;
			    k = ct->basepr[count][k] + 1;
			    jp = k-1;
			}else if (ct->basepr[count][k]==0)
			    k++;
		    }

		    if (sum==0) {		// hairpin loop

			ct->energy[count] = ct->energy[count] +
				erg3(i,j,ct,data,fce[i][j-i]);
			goto subroutine;

		    } else if (sum==1){		// bulge/internal loop

			ct->energy[count] = ct->energy[count] +
		erg2(i,j,ip,jp,ct,data,fce[i][ip-i],fce[jp][j-jp]);

	// cout << "after internal loop energy = "<<ct->energy[count]<<"\n";
	// cout << "the internal loop was = " 
	// 	<< er2(i,j,ip,jp,ct,data,fce[i][ip],fce[jp][j])<<"\n";
	// cout << "i = "<<i<<"  j =  "<<j<<"\n";

				i = ip;
				j = jp;
		    } else {		//multi-branch loop
			// total helixes = sum + 1
			sum = sum + 1;

			//initialize array helix and array coax
			helix = new int *[sum+1];
	    		for (k=0;k<=sum;k++)
				helix[k] = new int [2];

	    		coax = new int *[sum+1];
	    		for (k=0;k<=sum;k++)
				coax[k] = new int [sum+1];

			// find each helix and store info in
			// array helix
	    		// also place these helixes onto the
			// stack and calculate energy of the
			// intervening unpaired nucs
			helix[0][0] = i;
			helix[0][1] = j;

			inter = false;

			sum1 = 0;
			for (k=1;k<sum;k++) {
			    ip = helix[k-1][0]+1;
			    while(ct->basepr[count][ip]==0)
				ip++;
			    if(

			fce[helix[k-1][0]][ip-helix[k-1][0]]==5

			    ){
			    	inter = true;
			    }

			    // add? the terminal AU penalty 

			    ct->energy[count] =
	ct->energy[count] + penalty(ip,ct->basepr[count][ip],ct,data);

			    helix[k][1] = ip;
			    helix[k][0] = ct->basepr[count][ip];
			    push (&stack,ip, ct->basepr[count][ip],1,0);
			    sum1 = sum1 + (ip-helix[k-1][0]-1);
			}
			helix[sum][0] = helix[0][0];
			helix[sum][1] = helix[0][1];

			sum1 = sum1 + helix[sum][1]-helix[sum-1][0]-1;

			if (inter) {
			    // intermolecular interaction
			    // give the initiation penalty
			    ct->energy[count] = ct->energy[count]+data->init;
		    	}else{ 
			    // not an intermolecular interaction,
			    // treat like a normal multibranch loop:
			    // give the multibranch loop bonus:
			    ct->energy[count] = ct->energy[count] + data->efn2a;

			    // give the energy for each entering helix
			    ct->energy[count] = ct->energy[count] +
				sum*data->efn2c;

			    if (sum1<=6) {
				ct->energy[count]=
					ct->energy[count] + sum1*(data->efn2b);
			    } else {

	// cout << "efn2:  i = " << i << "   j = " << j <<
	//	"   " << int(11.*log(double(((sum1)/6.))) + 0.5) << "\n";
	// cout << "energy = "<<ct->energy[count]<<"\n";

				ct->energy[count]=
ct->energy[count] + 6*(data->efn2b) + int(11.*log(double(((sum1)/6.))) + 0.5);

			    }
			}

		    	// Now calculate the stacking energy:
			// k+1 indicates the number of helixes
			// to consider
		    	for (k=0;k<=sum;k++) {
			    if (k==0) {
				// this is the energy of stacking bases
				for (ip=0;ip<sum;ip++) {
				    coax[ip][ip]=0;
				    flag = false;
				    if(

			((ip==0) && ((helix[0][1]-helix[sum-1][0])>1))||
			((ip!=0)&&((helix[ip][1]-helix[ip-1][0])>1)))
				    {
					flag=true;
				    }
				    if(((helix[ip+1][1]-helix[ip][0])>1)&&flag){
					//use tstackm numbers
					//n5 = ct->numseq[helix[ip][0]+1];
					//n3 = ct->numseq[helix[ip][1]-1];

					coax[ip][ip] =
		data->tstkm[ct->numseq[helix[ip][0]]][ct->numseq[helix[ip][1]]]
		[ct->numseq[helix[ip][0]+1]][ct->numseq[helix[ip][1]-1]];

				    }else{
					// do not use tstackm numbers,
					// use individual terminal
					// stack numbers
					if ((helix[ip+1][1] - helix[ip][0])>1) {
					    coax[ip][ip] =
	min(0,erg4 (helix[ip][0],helix[ip][1], helix[ip][0]+1,1,ct,data,false));
					}
					if (ip==0) {
					    if((helix[0][1]-helix[sum-1][0])>1){
						coax[ip][ip] = coax[ip][ip] +
	min(0,erg4 (helix[ip][0],helix[ip][1], helix[ip][1]-1,2,ct,data,false));
					    }
					}else{
					    if((helix[ip][1]-helix[ip-1][0])>1){
						coax[ip][ip] = coax[ip][ip] +
	min(0,erg4 (helix[ip][0],helix[ip][1], helix[ip][1]-1,2,ct,data,false));
					    }
					}
				    }
				}
				  coax[sum][sum] = coax[0][0];
			    } else if (k==1) {
				// now consider whether coaxial stacking is
				// more favorable than just stacked bases
				for (ip=0;ip<sum;ip++) {
				    // cout << ip << "\n";
				    // see if they're close enough to stack
				    if ((helix[ip+1][1] - helix[ip][0])==1) {
					//flush stacking:
					coax[ip][ip+1] = min((coax[ip][ip] +
					    coax[ip+1][ip+1]),
					    data->coax[ct->numseq[helix[ip][1]]]
					    [ct->numseq[helix[ip][0]]]
					    [ct->numseq[helix[ip+1][1]]]
					    [ct->numseq[helix[ip+1][0]]]);
				    }else if(((helix[ip+1][1]-helix[ip][0])
						==2))
				    {
					//possible intervening mismatch:
					coax[ip][ip+1] = coax[ip][ip]+
					    coax[ip+1][ip+1];
					if (ip!=0) {
					    if((helix[ip][1]-helix[ip-1][0])>1){
						coax[ip][ip+1] =
		min( coax[ip][ip+1],
		    data->tstackcoax[ct->numseq[helix[ip][0]]]
		    [ct->numseq[helix[ip][1]]] [ct->numseq[helix[ip][0]+1]]
		    [ct->numseq[helix[ip][1]-1]] +
		    data->coaxstack[ct->numseq[helix[ip][0]+1]]
		    [ct->numseq[helix[ip][1]-1]][ct->numseq[helix[ip+1][1]]]
		    [ct->numseq[helix[ip+1][0]]]);

					    }
					}else{
					    //ip==0
					    if((helix[0][1]-helix[sum-1][0])>1){
					      coax[ip][ip+1] =
		min(coax[ip][ip+1],
		    data->tstackcoax[ct->numseq[helix[ip][1]]]
		    [ct->numseq[helix[ip][0]]][ct->numseq[helix[ip][0]+1]]
		    [ct->numseq[helix[ip][1]-1]] +
		    data->coaxstack[ct->numseq[helix[ip][0]+1]]
		    [ct->numseq[helix[ip][1]-1]][ct->numseq[helix[ip+1][1]]]
		    [ct->numseq[helix[ip+1][0]]]);
					    }
					}

					if (ip!=(sum-1)){
					    if((helix[ip+2][1]-helix[ip+1][0])
						>1)
					    {
						coax[ip][ip+1] =
		min(coax[ip][ip+1],
		    data->tstackcoax[ct->numseq[helix[ip][0]+1]]
		    [ct->numseq[helix[ip+1][0]+1]][ct->numseq[helix[ip+1][1]]]
		    [ct->numseq[helix[ip+1][0]]] +
		    data->coaxstack[ct->numseq[helix[ip][0]]]
		    [ct->numseq[helix[ip][1]]][ct->numseq[helix[ip][0]+1]]
		    [ct->numseq[helix[ip+1][0]+1]]);
					   }
					}else{
					    //ip = sum - 1
					    if((helix[1][1]-helix[0][0])>1){
					       coax[ip][ip+1] =
		min(coax[ip][ip+1],
		    data->tstackcoax[ct->numseq[helix[ip][0]+1]]
		    [ct->numseq[helix[ip+1][0]+1]][ct->numseq[helix[ip+1][1]]]
		    [ct->numseq[helix[ip+1][0]]] +
		    data->coaxstack[ct->numseq[helix[ip][0]]]
		    [ct->numseq[helix[ip][1]]][ct->numseq[helix[ip][0]+1]]
		    [ct->numseq[helix[ip+1][0]+1]]);
					    }
					}
				    }else{
					// no possible stacks
					coax[ip][ip+1] = coax[ip][ip] +
					    coax[ip+1][ip+1];
				    }
				}
			    }else if (k>1&&k<sum){
				for (i=0;(i+k)<=sum;i++){
					coax[i][i+k]=coax[i][i]+coax[i+1][i+k];
					for (j=1;j<k;j++) {
					coax[i][i+k] = min(coax[i][i+k],
						coax[i][i+j]+coax[i+j+1][i+k]);
				     }
				}
			    }else if (k==sum){
				ct->energy[count] =
			ct->energy[count] + min(coax[0][sum-1],coax[1][sum]);

				// cout << "efn2:  cs = " << "   "
				//	<< min(coax[0][sum-1],coax[1][sum])
				//	<< "\n";


				// cout << "structure = "<<count<<"\n";
				// cout << "helix[0][0] = "<<helix[0][0]<<"\n";

				// cout << min(coax[0][sum-1],coax[1][sum])
				// 	<< "\n";
				// for (i=0;i<=sum;i++) {
				//	for (j=i;j<=sum;j++) {
				//		cout << "i " << i <<
				//			"  j " << j <<
				//			"  coax[i][j] = " << 
				//			coax[i][j]<<"\n";
				//	}
				// }

			     }
			}

		    	for (k=0;k<=sum;k++) delete[] helix[k];
		    	delete[] helix;
		    	for (k=0;k<=sum;k++) delete[] coax[k];
		    	delete[] coax;

			goto subroutine;
		    }
		}

		// this is the exterior loop: ,i = 1
		// Find the number of helixes exiting the loop,
		// store this in sum:
		sum = 0;
		while (i<ct->numofbases) { 	
		    if (ct->basepr[count][i]!=0) {
			sum++;
		    	i = ct->basepr[count][i];
		    }
		    i++;
		}

		// initialize array helix and array coax
		helix = new int *[sum];
		for (k=0;k<sum;k++)
		    helix[k] = new int [2];

		coax = new int *[sum];
		for (k=0;k<sum;k++)
		    coax[k] = new int [sum];

		// find each helix and store info in array helix
		// also place these helixes onto the stack
		ip = 1;
		for (k=0;k<sum;k++) {
		    while (ct->basepr[count][ip]==0)
			ip++;
		    // add terminal au penalty if necessary
		    ct->energy[count]=ct->energy[count]+
		    penalty(ip,ct->basepr[count][ip],ct,data);
		    helix[k][1] = ip;
		    helix[k][0] = ct->basepr[count][ip];
		    push (&stack,ip,ct->basepr[count][ip],1,0);
		    ip = ct->basepr[count][ip]+1;
		}

		// Now calculate the energy of stacking:

		for (k=0;k<sum;k++) {
		    // k+1 indicates the number of helixes consider
		    if (k==0) {
			//this is the energy of stacking bases
			for (ip=0;ip<sum;ip++) {
			    coax[ip][ip] = 0;
			    if (ip<sum-1) {
				//not at 3' end of structure
				if ((helix[ip+1][1] - helix[ip][0])>1) {
				    // try 3' dangle
					coax[ip][ip] =
	min(0,erg4 (helix[ip][0],helix[ip][1],helix[ip][0]+1,1,ct,data,false));
				}
			    } else {
				//at 3' end of structure
				if ((ct->numofbases - helix[ip][0])>=1) {
				    // try 3' dangle
				    coax[ip][ip] =
	min(0,erg4 (helix[ip][0],helix[ip][1],helix[ip][0]+1,1,ct,data,false));
				}
			    }
			    if (ip==0) {
				if ((helix[0][1])>1) {
				    coax[ip][ip] = coax[ip][ip] +
	min(0,erg4 (helix[ip][0],helix[ip][1], helix[ip][1]-1,2,ct,data,false));
				}
			    }else{
				if ((helix[ip][1]-helix[ip-1][0])>=1) {
				    coax[ip][ip] = coax[ip][ip] +
	min(0,erg4 (helix[ip][0],helix[ip][1], helix[ip][1]-1,2,ct,data,false));
				}
			    }
			}
		    } else if (k==1) {
			//now consider whether coaxial stacking is
			//more favorable than just stacked bases
			for (ip=0;ip<sum-1;ip++) {
			    //see if they're close enough to stack
			    if ((helix[ip+1][1] - helix[ip][0])==1) {
				//flush stacking:
				coax[ip][ip+1] =
	min((coax[ip][ip] + coax[ip+1][ip+1]),
	    data->coax[ct->numseq[helix[ip][1]]]
	    [ct->numseq[helix[ip][0]]][ct->numseq[helix[ip+1][1]]]
	    [ct->numseq[helix[ip+1][0]]]);
			    } else if (((helix[ip+1][1] - helix[ip][0])==2)) {
				//possible intervening mismatch:
				coax[ip][ip+1] = coax[ip][ip]+coax[ip+1][ip+1];
				if (ip!=0) {
				    if ((helix[ip][1] - helix[ip-1][0])>1) {
					coax[ip][ip+1] =
	min(coax[ip][ip+1],
	    data->tstackcoax[ct->numseq[helix[ip][0]]]
	    [ct->numseq[helix[ip][1]]][ct->numseq[helix[ip][0]+1]]
	    [ct->numseq[helix[ip][1]-1]]+
	    data->coaxstack[ct->numseq[helix[ip][0]+1]]
	    [ct->numseq[helix[ip][1]-1]] [ct->numseq[helix[ip+1][1]]]
	    [ct->numseq[helix[ip+1][0]]]);
				    }
				}else{
				    // ip==0
				    if ((helix[0][1])>1) {
					coax[ip][ip+1] =
	min(coax[ip][ip+1],
	    data->tstackcoax[ct->numseq[helix[ip][1]]]
	    [ct->numseq[helix[ip][0]]] [ct->numseq[helix[ip][0]+1]]
	    [ct->numseq[helix[ip][1]-1]] +
	    data->coaxstack[ct->numseq[helix[ip][0]+1]]
	    [ct->numseq[helix[ip][1]-1]][ct->numseq[helix[ip+1][1]]]
	    [ct->numseq[helix[ip+1][0]]]);
				    }
				}

				if (ip!=(sum-2)) {
				    if ((helix[ip+2][1]-helix[ip+1][0])>1) {
					coax[ip][ip+1] =
	min(coax[ip][ip+1],
	    data->tstackcoax[ct->numseq[helix[ip][0]+1]]
	    [ct->numseq[helix[ip+1][0]+1]][ct->numseq[helix[ip+1][1]]]
	    [ct->numseq[helix[ip+1][0]]] +
	    data->coaxstack[ct->numseq[helix[ip][0]]]
	    [ct->numseq[helix[ip][1]]][ct->numseq[helix[ip][0]+1]]
	    [ct->numseq[helix[ip+1][0]+1]]);
				    }
				}else{
				    //ip = sum - 2
				    if (helix[sum-1][0]<ct->numofbases) {
					coax[ip][ip+1] =
	min(coax[ip][ip+1],
	    data->tstackcoax[ct->numseq[helix[ip][0]+1]]
	    [ct->numseq[helix[ip+1][0]+1]][ct->numseq[helix[ip+1][1]]]
	    [ct->numseq[helix[ip+1][0]]] +
	    data->coaxstack[ct->numseq[helix[ip][0]]]
	    [ct->numseq[helix[ip][1]]][ct->numseq[helix[ip][0]+1]]
	    [ct->numseq[helix[ip+1][0]+1]]);
				    }
				}
			    }else{
				//no possible stacks
				coax[ip][ip+1]=coax[ip][ip] + coax[ip+1][ip+1];
			    }
			}
		    }else if (k>1){
			for (i=0;(i+k)<sum;i++) {
			    coax[i][i+k] = coax[i][i]+coax[i+1][i+k];
			    for (j=1;j<k;j++) {
				coax[i][i+k] = min(coax[i][i+k],
				    coax[i][i+j]+coax[i+j+1][i+k]);
			    }
			}
		    }
		    if (k==(sum-1)) {
			ct->energy[count] = ct->energy[count] + coax[0][sum-1];
			// cout << "count = " << count << "\n" << flush;
			// for (i=0;i<sum;i++) {
			//     for (j=i;j<sum;j++) {
			//         cout << "i " << i << "  j " << j
			//             << "  coax[i][j] = " << coax[i][j]
			//             <<"\n";
			// 	}
			// }
			// cin >> i;
		    }
		}

		for (k=0;k<sum;k++)
		    delete[] helix[k];
		delete[] helix;
		for (k=0;k<sum;k++)
		    delete[] coax[k];
		delete[] coax;

		goto subroutine;

	/* Original efn code:
	 * 
	 *	// whittle away at 5' end:
	 *	while (ct->basepr[count][i]==0&&ct->basepr[count][i+1]==0) {
	 *		i++;
	 *		if (i>=(j-1)) goto subroutine;
	 *	}
	 *	// whittle away at 3' end
	 *	while (ct->basepr[count][j]==0&&ct->basepr[count][j-1]==0) {
	 *		j--;
	 *		if (i>=j-1) goto subroutine;
	 *	}
	 *	// i dangles on i+1
	 *	if (ct->basepr[count][i]==0&&ct->basepr[count][i+1]>(i+1)) {
	 *		ct->energy[count] = ct->energy[count]+
	 *		min(0,erg4(ct->basepr[count][i+1],(i+1),i,2,ct,data));
	 *		i++;
	 *	}
	 *	// j dangles on j-1
	 *	if ((ct->basepr[count][j]==0&&ct->basepr[count][j-1]!=0)&&
	 *		ct->basepr[count][j-1]<(j-1)) {
	 *		ct->energy[count]=ct->energy[count] +
	 *		min(0,erg4(j-1,ct->basepr[count][j-1],j,1,ct,data));
	 *		j--;
	 *	}
	 *	// structure bifurcates
	 *	if (ct->basepr[count][i]!=j) {
	 *		k = ct->basepr[count][i];
	 *		if (ct->basepr[count][k+1]!=0) {
	 *			push (&stack,i,k,open,0);
	 *			push (&stack,k+1,j,open,0);
	 *		}else if(ct->basepr[count][k+2]==0) {
	 *			push (&stack,i,k+1,open,0);
	 *			push (&stack,k+2,j,open,0);
	 *		}else if (erg4(k,i,k+1,1,ct,data)<=
	 *		    erg4((ct->basepr[count][k+2]),k+2,k+1,2,ct,data))
	 *		{
	 *			push (&stack,i,k+1,open,0);
	 *			push (&stack,k+2,j,open,0);
	 *		}else{
	 *			push (&stack,i,k,open,0);
	 *			push (&stack,k+1,j,open,0);
	 *		}
	 *		goto subroutine;
	 *	}
	 *	push (&stack,i,j,open,null);
	 *	goto subroutine;
	*/

	    }
	}

	for (i=0;i<=ct->numofbases;i++)
   		delete[] fce[i];
	delete[] fce;

	return;
}

void trace(structure *ct, datatable *data, int ii, int ji,
	arrayclass *v, arrayclass *w, arrayclass *w2,
	int rep, bool *lfce, int **fce, int *w3, int *w5 )
{
	int k,i,j,en,open,stz,ep,d,ip,jp;
	stackstruct stack;
	register int number;
	ofstream out;

	// if filename is not empty, open the file to add an energy
	// breakdown -- this is mostly important for debugging
	// if (filename != 0) {
	// 	out.open(filename);
	// }

	number = ct->numofbases;
	// Here is the trace algorithm:
	// When count = 1, it provides the best structure on the included
	// fragment (ie: iret to jret)
	// When count = 2, it provides the best structure on the excluded
	// fragment

	// Set the stack counter to zero
	stack.sp = 0;

	// Zero the basepr array:
	if (ji<=(number)) {
		for (k=ii;k<=ji;k++) ct->basepr[rep][k] = 0;
	} else {
		for (k=1;k<=(ji-(number));k++)
			ct->basepr[rep][k]=0;
		for (k=ii;k<=(number);k++)
			ct->basepr[rep][k]=0;
	}

	// Add forced pairs to the basepair array
	for (k=1;k<=ct->npair;k++) {
		ct->basepr[rep][ct->pair[k][0]]=ct->pair[k][1];
		ct->basepr[rep][ct->pair[k][1]]=ct->pair[k][0];
	}
	push (&stack,ii,ji,v->f(ii,ji),0);
         
	j = 0;
	sub100: ;
	i = j;
	while(i==j) {
		// Take a fragment from the stack, ie: i to j
		//	with expected energy e
		//	open = 0 =>multi loop
		// open = 1 =>exterior loop

		pull (&stack,&i,&j,&en,&open,&stz);
		if (stz!=0)
			return;
	}

	// if (i==310&&j==330) {
	//	ct->energy[0] = 0;
	// }

	//If i and j are paired, goto sub300
	if (en==v->f(i,j))
		goto sub300;

	if (open==0) {
		if (!ct->intermolecular) {
            		// whittle away at 5' end
			while (en==(w->f(i+1,j)+data->eparam[6])) {
				i++;
				en=w->f(i,j);
				if(i>=j)
					goto sub100;
			}
            		// whittle away at 3' end
			while (en==(w->f(i,j-1)+data->eparam[6])) {
				j--;
				en=w->f(i,j);
				if (i>=j)
					goto sub100;
			}
		} else {
			// whittle away at 5' end
			while (en==(w2->f(i+1,j))&&fce[i][i]!=2) {
				i++;
				en=w2->f(i,j);
				if(i>=j)
					goto sub100;
			}
            		// whittle away at 3' end
			while (en==(w2->f(i,j-1))&&fce[j][j]!=2) {
				j--;
				en=w2->f(i,j);
				if (i>=j)
					goto sub100;
			}

            		// whittle away at 5' end
			while (en==(w2->f(i+1,j)+INFINITY+data->init) &&
				fce[i][i]==2)
			{
				i++;
				en=w2->f(i,j);
				if(i>=j)
					goto sub100;
			}
            		// whittle away at 3' end
			while (en==(w2->f(i,j-1)+INFINITY+data->init) &&
				fce[j][j]==2)
			{
				j--;
				en=w2->f(i,j);
				if (i>=j)
					goto sub100;
			}
            		// whittle away at 5' end
			while (en==(w->f(i+1,j)+data->eparam[6])) {
				i++;
				en=w->f(i,j);
				if(i>=j)
					goto sub100;
			}
            		// whittle away at 3' end
			while (en==(w->f(i,j-1)+data->eparam[6])) {
				j--;
				en=w->f(i,j);
				if (i>=j)
					goto sub100;
			}
           	}

		if (en==(v->f(i+1,j)+data->eparam[10]+data->eparam[6] +
			erg4(j,i+1,i,2,ct,data,lfce[i])+penalty(i+1,j,ct,data)))
		{
			// i dangles over i+1,j
			i++;
			en=v->f(i,j);
		} else if (en==v->f(i,j-1)+data->eparam[10]+data->eparam[6] +
			erg4(j-1,i,j,1,ct,data,lfce[j])+penalty(i,j-1,ct,data))
		{
			// j dangles over i,j-1
			j--;
			en=v->f(i,j);
		}else if(en==(v->f(i+1,j-1)+data->eparam[10]+2*data->eparam[6] +
			erg4(j-1,i+1,i,2,ct,data,lfce[i]) +
			erg4(j-1,i+1,j,1,ct,data,lfce[j]) +
			enalty(j-1,i+1,ct,data)))
		{
			// both i and j dangle over i+1,j-1
			i++;
			j--;
			en=v->f(i,j);
		}

		// check for stem closing a multi-loop
		if (en==(v->f(i,j)+data->eparam[10]+penalty(j,i,ct,data))) {
			en=v->f(i,j);
		}

		if (ct->intermolecular) {
			if (en==(v->f(i+1,j)+INFINITY+
				erg4(j,i+1,i,2,ct,data,lfce[i]) + 
				penalty(i+1,j,ct,data)))
			{
				// i dangles over i+1,j
       				i++;
				en=v->f(i,j);
			} else if (en==v->f(i,j-1) + INFINITY +
				erg4(j-1,i,j,1,ct,data,lfce[j]) +
				penalty(i,j-1,ct,data))
			{
				// j dangles over i,j-1
				j--;
				en=v->f(i,j);
			} else if (en==(v->f(i+1,j-1)+INFINITY +
				erg4(j-1,i+1,i,2,ct,data,lfce[i]) +
				erg4(j-1,i+1,j,1,ct,data,lfce[j]) +
				enalty(j-1,i+1,ct,data)))
			{
				// both i and j dangle over i+1,j-1
				i++;
				j--;
				en=v->f(i,j);
			}
			// check for stem closing a multi-loop
			if (en==(v->f(i,j)+INFINITY+penalty(j,i,ct,data))) {
				en=v->f(i,j);
			}

		}
         }else{
		// whittle away at 5' end
		while ((j==number)&&(en==w3[i+1])){ //&& (fce[i][i]!=2))
			i++;
			if (i>=j)
				goto sub100;
		}

            	// whittle away at 3' end
		while ((i==1)&&(en==w5[j-1])){ //&&(fce[j][j]!=2))
			j--;
			if (i>=j)
				goto sub100;
		}

		// remove the initiation penalty from exterior loops:
	/*
         *  	// whittle away at 5' end
	 *	while ((j==number)&&(en==(w3[i+1]+data->init))&&(fce[i][i]==2)){
         *		en = w3[i+1];
         *		i++;
         *		if (i>=j)
	 *			goto sub100;
         *	}
	 *
         *   	// whittle away at 3' end
         *	while ((i==1)&&(en==(w5[j-1]+data->init))&&(fce[j][j]==2)) {
         *		en = w5[j-1];
         * 		j--;
         *		if (i>=j)
	 *			goto sub100;
         *	}
         *	// whittle away at 5' end
         *	while ((j==number)&&(en==w3[i+1])&&(fce[i][i]!=2)) {
         *		i++;
         *		if (i>=j)
	 *			goto sub100;
         *	}
	 *
         *	//whittle away at 3' end
         *	while ((i==1)&&(en==w5[j-1])&&(fce[j][j]!=2)) {
         *		j--;
         *		if (i>=j)
	 *			goto sub100;
         *	}
	 */

		// remove the initiation penalty from exterior loops:
        	if (en==(v->f(i+1,j)+erg4(j,i+1,i,2,ct,data,lfce[i])+
			penalty(i+1,j,ct,data)))
		{
            		// i dangles over i+1,j
			i++;
			en=v->f(i,j);
		} else if (en==(v->f(i,j-1)+erg4(j-1,i,j,1,ct,data,lfce[j])+
			penalty(j-1,i,ct,data)))
		{
			// j dangles over i,j-1
			j--;
			en = v->f(i,j);
		} else if (en==(v->f(i+1,j-1)+erg4(j-1,i+1,i,2,ct,data,lfce[i])+
	            	erg4(j-1,i+1,j,1,ct,data,lfce[j])+
			penalty(j-1,i+1,ct,data)))
		{
			// both j and i dangle over i+1,j-1
			i++;
			j--;
			en=v->f(i,j);
		} else if (en==(v->f(i,j)+penalty(i,j,ct,data))) {
             		en = v->f(i,j);
           	}
	}

        // At this point, the ends don't base pair and don't simply stack
        // on a helix, meaning the structure must bifurcate
        if (en!=v->f(i,j)) {
		k=i;
		while (k<(j-2)) {
			if ((open==0)&&(en==(w->f(i,k)+w->f(k+1,j)))) {
				// best structure is split into best
				// structures on i,k and k+1,j
				push (&stack,i,k,w->f(i,k),0);
				push (&stack,k+1,j,w->f(k+1,j),0);
				goto sub100;
			} else if ((open==1)&&(i==1)) {

				// else if ((open==0) &&
				//	(en==(w->f(i,k)+w->f(k+1,j)+
				//	penalty(i,j,ct,data))))
				// {
				//	//best structure is split into best
				//	// structures on i,k and //k+1,j
				//	push (&stack,i,k,w->f(i,k),0);
				//	push (&stack,k+1,j,w->f(k+1,j),0);
				//	goto sub100;
				// } else if ((open==0) &&
				//	(en==(w->f(i,k)+w->f(k+1,j)-
				//	penalty(i,j,ct,data))))
				// {
				//	// best structure is split into best
				//	// structures on i,k and k+1,j
				//	push (&stack,i,k,w->f(i,k),0);
				//	push (&stack,k+1,j,w->f(k+1,j),0);
				//	goto sub100;
				// }

				if (en==(w5[k]+
					v->f(k+1,j)+penalty(j,k+1,ct,data)))
				{
					// best structure on 1,j splits into
					// 1,k and pair k+1,j
					push (&stack,1,k,w5[k],1);
					push (&stack,k+1,j,v->f(k+1,j),0);
					goto sub100;

					// else if (en==(w5[k]+v->f(k+1,j))) {
					//	// best structure on 1,j splits
					//	// into 1,k and pair k+1,j
					//	push (&stack,1,k,w5[k],1);
					//	push (&stack,k+1,j,v->f(k+1,j),
					//		0);
					//	goto sub100;
					// }

				} else if (en==(w5[k]+
					erg4(j,k+2,k+1,2,ct,data,lfce[k+1])+
					v->f(k+2,j)+penalty(j,k+2,ct,data)))
				{
					// best structure splits on 1,k splits
					// into 1,k and k+2 paired to j and k+1
					//  stacks onto the pair
					push (&stack,1,k,w5[k],1);
					push (&stack,k+2,j,v->f(k+2,j),0);
					goto sub100;
				} else if (en==(w5[k]+
					erg4(j-1,k+1,j,1,ct,data,lfce[j])+
					v->f(k+1,j-1)+
					penalty(j-1,k+1,ct,data)))
				{
					// best structure splits into best
					// structure on 1,k and the pair
					// k+1,j-1 with j stacked onto the pair
					push(&stack,1,k,w5[k],1);
					push(&stack,k+1,j-1,v->f(k+1,j-1),0);
					goto sub100;

				}else if(en==(w5[k]+
					erg4(j-1,k+2,k+1,2,ct,data,lfce[k+1]) +
		                  	erg4(j-1,k+2,j,1,ct,data,lfce[j]) +
					v->f(k+2,j-1) +
       					penalty(j-1,k+2,ct,data)))
				{
					// best structure on 1,j splits into
					// 1,k and the pair of k+2,j-1 and k+1
					// and j both stack
					push(&stack,1,k,w5[k],1);
					push(&stack,k+2,j-1,v->f(k+2,j-1),0);
					goto sub100;
				}

		// else if (en==(w5[k]+erg4(j-1,k+2,k+1,2,ct,data,lfce[k+1]) +
		// 	erg4(j-1,k+2,j,1,ct,data,lfce[j]) + v->f(k+2,j-1)
		//    )) {
		//    //best structure on 1,j splits into 1,k and the pair
		//	of k+2,j-1 and k+1 and j both stack
		//    push(&stack,1,k,w5[k],1);
		//    push(&stack,k+2,j-1,v->f(k+2,j-1),0);
		//    goto sub100;
		// }

			} else if ((open==1)&&(j==number)) {
				if (en==(v->f(i,k+2)+
					w3[k+3]+penalty(k+2,i,ct,data)))
				{
					// best structure on i,n splits into
					// pair of i,k+2 and k+3,number
					push (&stack,i,k+2,v->f(i,k+2),0);
					push(&stack,k+3,number,w3[k+3],1);
					goto sub100;
				} else if (en==(v->f(i+1,k+2)+
					erg4(k+2,i+1,i,2,ct,data,lfce[i]) +
					w3[k+3]+penalty(i+1,k+2,ct,data)))
				{
					// best structure on i,n splits into
					// best structure of
       					// pair i+1,k+2 with i stacked and
					// k+3,number
					push (&stack,i+1,k+2,v->f(i+1,k+2),0);
					push (&stack,k+3,number,w3[k+3],1);
					goto sub100;
				} else if (en==(v->f(i,k+1)+
					erg4(k+1,i,k+2,1,ct,data,lfce[k+2])+
					w3[k+3]+penalty(i,k+1,ct,data)))
				{
					// best structure on i,n splits into
					// pair i,k+1 with k+2 stacked and
					// k+3,number
					push (&stack,i,k+1,v->f(i,k+1),0);
					push (&stack,k+3,number,w3[k+3],1);
					goto sub100;
				} else if (en==(v->f(i+1,k+1)+
					erg4(k+1,i+1,i,2,ct,data,lfce[i])+
					erg4(k+1,i+1,k+2,1,ct,data,lfce[k+2])+
					w3[k+3] +penalty(k+1,i+1,ct,data)))
				{
					// best structure on i,number splits
					// into pair of i+1,k+1
       					// with both i and k+2 stacked and
					// k+3,n
					push (&stack,i+1,k+1,v->f(i+1,k+1),0);
					push (&stack,k+3,number,w3[k+3],1);
					goto sub100;
				}
			}
			k++;
		}
		errmsg(100,1);
	}

	//Base pair found
	sub300: ;
       	if (j<=(number)) {
		ct->basepr[rep][i]=j;
		ct->basepr[rep][j]=i;
	}else if (i>(number)) {
		ct->basepr[rep][i-(number)] = j - (number);
		ct->basepr[rep][j-(number)] = i - (number);
		i = i - (number);
		j = j - (number);
	}else{
		ct->basepr[rep][j-(number)] = i;
		ct->basepr[rep][i] = j - (number);
	}
	open = 0;
	// does i,j stack over i+1,j-1
	if ((i!=(number))&&(j!=((number)+1))) {
		if (en==(erg1(i,j,i+1,j-1,ct,data) + v->f(i+1,j-1))) {
       			i++;
			j--;
			en = v->f(i,j);

			// output the stack value to dump file:
			// if (filename!=0) {
			//	out << "pair stack of " <<i <" and " << j <<
			//		" onto "<< (i-1) <<
			//		" and " << (j+1) << " dG = " <<
			//		(v->f(i-1,j+1)-v->f(i,j)) << "\n";
			// }

			goto sub300;
		}
	}

	// does i,j close a hairpin loop?
	if (en==erg3(i,j,ct,data,fce[i][j])) {
		// output the hairpin loop energy to dump file:
		// if (filename!=0) {
		//	out << "hairpin loop between nucleotides " << i <<
		//		" and " << j << " dG = " <<
		//		erg3(i,j,ct,data,fce[i][j]) << "\n";
         	// }
          	goto sub100;
	}

	// ep = e corrected for forced base pairs

	ep = en;

	if ((i+2)>(j-3)) {
		// tidy up loose ends
		if ((ep==0)||(ep==penalty(i,j,ct,data))||((i!=(number))&&
			(ep==erg4(i,j,i+1,1,ct,data,lfce[i+1])+
			penalty(i,j,ct,data))))
		{
			 goto sub100;
		}else if ((j!=((number)+1))&&
            		(ep==(erg4(i,j,j-1,2,ct,data,lfce[j-1])+
			penalty(i,j,ct,data))))
		{
			goto sub100;
		}else if ((i!=(number))&&(j!=((number)+1))&&
			(ep==(erg4(i,j,i+1,1,ct,data,lfce[i+1])+
			erg4(i,j,j-1,2,ct,data,lfce[j-1])+
			penalty(i,j,ct,data))))
		{
			goto sub100;
		}else{
            		errmsg (100,2);
			cin >> k;
		}
	} else if (i>=((number)-1)) {
         	// up to one base hanging on i
		if (ep==w5[j-(number)-1]+penalty(i,j,ct,data)) {
			push (&stack,1,j-(number)-1,w5[j-(number)-1],1);
			goto sub100;
		} else if (i!=(number)) {
			if (ep==(erg4(i,j,i+1,1,ct,data,lfce[i+1])+
				enalty(i,j,ct,data)+ w5[j-(number)-1]))
			{
				push (&stack,1,j-(number)-1,w5[j-(number)-1],1);
				goto sub100;
			} else if (ep==(erg4(i,j,i+1,1,ct,data,lfce[i+1]) +
				erg4(i,j,j-1,2,ct,data,lfce[j-1]) +
				w5[j-(number)-2] +penalty(i,j,ct,data)))
			{
				push (&stack,1,j-(number)-2,w5[j-(number)-2],1);
				goto sub100;
			}
		} else if (ep==(erg4(i,j,j-1,2,ct,data,lfce[j-1])+
			w5[j-(number)-2]+penalty(i,j,ct,data)))
		{
			push (&stack,1,j-(number)-2,w5[j-(number)-2], 1);
			goto sub100;
		}
	} else if ((j==((number)+1))||(j==(number)+2)) {
        	// up to one base hanging on j
		if (ep==w3[i+1]+penalty(i,j,ct,data)) {
			push (&stack,i+1,number,w3[i+1],1);
			goto sub100;
		} else if (ep == (erg4(i,j,i+1,1,ct,data,lfce[i+1])+
			penalty(i,j,ct,data)+ w3[i+2]))
		{
			push (&stack,i+2,number,w3[i+2],1);
			goto sub100;
		} else if (j!=((number)+1)) {
			if (ep==(erg4(i,j,j-1,2,ct,data,lfce[j-1])+
				penalty(i,j,ct,data)+w3[i+1]))
			{
				push(&stack,i+1,number,w3[i+1],1);
				goto sub100;
			} else if (ep==(erg4(i,j,i+1,1,ct,data,lfce[i+1])+
				erg4(i,j,j-1,2,ct,data,lfce[j-1]) + w3[i+2] +
				penalty(i,j,ct,data)))
			{
				push(&stack,i+2,number,w3[i+2],1);
				goto sub100;
			}
		}
	} else {
		// perhaps i,j closes a multi-branch loop
		k = i +2;
		while (k<=(j-3)) {
			if (k!=number) {
				if (ep==(w->f(i+1,k)+w->f(k+1,j-1)+
					data->eparam[10]+
					data->eparam[5] +
					penalty(i,j,ct,data)))
				{
					//multi loop, no dangling ends on i,j
					push (&stack,i+1,k,w->f(i+1,k),0);
					push (&stack,k+1,j-1,w->f(k+1,j-1),0);
					goto sub100;
				}else if (ep==
					(erg4(i,j,i+1,1,ct,data,lfce[i+1])+
					penalty(i,j,ct,data)+w->f(i+2,k)+
					w->f(k+1,j-1) +data->eparam[10]+
					data->eparam[6]+ data->eparam[5]))
				{
					// multi loop i and j basepaired with
					// i+1 dangling
					push (&stack,i+2,k,w->f(i+2,k),0);
					push (&stack,k+1,j-1,w->f(k+1,j-1),0);
					goto sub100;
				}else if (ep==
					(erg4(i,j,j-1,2,ct,data,lfce[j-1])+
					penalty(i,j,ct,data)+ w->f(i+1,k)+
					w->f(k+1,j-2)+data->eparam[10] +
					data->eparam[6] + data->eparam[5]))
				{
					// multi loop with j-1 dangling over a
					// basepair of i,j
					push (&stack,i+1,k,w->f(i+1,k),0);
					push (&stack,k+1,j-2,w->f(k+1,j-2),0);
					goto sub100;
				} else if (ep==
					(erg4(i,j,i+1,1,ct,data,lfce[i+1])+
					erg4(i,j,j-1,2,ct,data,lfce[j-1]) +
					w->f(i+2,k) + w->f(k+1,j-2) +
					data->eparam[10] + 2*(data->eparam[6]) +
					data->eparam[5]+penalty(i,j,ct,data)))
				{
					// multi loop with i+1 and j-1 dangling
					//  over pair i,j
					push(&stack,i+2,k,w->f(i+2,k),0);
					push(&stack,k+1,j-2,w->f(k+1,j-2),0);
					goto sub100;
				}
			}else{
				if (ep== (w3[i+1]+w5[j-(number)-1]+
					penalty(i,j,ct,data)))
				{
					// exterior loop, no dangling ends over
					//  i,j pair
					push(&stack,i+1,number,w3[i+1],1);
					push(&stack,1,(j-(number)-1),
						w5[j-(number)-1],1);
					goto sub100;
				}else if (ep==
					erg4(i,j,i+1,1,ct,data,lfce[i+1])+
					penalty(i,j,ct,data)+w3[i+2]+
					w5[j-(number)-1]))
				{
					// exterior loop i+1 dangles over i,j
					// pair
       					push(&stack,i+2,number,w3[i+2],1);
					push(&stack,1,j-(number)-1,
						w5[j-(number)-1],1);
					goto sub100;
				}else if (ep==
					(erg4(i,j,j-1,2,ct,data,lfce[j-1])+
					penalty(i,j,ct,data)+w3[i+1]+
					w5[j-(number)-2]))
				{
       					// exterior loop with j-1 dangling over
					//  i,j pair
					push(&stack,i+1,number,w3[i+1],1);
					push(&stack,1,j-(number)-2,
						w5[j-(number)-2],1);
					goto sub100;
				}else if (ep==
					(erg4(i,j,i+1,1,ct,data,lfce[i+1]) +
					erg4(i,j,j-1,2,ct,data,lfce[j-1]) +
					w3[i+2] + w5[j-(number)-2]+
					penalty(i,j,ct,data)))
				{
					// exterior loop with j-1 and i+1
					// dangling over i,j pair
					push(&stack,i+2,number,w3[i+2],1);
					push(&stack,1,j-(number)-2,
						w5[j-(number)-2],1);
					goto sub100;
				}
			}
			k++;
		}

		if (ct->intermolecular) {
            	    k = i +2;
            	    while (k<=(j-3)) {
            		if (k!=number) {
			    if (ep==(w2->f(i+1,k)+w2->f(k+1,j-1)
				+ penalty(i,j,ct,data)))
			    {
				// multi loop, no dangling ends on i,j
				push (&stack,i+1,k,w2->f(i+1,k),0);
				push (&stack,k+1,j-1,w2->f(k+1,j-1),0);
				goto sub100;
			    } else if (ep==
				(erg4(i,j,i+1,1,ct,data,lfce[i+1])+
				penalty(i,j,ct,data)+w2->f(i+2,k)+
				w2->f(k+1,j-1)))
			    {
				// multi loop i and j basepaired with
				// i+1 dangling
				push (&stack,i+2,k,w2->f(i+2,k),0);
				push (&stack,k+1,j-1,w2->f(k+1,j-1),0);
				goto sub100;
			    } else if (ep==
				(erg4(i,j,j-1,2,ct,data,lfce[j-1])+
				penalty(i,j,ct,data)+
				w2->f(i+1,k)+w2->f(k+1,j-2)))
			    {
				// multi loop with j-1 dangling over a
				// basepair of i,j
				push (&stack,i+1,k,w2->f(i+1,k),0);
				push (&stack,k+1,j-2,w2->f(k+1,j-2),0);
				goto sub100;
			    }else if (ep==
				(erg4(i,j,i+1,1,ct,data,lfce[i+1])+
				erg4(i,j,j-1,2,ct,data,lfce[j-1]) +
				w2->f(i+2,k) +
				w2->f(k+1,j-2) +penalty(i,j,ct,data)))
			    {
				// multi loop with i+1 and j-1 dangling
				// over pair i,j
				push(&stack,i+2,k,w2->f(i+2,k),0);
				push(&stack,k+1,j-2,w2->f(k+1,j-2),0);
				goto sub100;
			    }
			}
			k++;
            	    }
		}
	}

	// i,j must close a bulge or interior loop
	// sub500:
	for (d=(j-i-3);d>=1;d--) {
		for (ip=(i+1);ip<=(j-1-d);ip++) {
			jp=d+ip;
			if (abs(ip-i+jp-j)<=(data->eparam[8])) {
				if (en==(erg2(i,j,ip,jp,ct,data,fce[i][ip],
					fce[jp][j])+v->f(ip,jp)))
				{

// output the internal/bulge loop
// energy to the dump file:
// if (filename != 0) {
//	out << "bulge/internal loop starting with pair " << i << " and "
//		<< j << " ending with pair " << ip << " and " << jp <<
//		" dG = " << (v->f(i,j)-v->f(ip,jp)) <<"\n";
// }

					i=ip;
					j=jp;
					en=v->f(i,j);
					goto sub300;
				}
			}
		}
	}
}

#define	MAXSORT	90000	//The maximum number of basepairs within %cntrl8
			//of the minimum free energy

void traceback(structure *ct, datatable *data, arrayclass *v, arrayclass *w,
	arrayclass *w2,int *w3, int *w5, int **fce,
	bool *lfce,int vmin, int cntrl6, int cntrl8, int cntrl9)
{
	bool flag,**mark;
	register int number;
	int ii,sort;
	int rep,i,a;
	int iret,jret,numbp,count,count2,k1,k2,num,crit;
	// int heapi[MAXSORT+1],heapj[MAXSORT+1];
	int *heapi,*heapj;
	int cur,c,k,j,cntr;
	// stackstruct stack;
	int ji;	// (added during debugging)
	// int energy[MAXSORT+1];
	int *energy;
	// ofstream out("picked.out");

// cout << "traceback\n"<<flush;

	// mark keeps track of which pairs have been formed by the
	// suboptimal routine

	sort = MAXSORT;
	number = ct->numofbases;

	// Construct heapi and heapj composed of pairs ij such that a structure
	// containing ij has energy less than a given percent (ie:cntrl8) of
	// the minimum folding energy

	if (vmin>=0) {
		//no viable structure found
		ct->numofstructures=1;
		ct->energy[1]=0;
		for (i=1;i<=ct->numofbases;i++) {
			ct->basepr[1][i]=0;
		}
		return;
	}

	 // dynamically allocate space for mark:

	mark = new bool *[number + 1];
	for (i=0;i<=(number);i++)
		mark[i] = new bool [number + 1];

	// This is the traceback portion of the dynamic algorithm

	flag = true;
	rep = 1;

	for (count=1;count<=(number);count++) {
		for (count2=1;count2<=(number);count2++) {
   			mark[count][count2]=false;
		}
	}

	crit=abs(vmin)*(float (cntrl8)/100);

	// add limits to crit:
	if (crit>120)
		crit = 120;
	else if (crit<10)
		crit = 10;
	crit = crit + vmin;

	energy = new int [sort+1];
	heapi = new int [sort+1];
	heapj = new int [sort+1];

	num = 0;
	i = 1;
	j = 2;
	while (i<(number)) {
		if (num==sort) {
			// allocate more space for the heap
			delete[] heapi;
			delete[] heapj;
			delete[] energy;
			sort = 10*sort;
			heapi = new int [sort+1];
			heapj = new int [sort+1];
			energy = new int [sort+1];
			i = 1;
			j = 2;
			num = 0;
		}

// debugging:
// if (i==1&&j==26) {
// 	heapj[0] = (v->f(i,j)+v->f(j,i+number));
// }
//
// if ((v->f(i,j)+v->f(j,i+number))==crit) {
// 	heapi[0] = crit;
// }

		if ((v->f(i,j)+v->f(j,i+number))<=crit) {
			num++;
			heapi[num]=i;
			heapj[num]=j;
			energy[num] = (v->f(i,j)+v->f(j,i+number));
			j = j+cntrl9+1;
			if (j>number) {
				i++;
				j=i+1;
			}
		}else{
			j++;
			if (j>number) {
				i++;
				j=i+1;
			}
		}
	}

// debugging:
// ofstream out2("heap.out");
// out2 << crit << "\n";
// for (i=1;i<=num;i++) {
//	out2 << heapi[i] << " " << heapj[i] << " " <<
//		(v->f(heapi[i],heapj[i])+v->f(heapj[i],heapi[i]+number))
//		<<"\n";
// }

	// sort the base pair list:
	// make a heap:

	int q,up,ir;
	for (q=2;q<=num;q++) {
		cur = q;
		up = cur/2;
		while ((energy[cur]<energy[up])&&up>=1) {
			swap(&heapi[cur],&heapi[up]);
			swap(&heapj[cur],&heapj[up]);
			swap(&energy[cur],&energy[up]);
			cur = cur/2;
			up = up/2;
		}
	}

	// sort the heap:

	for (ir=num-1;ir>=1;ir--) {
		swap(&heapi[ir+1],&heapi[1]);
		swap(&heapj[ir+1],&heapj[1]);
		swap(&energy[ir+1],&energy[1]);

		up =1;
		c = 2;
		while (c<=ir) {
			if (c!=ir) {
				if (energy[c+1]<energy[c])
					c++;
			}
			if (energy[c]<energy[up]) {
				swap(&heapi[c],&heapi[up]);
				swap(&heapj[c],&heapj[up]);
				swap(&energy[c],&energy[up]);
				up=c;
				c=2*c;
			} else
				c = ir+1;
		}
	}

// out2 << "\n";
// for (i=1;i<=num;i++) {
//	out2 << heapi[i] << " " << heapj[i] << " " <<
//		(v->f(heapi[i],heapj[i])+v->f(heapj[i],heapi[i]+number))
//		<< "\n";
// }

	cntr = num;

// cout << "2.5" << flush;

	while (flag) {
	    // This is routine to select the region of the structure to be
	    // folded, it allows for sub-optimal structure predictions
	    // err=0;
	    // Select the next valid unmarked basepair
	    while (mark[heapi[cntr]][heapj[cntr]]) {
		if (cntr==1) {
		    flag=false;
		    goto sub900;
		}
		cntr--;
	    }
	    iret = heapi[cntr];
	    jret = heapj[cntr];
	    rep++;

	    // Traceback to find best structure on included fragment
	    // (ie:iret to jret)
	    if (flag) {
		for (count=1;count<=2;count++) {
		    if (count==1) {
			ii=iret;
			ji=jret;
		    }
		    if (count==2) {
			ii=jret;
			ji=iret+(number);
		    }
		    trace(ct,data,ii,ji,v,w,w2, ep-1,lfce,fce,w3,w5);
		    if (count==2) {
			ct->energy[rep-1] = v->f(iret,jret) +
			    v->f(jret,iret+(number));
			// count the number of new base pairs
			// not within window of existing base pairs
			numbp = 0;
			for (k=1;k<=number;k++) {
			    if (k<(ct->basepr[rep-1][k])) {
				if (!(mark[k][ct->basepr[rep-1][k]]))
				    numbp++;
			    }
			}
			for (k=1;k<=(number);k++) {
			    if (k<ct->basepr[rep-1][k]) {
       				// Mark "traced back" base pairs and also
				// base pairs which are within a window of
				// cntrl9
				mark[k][ct->basepr[rep-1][k]]=true;
				if (cntrl9>0) {
				    for (k1=-(cntrl9);k1<=cntrl9;k1++) {
					for (k2=-cntrl9;k2<=cntrl9;k2++) {
					    if (((k+k1)>0)&&((k+k1)<
						(ct->basepr[rep-1][k]))&&
						((ct->basepr[rep-1][k]+k2)
						<=(number)))
					    {

				mark[k+k1][(ct->basepr[rep-1][k])+k2]=true;

					    }
					}
				    }
				}
			    }
			}

			if (numbp<=cntrl9){
			    rep--;
			    goto sub900;
			}else{
			    // place the structure name (from ctlabel[1])
			    // into each structure
			    strcpy(ct->ctlabel[rep-1],ct->ctlabel[1]);

// out << (rep-1) << " picked pair: " << iret << " " << jret << "\n";

			}
			if (rep>cntrl6)
			    flag=false;
		    }
		}
		sub900:
		continue;
	    }
	}

(ct->numofstructures)=rep-1;

// for debugging:
// energydump (ct, v, 1,"dump.out");
// energydump (ct, data,v, 1,"dump-au.out");

	de_allocate (mark,number+1);
	delete[] energy;
	delete[] heapi;
	delete[] heapj;
}

void force(structure *ct,arrayclass *v, int **fce, bool *lfce)
{
	int i;
	register int number;

	number = ct->numofbases;

	for (i=1;i<=ct->nnopair;i++) {
		forcesingle(ct->nopair[i],ct,v);
	}

	for (i=1;i<=ct->npair;i++) {
		forcepair(ct->pair[i][0],ct->pair[i][1],ct,v);
	}

	for (i=1;i<=ct->ndbl;i++) {
		forcedbl(ct->dbl[i],ct,fce,lfce);
	}


	// u's in gu pairs must be double stranded
	for (i=0;i<ct->ngu;i++)
		forcedbl(ct->gu[i],ct,fce,lfce);

	if (ct->intermolecular) {
		// this indicates an intermolecular folding
		// for (i=0;i<=3;i++) {
		// 	forcesingle(ct->inter[i]);
		//	don't allow the intermolecular indicators to pair
		// }
		for (i=0;i<3;i++) {
			forceinter(ct->inter[i],ct,fce);
		}

		// mark the fce array so that initiation is added to
		// multibranch loops:
		// fce[ct->inter[0]][ct->inter[1]] = 2;
		// fce[ct->inter[1]][ct->inter[2]] = 2;
		// fce[ct->inter[0]][ct->inter[1]+number] = 2;
		// fce[ct->inter[1]][ct->inter[2]+number] = 2;

		fce[ct->inter[1]][ct->inter[1]] = 2;
	}

	// Double up the sequence
	for (i=1;i<=number;i++) {
		ct->numseq[(number)+i] = ct->numseq[i];
	}
}

#define MINLOOP 3 //The minimum substructure with a basepair possible

//This is the dynamic algorithm of Zuker:
void dynamic(structure* ct,datatable* data,int cntrl6, int cntrl8,int cntrl9,
	TProgressDialog* update, char* save)
{

// cout << "2!!!" << flush;

	// int **v,**w,
	int vmin,d,ip,jp,ii,jj;
	int e[6],i;
	int k,j,l,m,n,o,p;
	int inc[6][6] = {
		{0,0,0,0,0,0},
		{0,0,0,0,1,0},
		{0,0,0,1,0,0},
		{0,0,1,0,1,0},
		{0,1,0,1,0,0},
		{0,0,0,0,0,0}
	};
	bool *lfce; //[MAXBASES+1][MAXBASES+1];
	int before,after,key;
	int **fce;
	int **work,**work2;
	int *w5,*w3,**wmb,**wmb2;
	register int number,jmt;
	output convert;

	// array *v,*w;
	// inc is an array that saves time by showing which bases can pair
	// before erg is called

	// number is the number of bases being folded
	// v[i][j] is the best energy for subsequence i to j when i and j
	// are paired
	// w[i][j] is the best energy for subsequence i to j

	// place the number of bases in a registered integer
	number = (ct->numofbases);
	key = 0;

	// w = new array(number);
	// v = new array(number);

// cout << "preallocate\n";

	arrayclass w(number);
	arrayclass v(number);

	// add a second array for intermolecular folding:
	arrayclass *w2;
	if (ct->intermolecular) {
		w2 = new arrayclass(number);
		work2 = new int *[2*number+3];
		wmb2 = new int *[2*number+3];
		for (i=0;i<2*number+3;i++) {
			work2[i] = new int [3];
			wmb2[i] = new int [3];
			for (j=0;j<=2;j++)
				wmb2[i][j] = INFINITY;
		}
	}

	fce = new int *[2*number+1];
	for (i=0;i<=2*number;i++)
		fce[i] = new int [2*number + 1];

	for (i=1;i<=2*number;i++) {
		for (j=1;j<=2*number;j++) {
			fce[i][j] = 0;
		}
	}

	lfce = new bool [2*number+1];
	for (i=0;i<=2*number;i++)
		lfce[i] = false;

	work = new int *[2*number+3];
	wmb = new int *[2*number+3];
	for (i=0;i<2*number+3;i++) {
		work[i] = new int [3];
		wmb[i] = new int [3];
		for (j=0;j<=2;j++)
			wmb[i][j] = INFINITY;
	}

	w5 = new int [number+1];
	w3 = new int [number+2];
	for (i=0;i<=number;i++) {
		w5[i] = 0;
		w3[i] = 0;
	}
	w3[number+1] = 0;

	force(ct,&v,fce,lfce);

	// This is the fill routine:
	vmin=INFINITY;
	for (j=1;j<=(2*(number)-1);j++) {
		if (((j%10)==0)&&update)
			update->update((100*j)/(2*ct->numofbases));
		for (i=min(j,number);i>=max(1,j-(number)+1);i--) {

// debug
// if(i==25&&j==26) {
//	v.f(0,0) = 0;
// }

			wmb[i][j%3] = INFINITY;

			if (ct->templated) {
				if (i>ct->numofbases)
					ii = i - ct->numofbases;
				else
					ii = i;
				if (j>ct->numofbases)
					jj = j - ct->numofbases;
				else
					jj = j;
				if (jj<ii) {
					p = jj;
					jj = ii;
					ii = p;
				}
				if (!ct->tem[jj][ii])
				goto sub2;
			}

			// Compute v[i][j], the minimum energy of the
			// substructure from i to j,
			// inclusive, where i and j are base paired

			// if fce[i][j] = 1 or 3, these need to be set to
			// INFINITY because 1 indicates one of the bases
			// should be single-stranded and 3 indicates one of
			// the bases needs to be paired to a different base

			if (v.f(i,j)==1) {
				v.f(i,j) = INFINITY + 50;
				goto sub2;
			}
			if (v.f(i,j)==3) {
				v.f(i,j)= INFINITY+50;
				// w[i][j]= INFINITY;
				goto sub3;
			}
			if (v.f(i,j)==2) {
				// forcing a pair between i and j
				key = 2;
			}

			// v[i][j] = INFINITY; // added 8/18/97

			if (j<=(number)) {
				if ((j-i)<=MINLOOP)
					goto sub3;
			}else{
				if (i==(number)||j==((number)+1))
					goto sub1;
			}

			if (inc[ct->numseq[i]][ct->numseq[j]]==0)
				goto sub2;

			// force u's into gu pairs
			for (ip=0;ip<ct->ngu;ip++) {
   				if (ct->gu[ip]==i) {
					if (ct->numseq[j]!=3) {
						v.f(i,j) = INFINITY;
						goto sub2;
					}
				}else if (ct->gu[ip]==j) {

				// else if ((ct->gu[ip]+number)==i) {
				// 	if (ct->numseq[j]!=3) {
				//   	v.f(i,j) = INFINITY;
				//      goto sub2;
				//   }
				// }

					if (ct->numseq[i]!=3) {
						v.f(i,j) = INFINITY;
						goto sub2;
					}
				} else if ((ct->gu[ip]+number)==j) {
					if (ct->numseq[i]!=3) {
						v.f(i,j) = INFINITY;
						goto sub2;
					}
				}
			}

			// now check to make sure that this isn't an
			// isolated pair: (consider a pair separated by a
			// bulge as not! stacked)

			// before = 0 if a stacked pair cannot form 5' to i
			if (i>1&&j<(2*number)) {
				before = inc[ct->numseq[i-1]][ct->numseq[j+1]];

// What follows is to allow a bulge
// if (before == 0) {
//	if (i>2) {
//		if (incp[ct->numseq[i-2]][ct->numseq[j+1]]==1)
//			before = 1;
//	}else if (j-1<(2*number)) {
//		if (incp[ct->numseq[i-1]][ct->numseq[j+2]]==1)
//			before = 1;
//	}
// }

			} else
				before = 0;

			//after = 0 if a stacked pair cannot form 3' to i
			if ((j-i)>5) {
				after = inc[ct->numseq[i+1]][ct->numseq[j-1]];

// What follows is to allow a bulge
// if (after == 0) {
//	if ((incp[ct->numseq[i+2]][ct->numseq[j-1]]==1)&&((j-i)>6))
//		after = 1;
//	else if ((incp[ct->numseq[i+1]][ct->numseq[j-2]]==1)&&((j-i)>6))
//		after = 1;
// }

			}else
				after = 0;

			// if there are no stackable pairs to i.j then
			// don't allow a pair i,j
			if ((before==0)&&(after==0)) {
				goto sub2;
			}

			//Perhaps i and j close a hairpin:
			v.f(i,j)=min(v.f(i,j),erg3(i,j,ct,data,fce[i][j]));

      			// Perhaps i,j stacks over i+1,j-1
			if ((j-i-1)>=(MINLOOP+2)||j>(number))
         			v.f(i,j) =
					min(v.f(i,j),(erg1(i,j,i+1,j-1,ct,data)+
					v.f(i+1,j-1)));

			// Perhaps i,j closes an interior or bulge loop,
			// search for the best possibility
			if (((j-i-1)>=(MINLOOP+3))||(j>(number))) {
			    for (d=(j-i-3);d>=1;d--) {
				for (ip=(i+1);ip<=(j-1-d);ip++) {
				    jp = d+ip;
       				    if ((j-i-2-d)>(data->eparam[7]))
					goto sub1;
				    if (abs(ip-i+jp-j)<=(data->eparam[8])){
					if (ip>(number)) {
	// if (jp<=number) {
	//	v.f(i,j)=min(v.f(i,j),(erg2(i,j,ip,jp,ct,data,fce[i][ip-number],
	//		fce[jp][j])+
	//		v.f(ip-(number),jp)));
	// } else {

					    v.f(i,j) =
	min(v.f(i,j),(erg2(i,j,ip,jp,ct,data,fce[i][ip-number],
	fce[jp-number][j-number])+ v.f(ip-(number),jp-(number))));

	// }
					}else{
					    if (jp<=number) {

						v.f(i,j) =
	min(v.f(i,j),(erg2(i,j,ip,jp,ct,data,fce[i][ip],
	fce[jp][j])+ v.f(ip,jp)));

					    } else {

						v.f(i,j) =
	min(v.f(i,j),(erg2(i,j,ip,jp,ct,data,fce[i][ip],
       fce[jp-number][j-number])+ v.f(ip,jp)));

					    }
					}
				    }
				}
			    }
			}

			// Perhaps i,j closes a multi-loop, search for the
			// best possibility

     			sub1: ;

			if (((j-i-1)>=(2*MINLOOP+4))||(j>(number))) {
				for (ii=1;ii<=4;ii++)
					e[ii]=INFINITY;

				if (j>number) {
				    e[1] =
						w3[i+1] + w5[j-number-1] +
						penalty(i,j,ct,data);
	// if (e[1]==-1204) {
	//    	v.f(0,0) = 0;
	// }

				    if (i!=number)
					e[2] =
					    erg4(i,j,i+1,1,ct,data,lfce[i+1]) +
					    penalty(i,j,ct,data)+w3[i+2] +
					    w5[j-number-1];

				    if (j!=(number+1))
					e[3] =
					    erg4(i,j,j-1,2,ct,data,lfce[j-1]) +
					    penalty(i,j,ct,data)+ w3[i+1] +
					    w5[j-number-2];
	
       				    if ((i!=number)&&(j!=(number+1))) {
			            	e[4] =
					    erg4(i,j,i+1,1,ct,data,lfce[i+1]) +
					    erg4(i,j,j-1,2,ct,data,lfce[j-1]) +
					    w3[i+2] + w5[j-number-2] +
					    penalty(i,j,ct,data);
				    }
				}

				if ((j-i)>(2*MINLOOP+4)) {
					// no dangling ends on i-j pair:
					e[1] =
					    min(e[1],wmb[i+1][(j-1)%3]+
					    data->eparam[5]+data->eparam[10]+
					    penalty(i,j,ct,data));

					//i+1 dangles on i-j pair:
					if (i!=number)
					    e[2] =
				min(e[2],erg4(i,j,i+1,1,ct,data,lfce[i+1]) +
				penalty(i,j,ct,data) + wmb[i+2][(j-1)%3] +
				data->eparam[5] + data->eparam[6] +
				data->eparam[10]);

					//j-1 dangles
					if (j!=(number+1))
					    e[3] =
				min(e[3],erg4(i,j,j-1,2,ct,data,lfce[j-1]) +
				penalty(i,j,ct,data) + wmb[i+1][(j-2)%3] +
				data->eparam[5] + data->eparam[6] +
				data->eparam[10]);

					//both i+1 and j-1 dangle
					if ((i!=number)&&(j!=(number+1))) {
					    e[4] = min(e[4],
				erg4(i,j,i+1,1,ct,data,lfce[i+1]) +
				erg4(i,j,j-1,2,ct,data,lfce[j-1]) +
				wmb[i+2][(j-2)%3] + data->eparam[5] +
				2*data->eparam[6] + data->eparam[10] +
				penalty(i,j,ct,data));

					}

					if (ct->intermolecular) {
						// intermolecular, so consider
						// wmb2, // don't add the
						// multiloop penalties because
						// this is a exterior loop

						e[1] = min(e[1],
				wmb2[i+1][(j-1)%3] + penalty(i,j,ct,data));

       						// i+1 dangles on i-j pair:
						if (i!=number)
						     e[2] = min(e[2],
				erg4(i,j,i+1,1,ct,data,lfce[i+1]) +
				penalty(i,j,ct,data) + wmb2[i+2][(j-1)%3]);

						//j-1 dangles
						if (j!=(number+1))
						    e[3] = min(e[3],
				erg4(i,j,j-1,2,ct,data,lfce[j-1]) +
				penalty(i,j,ct,data) + wmb2[i+1][(j-2)%3] );

						//both i+1 and j-1 dangle
						if((i!=number)&&
							(j!=(number+1)))
						{
						    e[4] = min(e[4],
				erg4(i,j,i+1,1,ct,data,lfce[i+1]) +
				erg4(i,j,j-1,2,ct,data,lfce[j-1]) +
				wmb2[i+2][(j-2)%3] +penalty(i,j,ct,data));

						}
					}
				}

				v.f(i,j)=min(v.f(i,j),e[1]);
				v.f(i,j)=min(v.f(i,j),e[2]);
				v.f(i,j)=min(v.f(i,j),e[3]);
				v.f(i,j)=min(v.f(i,j),e[4]);
			}

			// Compute w[i][j]: best energy between i and j
			// where i,j does not have to be a base pair
			// (an exterior loop when it contains n and 1
			// (ie:n+1)   )

			sub2: ;

			// w[i][j]=INFINITY;

			if (key==2) {
				// force a pair between i and j
				w.f(i,j) = v.f(i,j);
				key = 0;
				goto sub3;
			}

			for (ii=1;ii<=5;ii++)
				e[ii] = INFINITY;


			if (i!=number) {
				// calculate the energy of i stacked onto
				// the pair of i+1,j

				e[1] = v.f(i+1,j) + data->eparam[10] +
					data->eparam[6] +
			         	erg4(j,i+1,i,2,ct,data,lfce[i]) +
					penalty(i+1,j,ct,data);

				if (!lfce[i]) {
					if (fce[i][i]!=2)
						e[4] = w.f(i+1,j) +
							data->eparam[6];
					// this is for when i represents the
					// center of an intermolecular linker:
					else
						e[4] = w.f(i+1,j) +
							data->eparam[6] +
							INFINITY;
				}
			}

			if (j!=((number)+1)) {
				// calculate the energy of j stacked onto
				// the pair of i,j-1
				if (j!=1) {
					e[2] = v.f(i,j-1) + data->eparam[10] +
						data->eparam[6] +
						erg4(j-1,i,j,1,ct,data,lfce[j])+
						penalty(i,j-1,ct,data);

					if (!lfce[j]) {
						if (fce[j][j]!=2) {
							e[5] = w.f(i,j-1) +
							    data->eparam[6];
						} else
							e[5] = w.f(i,j-1) +
							    data->eparam[6] +
							    INFINITY;

					}
				}
			}

			if ((i!=(number))&&(j!=((number)+1))) {
				// calculate i and j stacked onto the pair
				// of i+1,j-1
				if (j!=1) {
					e[3] = v.f(i+1,j-1) +
					    data->eparam[10] +
					    2*(data->eparam[6]) +
					    erg4(j-1,i+1,i,2,ct,data,lfce[i]) +
					    erg4(j-1,i+1,j,1,ct,data,lfce[j]) +
					    penalty(j-1,i+1,ct,data);
				}
			}

			w.f(i,j) =
				min(((data->eparam[10])+
				v.f(i,j)+penalty(j,i,ct,data)),e[1]);
			w.f(i,j) = min(w.f(i,j),e[2]);
			w.f(i,j) = min(w.f(i,j),e[3]);
			w.f(i,j) = min(w.f(i,j),e[4]);
			w.f(i,j) = min(w.f(i,j),e[5]);

			if (ct->intermolecular) {
				wmb2[i][j%3] = INFINITY;
				// keep track of w2:
				for (ii=1;ii<=5;ii++)
					e[ii] = 2*INFINITY;

				if (i!=number) {
					// calculate the energy of i stacked
					// onto the pair of i+1,j

					e[1] = v.f(i+1,j) +
					    erg4(j,i+1,i,2,ct,data,lfce[i])+
					    penalty(i+1,j,ct,data)+INFINITY;
					if (!lfce[i]) {
						if (fce[i][i]!=2)
						    e[4] = w2->f(i+1,j);
						    // this is for when i
						    // represents the center of
						    //  an intermolecular
						    // linker:
						else
						    e[4] = w2->f(i+1,j) -
							INFINITY + data->init;
					}
				}
				if (j!=((number)+1)) {
					// calculate the energy of j stacked
					// onto the pair of i,j-1
					if (j!=1) {
						e[2] = v.f(i,j-1) +
				INFINITY + erg4(j-1,i,j,1,ct,data,lfce[j]) +
				penalty(i,j-1,ct,data);

						if (!lfce[j]) {
						    if (fce[j][j]!=2) {
							e[5] = w2->f(i,j-1);
						    }else
							e[5] = w2->f(i,j-1) -
							    INFINITY +
							    data->init;

						}
					}
				}
				if ((i!=(number))&&(j!=((number)+1))) {
					// calculate i and j stacked onto the
					// pair of i+1,j-1
					if (j!=1) {
						e[3] = v.f(i+1,j-1) +
				INFINITY + erg4(j-1,i+1,i,2,ct,data,lfce[i]) +
				erg4(j-1,i+1,j,1,ct,data,lfce[j]) +
				penalty(j-1,i+1,ct,data);
					}
				}
				w2->f(i,j) =
					min((INFINITY+v.f(i,j)+
					penalty(j,i,ct,data)),e[1]);
				w2->f(i,j) = min(w2->f(i,j),e[2]);
				w2->f(i,j) = min(w2->f(i,j),e[3]);
				w2->f(i,j) = min(w2->f(i,j),e[4]);
				w2->f(i,j) = min(w2->f(i,j),e[5]);
			}

	/*
	 * if (((j-i-1)>(2*MINLOOP+2))||(j>(number))) {
	 * 	// search for an open bifuraction:
	 *	for (k=i;k<=j-1;k++) {
	 *		if (k==(number))
	 *			w.f(i,j)=min(w.f(i,j),
	 *		w3[i]+w5[j-(number)]);
	 *		else w.f(i,j) = min(w.f(i,j),
	 *		w.f(i,k)+work[k+1][j%3]);
	 *	}
	 * }
	 */

			// fill wmb:

			if (((j-i-1)>(2*MINLOOP+2))||j>number) {
				jmt = j%3;
				// search for an open bifurcation:
				for (k=i;k<=j-1;k++) {
					if (k!=number)
						wmb[i][jmt] =
				   min(wmb[i][jmt],w.f(i,k)+work[k+1][jmt]);
					else
						wmb[i][jmt] =
					min(wmb[i][jmt],w3[i]+w5[j-number]);
				}

				if (ct->intermolecular) {
					// intermolecular folding:
					// search for an open bifurcation:
					for (k=i;k<=j-1;k++) {
						if (k!=number) {
							wmb2[i][jmt] =
					    min(wmb2[i][jmt],w2->f(i,k) +
					    work[k+1][jmt]);
						}
					}
					w2->f(i,j) =
						min(w2->f(i,j),wmb2[i][j%3]);
				}
			}
			w.f(i,j) = min(w.f(i,j),wmb[i][j%3]);

			// Fill in work, the best energy for columns j,j-1,j-2
			sub3: ;

			work[i][j%3] = w.f(i,j);

			if (ct->intermolecular)
				work2[i][j%3] = w2->f(i,j);

			// Calculate vmin, the best energy for the entire
			// sequence
			if (j>(number)) {
				vmin = min(vmin,v.f(i,j)+v.f(j-(number),i));
			}
		}

		// Compute w5[i], the energy of the best folding from 1->i, and
		// w3[i], the energy of the best folding from i-->numofbases
		if (j==(number)) {
			for (i=0;i<=(MINLOOP+1);i++) {
				if (lfce[i])
					w5[i] = INFINITY;
				else if (i!=0)
					w5[i]=w5[i-1];//condition added 8/18/97
				else
					w5[i] = 0;//added 8/18/97
				// w5[i]=0;
			}
			for (i=MINLOOP+2;i<=(number);i++) {
				if (lfce[i])
					w5[i] = INFINITY;
				// else if (fce[i][i]==2) {
				//	// add the initiation
				//	w5[i] = w5[i-1] + data->init;
				// }	// for an intermolecular interaction
				else
					w5[i] = w5[i-1];
				// w5[i]=w5[i-1];
				for (k=1;k<=5;k++)
					e[k] = INFINITY; // e[k]=0;
				for (k=0;k<=(i-4);k++) {
					e[1] =
		min(e[1],(w5[k]+v.f(k+1,i)+penalty(i,k+1,ct,data)));

					e[2] =
		min(e[2],(w5[k]+erg4(i,k+2,k+1,2,ct,data,lfce[k+1])+
		v.f(k+2,i)+penalty(i,k+2,ct,data)));

					e[3] =
		min(e[3],(w5[k]+erg4(i-1,k+1,i,1,ct,data,lfce[i])+
		v.f(k+1,i-1)+penalty(i-1,k+1,ct,data)));

					e[4] =
		min(e[4],(w5[k]+erg4(i-1,k+2,k+1,2,ct,data,lfce[k+1])+
		erg4(i-1,k+2,i,1,ct,data,lfce[i]) + v.f(k+2,i-1)+
		penalty(i-1,k+2,ct,data)));

				}
				w5[i] = min(w5[i],e[1]);
				w5[i] = min(w5[i],e[2]);
				w5[i] = min(w5[i],e[3]);
				w5[i] = min(w5[i],e[4]);
			}
			w3[0] = 0;
			w3[number+1] = 0;
			for (i=(number);i>=(number-MINLOOP);i--){
				// number+1 ... number-MINLOOP
				if (lfce[i])
					w3[i] = INFINITY;
				else
					w3[i]=w3[i+1];
				// w3[i]=0;
			}
			for (i=((number)-MINLOOP-1);i>=1;i--) {
				if (lfce[i])
					w3[i] = INFINITY;
				// else if (fce[i][i]==2) {
				//	// add the initiation
				// 	// for an intermolecular interaction
				//	w3[i] = w3[i+1] + data->init;
         			// }
				else
					w3[i] = w3[i+1];
				// w3[i]=w3[i+1];

				for (k=1;k<=5;k++)
					e[k] = INFINITY;
				for (k=((number)+1);k>=(i+4);k--) {

					e[1] =
	min(e[1],(v.f(i,k-1)+w3[k]+penalty(k-1,i,ct,data)));

					e[2] =
	min(e[2],(v.f(i+1,k-1)+erg4(k-1,i+1,i,2,ct,data,lfce[i])+
	penalty(k-1,i+1,ct,data) + w3[k]));

       					e[3] =
	min(e[3],(v.f(i,k-2)+erg4(k-2,i,k-1,1,ct,data,lfce[k-1]) +
	penalty(k-2,i,ct,data) + w3[k]));

       					e[4] =
	min(e[4],(v.f(i+1,k-2)+erg4(k-2,i+1,i,2,ct,data,lfce[i]) +
	erg4(k-2,i+1,k-1,1,ct,data,lfce[k-1])+w3[k]+
	penalty(k-2,i+1,ct,data)));

				}
				w3[i] = min(w3[i],e[1]);
				w3[i] = min(w3[i],e[2]);
				w3[i] = min(w3[i],e[3]);
				w3[i] = min(w3[i],e[4]);
			}
		}

		// fill in some work array values:
		if (j>=(number)) {
			for (k=j+1;k>=((number)+1);k--)
				work[k][(j+1)%3] = w.f(k-(number),j+1-(number));

			if (ct->intermolecular)
				work2[k][(j+1)%3] =
					w2->f(k-(number),j+1-(number));
		}
	}

// for (i=((number)+1);i<=(2*(number)-1);i++) {
//	for (j=((number)+1);j<=(2*(number));j++) {
//		v.f(i,j)=v.f(i-(number),j-(number));
//		w.f(i,j)=w.f(i-(number),j-(number));
// 	}
// }

// cout << "presave" << "\n";

// debugging:
// if (ct->intermolecular) {
//	 ofstream dump("arrays.out");
//
//
//	output w and v along diagonals:
//	for (l=1;l<ct->numofbases;l++) {
// 		for (i=1;i+l<=2*ct->numofbases;i++) {
//			j = i+l;
//			dump << i << " " << j << " " << v.f(i,j) << " " <<
//				w.f(i,j) << " " << w2->f(i,j) << "\n";
//		}
//	}
//
//	 for (j=1;j<=(2*(number)-1);j++) {
//		for (i=min(j,number);i>=max(1,j-(number)+1);i--) {
//			dump << i << " " << j << " " << v.f(i,j) << " " <<
//				w.f(i,j) << " " << w2->f(i,j) << "\n";
//		}
// 	}
//	 dump.close();
// }

	if (save!=0) {
		ofstream sav(save,ios::binary);

		// sav << ct->ctlabel[1];
		sav.write((ct->ctlabel[1]),CTHEADERLENGTH);

		convert.ch[0] = 0;
		convert.ch[1] = 0;
		convert.ch[2] = 0;
		convert.ch[3] = 0;

		convert.i = ct->numofbases;
		sav.write(convert.ch,2);

		// sav << ct->numofbases << "\n";

		for (i=1;i<=ct->numofbases;i++) {
			convert.i = int (ct->numseq[i]);
			sav.write(convert.ch,2);
			sav.write(ct->nucs+i,1);
			convert.i = int (ct->hnumber[i]);
			sav.write(convert.ch,2);
			// sav << ct->numseq[i]<<"\n";
		}

		convert.i = ct->npair;
		sav.write(convert.ch,2);
		// sav << ct->npair << "\n";

		convert.i = ct->ndbl;
		sav.write(convert.ch,2);
		// sav << ct->ndbl << "\n";

		convert.i = ct->nnopair;
		sav.write(convert.ch,2);

		convert.i = ct->ngu;
		sav.write(convert.ch,2);

		//sav << ct->nnopair << "\n";

		for (i=1;i<=ct->npair;i++) {
			convert.i = ct->pair[i][0];
			sav.write(convert.ch,2);
			convert.i = ct->pair[i][1];
			sav.write(convert.ch,2);
			// sav << ct->pair[i][0]<<"\n";
			// sav << ct->pair[i][1]<<"\n";
		}

		for (i=1;i<=ct->ndbl;i++) {
			convert.i = ct->dbl[i];
			sav.write(convert.ch,2);
			// sav << ct->dbl[i] << "\n";
		}

		for (i=1;i<=ct->nnopair;i++) {
			convert.i = ct->nopair[i];
			sav.write(convert.ch,2);
			// sav << ct->nopair[i] << "\n";
		}

		for (i=0;i<ct->ngu;i++) {
			convert.i = ct->gu[i];
			sav.write(convert.ch,2);
		}

		if (ct->intermolecular) {
			// sav << 1 << "\n";
			convert.i = int (1);
			sav.write(convert.ch,1);
			for (i=0;i<3;i++) {
				convert.i = ct->inter[i];
				sav.write(convert.ch,2);
				// sav << ct->inter[i] << "\n";
			}
		}else{
			convert.i = int (0);
			sav.write(convert.ch,1);
			// sav << 0 << "\n";
		}

		for (i=0;i<=ct->numofbases;i++) {
			// sav <<w5[i] << "\n";
			if (w5[i] < 0) {
				convert.i = 0;
				sav.write(convert.ch,1);
				convert.i = -w5[i];
			} else {
				convert.i = 1;
				sav.write(convert.ch,1);
				convert.i = w5[i];
			}
			sav.write(convert.ch,2);
		}
		for (i=0;i<=ct->numofbases+1;i++) {
			// sav <<w3[i] << "\n";
			if (w3[i] < 0) {
				convert.i = 0;
				sav.write(convert.ch,1);
				convert.i = -w3[i];
			}else{
				convert.i = 1;
				sav.write(convert.ch,1);
				convert.i = w3[i];
			}
			sav.write(convert.ch,2);
		}
		for (i=0;i<=ct->numofbases;i++) {
			for (j=0;j<=ct->numofbases;j++) {
				// sav << v.dg[i][j] << "\n";
				// sav << w.dg[i][j] << "\n";
				if (v.dg[i][j] < 0) {
					convert.i = 0;
					sav.write(convert.ch,1);
					convert.i = -v.dg[i][j];
				}else{
					convert.i = 1;
					sav.write(convert.ch,1);
					convert.i = v.dg[i][j];
				}
				sav.write(convert.ch,2);

				if (w.dg[i][j] < 0) {
					convert.i = 0;
					sav.write(convert.ch,1);
					convert.i = -w.dg[i][j];
				}else{
					convert.i = 1;
					sav.write(convert.ch,1);
					convert.i = w.dg[i][j];
				}
				sav.write(convert.ch,2);

				if (ct->intermolecular) {
					if (w2->dg[i][j] < 0) {
						convert.i = 0;
						sav.write(convert.ch,1);
						convert.i = -w2->dg[i][j];
					}else{
						convert.i = 1;
						sav.write(convert.ch,1);
						convert.i = w2->dg[i][j];
					}
					sav.write(convert.ch,2);
				}
			}
		}

		// sav << vmin << "\n";
		if (vmin<0) {
			convert.i=0;
			sav.write(convert.ch,1);
			convert.i = -vmin;
		}else{
			convert.i = 1;
			sav.write(convert.ch,1);
			convert.i = vmin;
		}
		sav.write(convert.ch,2);

		// also save the thermodynamic parameters
		// convert.i = ct->pair[i][0];
		// sav.write(convert.ch,2);
		for (i=0;i<5;i++) {
			savefile(data->poppen[i],&sav);
		}
		savefile(data->maxpen,&sav);

		for (i=0;i<11;i++) {
			savefile(data->eparam[i],&sav);
		}
		for (i=0;i<6;i++) {
		    for (j=0;j<6;j++) {
			for (k=0;k<6;k++) {
			    for (l=0;l<3;l++) {
				savefile(data->dangle[i][j][k][l],&sav);
			    }
			}
		    }
		}
		for (i=0;i<31;i++) {
			savefile(data->inter[i],&sav);
			savefile(data->bulge[i],&sav);
			savefile(data->hairpin[i],&sav);
		}
		for (i=0;i<6;i++) {
		    for (j=0;j<6;j++) {
			for (k=0;k<6;k++) {
			    for (l=0;l<6;l++) {
l				savefile(data->stack[i][j][k][l],&sav);
				savefile(data->tstkh[i][j][k][l],&sav);
				savefile(data->tstki[i][j][k][l],&sav);
				savefile(data->coax[i][j][k][l],&sav);
				savefile(data->tstackcoax[i][j][k][l],&sav);
				savefile(data->coaxstack[i][j][k][l],&sav);
				savefile(data->tstack[i][j][k][l],&sav);
				savefile(data->tstkm[i][j][k][l],&sav);
			    }
			}
		    }
		}
		for (i=0;i<MAXTLOOP+1;i++) {
			for (j=0;j<2;j++) {
				savefile(data->tloop[i][j],&sav);
			}
		}
		savefile(data->numoftloops,&sav);

		for (i=0;i<6;i++) {
		    for (j=0;j<6;j++) {
			for (k=0;k<6;k++) {
			    for (l=0;l<6;l++) {
				for (m=0;m<6;m++) {
				    for (n=0;n<6;n++) {
			savefile(data->iloop11[i][j][k][l][m][n],&sav);

					for (o=0;o<6;o++) {
			savefile(data->iloop21[i][j][k][l][m][n][o],&sav);

					    for (p=0;p<6;p++) {
			savefile(data->iloop22[i][j][k][l][m][n][o][p],&sav);
					    }
					}
				    }
				}
			    }
			}
		    }
		}
		savefile(data->auend,&sav);
		savefile(data->gubonus,&sav);
		savefile(data->cint,&sav);
		savefile(data->cslope,&sav);
		savefile(data->c3,&sav);
		savefile(data->efn2a,&sav);
		savefile(data->efn2b,&sav);
		savefile(data->efn2c,&sav);
		savefile(data->numoftriloops,&sav);

		for (i=0;i<MAXTLOOP+1;i++) {
			for (j=0;j<2;j++) {
				savefile(data->triloop[i][j],&sav);
			}
		}
		convert.f = data->prelog;
		sav.write(convert.ch,4);
		savefile(data->init,&sav);

		sav.close();
	}

// cout << "pretrace\n";

	for (i=0;i<2*number+3;i++) {
		delete[] work[i];
		delete[] wmb[i];
	}

	delete[] wmb;
	delete[] work;

	if (ct->intermolecular) {
		for (i=0;i<2*number+3;i++) {
			delete[] work2[i];
			delete[] wmb2[i];
		}
		delete[] wmb2;
		delete[] work2;
	}

	traceback(ct, data, &v, &w, w2, w3, w5, fce, lfce,
		vmin, cntrl6, cntrl8, cntrl9);

	delete[] lfce;
	de_allocate (fce,2*number+1);

	delete[] w5;
	delete[] w3;

	if (ct->intermolecular)
		delete w2;

	return;
}

/*
 * void debug(char *debug,int i)
 * {
 *	char str[50];
 *	FILE *file;
 *
 *	file=fopen(debug,"w");
 *	itoa(i,str,10);
 *	fputs (str,file);
 *	fclose (file);
 * }
 *
 * void debug2(char *debug,char *text)
 * {
 *	char str[50];
 *	FILE *file;
 *
 *	file=fopen(debug,"w");
 *	fputs (text,file);
 *	fclose (file);
 * }
 */

//	Function opens data files to read thermodynamic data
int opendat (char *loop2,char *stackf,char *tstackh,char *tstacki,
	char *tloop,char *miscloop, char *danglef, char *int22, char *int21,
     	char *coax,char *tstackcoax,char *coaxstack,
     	char *tstack,char *tstackm, char *triloop, char *int11, datatable* data)
{
	char lineoftext[144],base[110];
	int count,i,j,k,l, m, a, b, c, d,e,f,g;
	float temp;
	FILE *check;

	//eparam[1] is a basepair bonus
	//eparam[2] is a bulge loop bonus
	//eparam[3] is an interior loop bonus

	ifstream ml1;
	ifstream lo1;
	ifstream st1;
	ifstream th1;
	ifstream ti1;
	ifstream tl1;
	ifstream da1;
	ifstream in1;
	ifstream in2;
	ifstream tri;
	ifstream co1;
	ifstream co2;
	ifstream co3;
	ifstream st2;
	ifstream tsm;
	ifstream i11;

	// ofstream out("check.out");


	//check that all the files exist with a C i/o function
	if ((check = fopen(miscloop, "r")) == NULL) {
		return 0;
	}
	fclose(check);

	if ((check = fopen(loop2, "r")) == NULL) {
		return 0;
	}
	fclose(check);

	if ((check = fopen(stackf, "r")) == NULL) {
		return 0;
	}
	fclose(check);

	if ((check = fopen(tstackh, "r")) == NULL) {
		return 0;
	}
	fclose(check);

	if ((check = fopen(tstacki, "r")) == NULL) {
		return 0;
	}
	fclose(check);

	if ((check = fopen(tloop, "r")) == NULL) {
		return 0;
	}
	fclose(check);

	if ((check = fopen(danglef, "r")) == NULL) {
		return 0;
	}
	fclose(check);

	if ((check = fopen(int22, "r")) == NULL) {
		return 0;
	}
	fclose(check);

	if ((check = fopen(int21, "r")) == NULL) {
		return 0;
	}
	fclose(check);

	if ((check = fopen(triloop, "r")) == NULL) {
		return 0;
	}
	fclose(check);

	if ((check = fopen(coax, "r")) == NULL) {
		return 0;
	}
	fclose(check);

	if ((check = fopen(tstackcoax, "r")) == NULL) {
		return 0;
	}
	fclose(check);

	if ((check = fopen(coaxstack, "r")) == NULL) {
		return 0;
	}
	fclose(check);

	if ((check = fopen(tstack, "r")) == NULL) {
		return 0;
	}
	fclose(check);

	if ((check = fopen(tstackm, "r")) == NULL) {
		return 0;
	}
	fclose(check);

	if ((check = fopen(int11,"r")) == NULL){
		return 0;
	}
	fclose(check);

	//open the files using the C++ method for reading
	ml1.open(miscloop);
	lo1.open(loop2);
	st1.open(stackf);
	th1.open(tstackh);
	ti1.open(tstacki);
	tl1.open(tloop);
	da1.open(danglef);
	in1.open(int22);
	in2.open(int21);
	tri.open(triloop);
	co1.open(coax);
	co2.open(tstackcoax);
	co3.open(coaxstack);
	st2.open(tstack);
	tsm.open(tstackm);
	i11.open(int11);

	// Read information from miscloop
	// the key sequence "-->" now indicates a record

	ml1 >> lineoftext;
	while(strcmp(lineoftext,"-->"))
		ml1 >> lineoftext;

	ml1 >> (data->prelog);
	data->prelog = (data->prelog)*100.0;


	ml1 >> lineoftext;
	while(strcmp(lineoftext,"-->"))
		ml1 >> lineoftext;

	ml1 >> temp;
	data->maxpen = int (temp*100.0 + .5);


	ml1 >> lineoftext;
	while(strcmp(lineoftext,"-->"))
		ml1>>lineoftext;

	// this reads float values, converts them int and assigns them into
	// array poppen
	for (count=1;count<= 4;count ++){
		ml1 >> temp;
		(data->poppen[count])= (int) (temp*100.0 + .5);
	}

	ml1 >> lineoftext;
	while(strcmp(lineoftext,"-->"))
		ml1 >> lineoftext;

	data->eparam[1] = 0; // assign some variables that are
	data->eparam[2] = 0; // "hard-wired" into code
	data->eparam[3] = 0;
	data->eparam[4] = 0;
	ml1 >> temp;
	data->eparam[5] = floor (temp*100.0+.5); // constant multi-loop penalty

	ml1 >> temp;
	data->eparam[6] = floor (temp*100.0+.5);

	data->eparam[7] = 30;
	data->eparam[8] = 30;
	data->eparam[9] = -500;
	ml1 >> temp;
	data->eparam[10] = floor (temp*100.0+.5);
	ml1 >> lineoftext;
	while(strcmp(lineoftext,"-->"))
		ml1 >> lineoftext;

	ml1 >> temp;
	if (ml1.peek()==EOF) {
		// these are old energy rules -- treat the other constants
		// properly
		data->efn2a = data->eparam[5];
		data->efn2b = data->eparam[6];
		data->efn2c = data->eparam[10];
		data->auend=0;
		data->gubonus=0;
		data->cslope = 0;
		data->cint=0;
		data->c3=0;
		data->init=0;
		data->gail = 0;
	}else{
		// constant multi-loop penalty for efn2
		data->efn2a = floor (temp*100.0+.5);
		ml1 >> temp;
		data->efn2b= floor(temp*100.0+.5);
		ml1 >> temp;
		data->efn2c=floor(temp*100.0+.5);

		// now read the terminal AU penalty:
		ml1 >> lineoftext;
		while(strcmp(lineoftext,"-->"))
			ml1 >> lineoftext;
		ml1 >> temp;
		data->auend = floor (temp*100.0+.5);

		// now read the GGG hairpin bonus:
		ml1 >> lineoftext;
		while(strcmp(lineoftext,"-->"))
			ml1 >> lineoftext;
		ml1 >> temp;
		data->gubonus = floor (temp*100.0+.5);

		// now read the poly c hairpin penalty slope:
		ml1 >> lineoftext;
		while(strcmp(lineoftext,"-->"))
			ml1 >> lineoftext;
		ml1 >> temp;
		data->cslope = floor (temp*100.0+.5);

		// now read the poly c hairpin penalty intercept:
		ml1 >> lineoftext;
		while(strcmp(lineoftext,"-->"))
			ml1 >> lineoftext;
		ml1 >> temp;
		data->cint = floor (temp*100.0+.5);

		// now read the poly c penalty for a loop of 3:
		ml1 >> lineoftext;
		while(strcmp(lineoftext,"-->"))
			ml1 >> lineoftext;
		ml1 >> temp;
		data->c3 = floor (temp*100.0+.5);

		ml1 >> lineoftext;
		while(strcmp(lineoftext,"-->"))
			ml1 >> lineoftext;
		ml1 >> temp;
		data->init = floor (temp*100.0+.5);

		// now read the GAIL rule indicator
		ml1 >> lineoftext;
		while(strcmp(lineoftext,"-->"))
			ml1 >> lineoftext;
		ml1>> temp;
		data->gail = floor (temp+.5);
	}


	// read info from dangle
	// add to dangle the case where X (represented as 0) is looked up
	for (l = 1;l <=2; l++){
	    for (i = 0;i <=5; i++){
		if ((i!=0)&&(i!=5))
		    for (count=1;count <=60;count++)
			da1 >> lineoftext;
		for (j=0;j<=5; j++) {
		    for (k=0;k<=5; k++) {
			if ((i==0)||(j==0)||(k==0)) {
			    data->dangle[i][j][k][l] = 0;
			} else if ((i==5)||(j==5)||(k==5)) {
			    data->dangle[i][j][k][l] = INFINITY;
			} else {
			    da1 >> lineoftext;

// cout << lineoftext << "\n";

			    if (strcmp(lineoftext,".")){
				data->dangle[i][j][k][l] =
				    floor (100.0*(atof(lineoftext))+.5);
			    }else
				data->dangle[i][j][k][l] = INFINITY;
			}
// cout << "dangle = " << i << " " << j << " " << k << " " << l << " " <<
//	data->dangle[i][j][k][l] << "\n";
		    }
// cin >> m;
		}
	    }
	}

	// read info from loop for internal, hairpin and bulge loops

	// get past text in file
	for (count = 1; count <=26; count++)
		lo1 >> lineoftext;

	for (i=1;i <= 30; i++) {
		lo1 >> lineoftext;//get past the size column in table
		lo1 >> lineoftext;
		if (strcmp(lineoftext,".")){
			data->inter[i] = floor (100.0*(atof(lineoftext))+.5);
		} else
			data->inter[i] = INFINITY;

// cout << "inter = " << data->inter[i] << "\n";

		lo1 >> lineoftext;
		if (strcmp(lineoftext,"."))
			data->bulge[i] = floor(100.0*(atof(lineoftext))+.5);
		else
			data->bulge[i] = INFINITY;

// cout << "bulge = " << data->bulge[i] << "\n";

		lo1 >> lineoftext;
		if (strcmp(lineoftext,".")){
			data->hairpin[i] = floor(100.0*(atof(lineoftext))+.5);
		}else
			data->hairpin[i] = INFINITY;

// cout << "hair = " << data->hairpin[i] << "\n";

	}

	// Read info from stack
	// add to the stack table the case where X (represented as 0) is
	// looked up:

	// get past text in file
	for (count=1;count<=42;count++)
		st1 >> lineoftext;
	for (i=0;i<=5;i++) {
	    if ((i!=0)&&(i!=5))
		for (count=1;count<=60;count++)
		    st1 >> lineoftext;
	    for (k=0;k<=5;k++) {
		for (j=0;j<=5;j++) {
		    for (l=0;l<=5;l++) {
			if ((i==0)||(j==0)||(k==0)||(l==0)) {
			    data->stack[i][j][k][l]=0;
			} else if((i==5)||(j==5)||(k==5)||(l==5)) {
			    data->stack[i][j][k][l] = INFINITY;
			} else {
			    st1 >> lineoftext;
			    if (strcmp(lineoftext,".")){
				data->stack[i][j][k][l] =
				    floor(100.0*(atof(lineoftext))+.5);
			    } else
				data->stack[i][j][k][l] = INFINITY;
			}
		    }
		}
	    }
	}

	// Read info from tstackh
	// add to the tstackh table the case where X (represented as 0) is
	// looked up:

	// get past text in file
	for (count=1;count<=46;count++)
		th1 >> lineoftext;
	for (i=0;i<=5;i++) {
	    if ((i!=0)&&(i!=5))
		for (count=1;count<=60;count++)
		    th1 >> lineoftext;
	    for (k=0;k<=5;k++) {
		for (j=0;j<=5;j++) {
		    for (l=0;l<=5;l++) {
			if ((i==0)||(j==0)||(k==0)||(l==0)) {
			    data->tstkh[i][j][k][l]=0;
			}else if ((i==5)||(j==5)||(k==5)||(l==5)) {
			    data->tstkh[i][j][k][l] = INFINITY;
			}else{
			    th1 >> lineoftext;
			    if (strcmp(lineoftext,".")){
				data->tstkh[i][j][k][l] =
				    floor(100.0*(atof(lineoftext))+.5);
			    }else
				data->tstkh[i][j][k][l] = INFINITY;
			}

// cout << "stack " << i << " " << j << " " << k << " " << l << "  " <<
//	data->tstkh[i][j][k][l]<<"\n";

		    }
// cin >> m;

		}
	    }
	}

	// Read info from tstacki
	// add to the tstacki table the case where X (represented as 0) is
	// looked up:

	// get past text in file
	for (count=1;count<=46;count++)
		ti1 >> lineoftext;

	for (i=0;i<=5;i++) {
	    if ((i!=0)&&(i!=5))
		for (count=1;count<=60;count++)
		    ti1 >> lineoftext;
	    for (k=0;k<=5;k++) {
		for (j=0;j<=5;j++) {
		    for (l=0;l<=5;l++) {
			if ((i==0)||(j==0)||(k==0)||(l==0)) {
			    data->tstki[i][j][k][l]=0;
			} else if ((i==5)||(j==5)||(k==5)||(l==5)) {
			    data->tstki[i][j][k][l] = INFINITY;
			} else {
			    // else if ((k==5)||(l==5)) {
		 	    //    include "5", linker for intermolecular for
			    //    case of flush ends
			    //	  data->tstki[i][j][k][l]=0;
			    // }
			    ti1 >> lineoftext;

// cout <<lineoftext<<"\n";

			    if (strcmp(lineoftext,".")){
				data->tstki[i][j][k][l] =
				    floor (100.0*(atof(lineoftext))+.5);
			    }else
				data->tstki[i][j][k][l] = INFINITY;
			}

// cout << "stack " << i << " " << j << " " << k << " " << l << "  " <<
//	data->tstki[i][j][k][l]<<"\n";

		    }

// cin >> m;

		}
	    }
	}

	// Read info from tloops
	/ /get past text in file
	for (count=1;count<=3;count++)
		tl1 >> lineoftext;

	data->numoftloops=0;
	tl1>>lineoftext;

	for (count=1;count<=MAXTLOOP&&!tl1.eof();count++){

// cout << lineoftext;

		(data->numoftloops)++;
		strcpy(base,lineoftext);
		strcpy(base+1,"\0");
		data->tloop[data->numoftloops][0] = tonumi(base);

// cout << base << "\n";
// cout << data->tloop[data->numoftloops][0] << "\n";

		strcpy(base,lineoftext+1);
		strcpy(base+1,"\0");
		data->tloop[data->numoftloops][0] =
			data->tloop[data->numoftloops][0]+ 5*tonumi(base);

// cout << base << "\n";
// cout << data->tloop[data->numoftloops][0] << "\n";

		strcpy(base,lineoftext+2);
		strcpy(base+1,"\0");
		data->tloop[data->numoftloops][0] =
			data->tloop[data->numoftloops][0]+ 25*tonumi(base);

// cout << base << "\n";
// cout << data->tloop[data->numoftloops][0] << "\n";

		strcpy(base,lineoftext+3);
		strcpy(base+1,"\0");
		data->tloop[data->numoftloops][0] =
			data->tloop[data->numoftloops][0]+ 125*tonumi(base);

// cout << base << "\n";
// cout << data->tloop[data->numoftloops][0] << "\n";

		strcpy(base,lineoftext+4);
		strcpy(base+1,"\0");
		data->tloop[data->numoftloops][0] =
			data->tloop[data->numoftloops][0]+ 625*tonumi(base);

// cout << base << "\n";
// cout << data->tloop[data->numoftloops][0] << "\n";

		strcpy(base,lineoftext+5);
		strcpy(base+1,"\0");
		data->tloop[data->numoftloops][0] =
			data->tloop[data->numoftloops][0]+ 3125*tonumi(base);

// cout << base << "\n";
// cout << data->tloop[data->numoftloops][0] << "\n";

		tl1 >> temp;
		data->tloop[data->numoftloops][1] = floor (100.0*temp+0.5);

// cout << "key = " << data->tloop[data->numoftloops][0] << "\n";
// cout << "bonus = " << data->tloop[data->numoftloops][1] << "\n";
// cin >> j;

		tl1 >> lineoftext;
	}

	// Read the 2x2 internal loops
	// key iloop22[a][b][c][d][j][l][k][m] =
	// a j l b
	// c k m d

	// get past text in file
	for (count=1;count<=340;count++)
		in1 >> lineoftext;

	for (i=1;i<=36;i++) {
		// read each of 21 tables
		// get past text in file
		for (j=1;j<=39;j++)
			in1 >> lineoftext;
		strcpy(base,lineoftext);
		strcpy(base+1, "\0");
		a = tonumi(base);
		for (j=1;j<=3;j++)
			in1 >> lineoftext;
		strcpy(base, lineoftext);
		strcpy(base+1, "\0");
		b = tonumi(base);
		in1 >> lineoftext;
		strcpy(base, lineoftext);
		strcpy(base+1, "\0");
		c = tonumi(base);
		for (j=1;j<=3;j++)
			in1 >> lineoftext;
		strcpy(base, lineoftext);
		strcpy(base+1, "\0");
		d = tonumi(base);
		// get past text in file
		for (j=1;j<=3;j++)
			in1 >> lineoftext;
		for (j=1;j<=4;j++) {
		    for (k=1;k<=4;k++) {
			for (l=1;l<=4;l++) {
			    for (m=1;m<=4;m++) {
				in1 >> temp;
				data->iloop22[a][b][c][d][j][l][k][m] =
				    floor(100.0*temp+0.5);

		// no longer need to store the reverse order at same time
		// because the tables contain redundancy:
		//data->iloop22[d][c][b][a][m][k][l][j] = floor(100.0*temp+0.5);

// cout << "a = " << a << " b= " << b << " c= " << c << " d = " << d << "\n";
// cout << "w = " << j << " x= " << l << " y= " << k << " z = " << m << "\n";
// cout << data->iloop22[a][b][c][d][j][l][k][m]<<"\n";

			    }
// cin >> foo;
			}
		    }
		}
	}

	// Read the 2x1 internal loop data
	// get past text at top of file
	for (i=1;i<=58;i++)
		in2 >> lineoftext;
	for (i=1;i<=6;i++) {
	    // read each row of tables
	    for (e=1;e<=4;e++) {
		// get past text in file
		for (j=1;j<=66;j++)
		    in2 >> lineoftext;
		in2 >> lineoftext;
		strcpy(base,lineoftext);
		strcpy(base+1,"\0");
		a = tonumi(base);
		// get past text in file
		for (j=1;j<=11;j++)
		    in2 >> lineoftext;
		in2 >> lineoftext;
		strcpy(base,lineoftext);
		strcpy(base+1,"\0");
		b = tonumi(base);
		// get past text in file
		for (j=1;j<=35;j++)
		    in2 >> lineoftext;
		for (c=1;c<=4;c++) {
		    for (j=1;j<=6;j++) {
			switch (j) {
			case 1:
			    f = 1;
			    g = 4;
			    break;
			case 2:
			    f = 2;
			    g = 3;
			    break;
			case 3:
			    f = 3;
			    g = 2;
			    break;
			case 4:
			    f = 4;
			    g = 1;
			    break;
			case 5:
			    f = 3;
			    g = 4;
			    break;
			case 6:
			    f = 4;
			    g = 3;
			    break;
			}
			for (d=1;d<=4;d++) {
			    in2 >> temp;
			    data->iloop21[a][b][c][d][e][f][g] =
				floor(100.0*temp+0.5);

//cout << a<<" "<<b<<" "<<c<<" "<<d<<" "<<e<<" "<<f<<" "<<g<<"\n";
//cout << temp<<"\n";
//cout << "100.0*temp = "<<100.0*temp<<"\n";
//cout << data->iloop21[a][b][c][d][e][f][g]<<"\n";
//cin >> temp;

			}
//cin >> temp;
		    }
		}
	    }
	}

	// Read info from triloops
	// get past text in file
	for (count=1;count<=3;count++)
		tri >> lineoftext;
	data->numoftriloops=0;
	tri >> lineoftext;

	for (count=1;count<=MAXTLOOP&&!tri.eof();count++){

// cout << lineoftext << "\n";

		(data->numoftriloops)++;
		strcpy(base,lineoftext);
		strcpy(base+1,"\0");
		data->triloop[data->numoftriloops][0] = tonumi(base);
		strcpy(base,lineoftext+1);
		strcpy(base+1,"\0");
		data->triloop[data->numoftriloops][0] =
			data->triloop[data->numoftriloops][0]+ 5*tonumi(base);
		strcpy(base,lineoftext+2);
		strcpy(base+1,"\0");
		data->triloop[data->numoftriloops][0] =
			data->triloop[data->numoftriloops][0]+ 25*tonumi(base);
		strcpy(base,lineoftext+3);
		strcpy(base+1,"\0");
		data->triloop[data->numoftriloops][0] =
			data->triloop[data->numoftriloops][0]+ 125*tonumi(base);
		strcpy(base,lineoftext+4);
		strcpy(base+1,"\0");
		data->triloop[data->numoftriloops][0] =
			data->triloop[data->numoftriloops][0]+ 625*tonumi(base);
		tri >> temp;
		data->triloop[data->numoftriloops][1] = floor (100.0*temp+0.5);

// cout << data->triloop[data->numoftriloops][1] << "  " <<
//	data->triloop[data->numoftriloops][0] << "\n";

		tri >> lineoftext;
	}

	// Read info from coax
	// add to the stack table the case where X (represented as 0) is
	// looked up:
	// data arrangement of coax: data->coax[a][b][c][d]
	// 5'bc3'
	// 3'ad

	// get past text in file
	for (count=1;count<=42;count++)
		co1 >> lineoftext;
	for (i=0;i<=5;i++) {
	    if ((i!=0)&&(i!=5))
		for (count=1;count<=60;count++)
		    co1 >> lineoftext;
	    for (k=0;k<=5;k++) {
		for (j=0;j<=5;j++) {
		    for (l=0;l<=5;l++) {
			if ((i==0)||(j==0)||(k==0)||(l==0)) {
				data->coax[j][i][k][l]=0;
			}else if ((i==5)||(j==5)||(k==5)||(l==5)) {
			    data->coax[j][i][k][l] = INFINITY;
			}else{
		 	    co1 >> lineoftext;

// cout << lineoftext << "end\n";

			    if (strcmp(lineoftext,".")){
				data->coax[j][i][k][l] =
				    floor(100.0*(atof(lineoftext))+.5);
			    }else
				data->coax[j][i][k][l] = INFINITY;

// cin >> a;
			}

// cout << j << " " << i << " " << k << " " << l << "  " <<
//	data->coax[j][i][k][l]<<"\n";

		    }
		}

// cin>>foo;

	    }
	}

	// Read info from tstackcoax
	// add to the tstackh table the case where X (represented as 0) is
	// looked up:

	// get past text in file
	for (count=1;count<=46;count++)
		co2 >> lineoftext;
	for (i=0;i<=5;i++) {
	    if (!(i==0||i==5))
		for (count=1;count<=60;count++)
		    co2 >> lineoftext;
	    for (k=0;k<=5;k++) {
		for (j=0;j<=5;j++) {
		    for (l=0;l<=5;l++) {
			if( (i==0)||(j==0)||(k==0)||(l==0)||
			    (i==5)||(j==5)||(k==5)||(l==5))
			{
			    data->tstackcoax[i][j][k][l]=0;
			}else{
			    co2 >> lineoftext;
			    if (strcmp(lineoftext,".")){
				data->tstackcoax[i][j][k][l] =
				    floor(100.0*(atof(lineoftext))+.5);
			    }else
				data->tstackcoax[i][j][k][l] = INFINITY;
			}
		    }
		}
	    }
	}

	// Read info from coaxstack
	// add to the tstackh table the case where X (represented as 0) is
	// looked up:

	// get past text in file
	for (count=1;count<=46;count++)
		co3 >> lineoftext;
	for (i=0;i<=4;i++) {
	    if (!(i==0||i==5))
		for (count=1;count<=60;count++)
		    co3 >> lineoftext;
	    for (k=0;k<=4;k++) {
		for (j=0;j<=4;j++) {
		    for (l=0;l<=4;l++) {
			if( (i==0)||(j==0)||(k==0)||(l==0)||
			    (i==5)||(j==5)||(k==5)||(l==5))
			{
			    data->coaxstack[i][j][k][l]=0;
			}else{
			    co3 >> lineoftext;
			    if (strcmp(lineoftext,".")){
				data->coaxstack[i][j][k][l] =
				    floor(100.0*(atof(lineoftext))+.5);
			    }else
				data->coaxstack[i][j][k][l] = INFINITY;
			}
		    }
		}
	    }
	}

	// Read info from tstack
	// this is the terminal mismatch data used in intermolecular folding
	// add to the tstack table the case where X (represented as 0) is
	// looked up.
	// also add the case where 5 (the intermolecular linker) is looked up,
	// this is actually a dangling end, not a terminal mismatch.

	// get past text in file
	for (count=1;count<=46;count++)
		st2 >> lineoftext;
	for (i=0;i<=5;i++) {
	    if ((i!=0)&&(i!=5))
		for (count=1;count<=60;count++)
		    st2 >> lineoftext;
	    for (k=0;k<=5;k++) {
		for (j=0;j<=5;j++) {
		    for (l=0;l<=5;l++) {
			if ((i==0)||(j==0)||(k==0)||(l==0)) {
			    data->tstack[i][j][k][l]=0;
			}else if ((i==5)||(j==5)) {
			    data->tstack[i][j][k][l] = INFINITY;
			}else if ((k==5)||(l==5)) {
			    // include "5", linker for intermolecular for
			    // case of flush ends
			    if ((k==5)&&(l==5)){
				// flush end
				data->tstack[i][j][k][l]=0;
			    }else if (k==5){
				// 5' dangling end
				// look up number for dangling end
				data->tstack[i][j][k][l] =
				    data->dangle[i][j][l][2]+penalty2(i,j,data);
			    }else if (l==5){
				//3' dangling end
				data->tstack[i][j][k][l] =
				    data->dangle[i][j][k][1]+penalty2(i,j,data);
			    }
			}else{
			    st2 >> lineoftext;
			    if (strcmp(lineoftext,".")){
				data->tstack[i][j][k][l] =
				    floor (100.0*(atof(lineoftext))+.5);
			    }else
				data->tstack[i][j][k][l] = INFINITY;
			}

// cout << "stack " << i << " " << j << " " << k << " " << l << "  " <<
//	data->tstki[i][j][k][l]<<"\n";

		    }

// cin >> m;

		}
	    }
	}

	// Read info from tstackm
	// add to the tstackm table the case where X (represented as 0) is
	// looked up:

	//get past text in file
	for (count=1;count<=46;count++)
		tsm >> lineoftext;
	for (i=0;i<=4;i++) {
	    if (i!=0)
		for (count=1;count<=60;count++)
		    tsm >> lineoftext;
	    for (k=0;k<=4;k++) {
		for (j=0;j<=4;j++) {
		    for (l=0;l<=4;l++) {
			if ((i==0)||(j==0)||(k==0)||(l==0)) {
			    data->tstkm[i][j][k][l]=0;
			}else{
			    tsm >> lineoftext;
			    if (strcmp(lineoftext,".")){
				data->tstkm[i][j][k][l] =
				    floor(100.0*(atof(lineoftext))+.5);
			    }else
				data->tstkm[i][j][k][l] = INFINITY;
			}

// cout << "stack " << i << " " << j << " " << k << " " << l << "  " <<
//	data->tstkh[i][j][k][l]<<"\n";

		    }

// cin >> m;

		}
	    }
	}

	// data arrangement for 1x1 loop tables iloop11[a][b][c][d][e][f]:
	// abc
	// def

	// Read the 1x1 internal loop data
	// encode the data like:  abc
	//                        def where b-e is a mismatch

	// get past text at top of file
	for (i=1;i<=58;i++)
		i11 >> lineoftext;
	for (i=1;i<=6;i++) {
		// read each row of table
		if (i==1) {
			a = 1;
			d = 4;
		} else if (i==2) {
			a = 2;
			d = 3;
		} else if (i==3) {
			a = 3;
			d = 2;
		} else if (i==4) {
			a = 4;
			d = 1;
		} else if (i==5) {
			a = 3;
			d = 4;
		} else {
			a = 4;
			d = 3;
		}

		// get past text
		for (j=1;j<=114;j++)
			i11 >> lineoftext;

		for (b=1;b<=4;b++) {
			for (j=1;j<=6;j++) {
				if (j==1) {
					c = 1;
					f = 4;
				} else if (j==2) {
					c = 2;
					f = 3;
				} else if (j==3) {
					c = 3;
					f = 2;
				} else if (j==4) {
					c = 4;
					f = 1;
				} else if (j==5) {
					c = 3;
					f = 4;
				} else {
					c = 4;
					f = 3;
				}
				for (e=1;e<=4;e++) {
					i11 >> temp;
					data->iloop11[a][b][c][d][e][f] =
						floor(100.0*temp+0.5);
				}
			}
		}
	}

	return 1;
}

// read a ct file with sequence and structural information
#define LINELENGTH 20

void openct(structure *ct,char *ctfile)
{
	int count,i,j;
	char base[2],header[CTHEADERLENGTH],line[LINELENGTH],temp[50];
	ifstream in;

	in.open(ctfile);
	in >> count;
	j = 0;

	if (count == -100) {
		// this is a CCT formatted file:
		in >> ct->numofbases;
		ct->allocate(ct->numofbases);
		in >> ct->numofstructures;
		// in >> ct->ctlabel[1];
		for (count=1;count<=ct->numofstructures;count++) {
			strcpy(ct->ctlabel[count],"\n");
		}
		for (i=1;i<=ct->numofbases;i++) {
			in >> ct->numseq[i];
			ct->nucs[i]=*tobase(ct->numseq[i]);
			ct->hnumber[i] = i;
		}
		for (count=1;count<=ct->numofstructures;count++) {
			for (i=1;i<=ct->numofbases;i++) {
				in >> ct->basepr[count][i];
			}
		}
		return;
	}else{
		// this is a ct file:
		// in >> ct->numofbases;
		ct->allocate(count);
		in.close();
		in.open(ctfile);
		for (ct->numofstructures = 1;
			((ct->numofstructures)<=(MAXSTRUCTURES))
			;(ct->numofstructures)++)
		{
			strcpy (header,"");
			if ((ct->numofstructures)==(MAXSTRUCTURES))
				errmsg (4,0);
			in >> ct->numofbases;
			strcpy(line,"");
			in.getline(header,CTHEADERLENGTH);

			// do {
			//	strcat(header,line);
			//	if (in.eof()) {
			//		ct->numofstructures--;
			//		return;
			//	}
			//	in >> line;
			//	strcat(header," ");
			// } while(strcmp(line,"1"));
			// in.putback(*line);*/

			if(in.eof()) {
				ct->numofstructures--;
				return;
			}

			strcpy((ct->ctlabel[ct->numofstructures]),header);
			for (count=1;count<=((ct->numofbases));count++)	{
				// if(in.eof()) {
				//	ct->numofstructures--;
				//	return;
				// }
				in >> temp; //ignore base number in ctfile
				in >> base; //read the base
				strcpy(base+1,"\0");
				ct->nucs[count]=base[0];
				tonum(base,ct,count);
				if (ct->numseq[count]==5) {
					ct->intermolecular = true;
					ct->inter[j] = count;
					j++;
				}
				in >> temp; //ignore numbering
				in >> temp; //ignore numbering
				// read base pairing info
				in >> ct->basepr[ct->numofstructures][count];
				// read historical numbering
				in >> ct->hnumber[count];
			}
		}
	}
	(ct->numofstructures)--;
	return;
}

void tonum(char *base,structure *ct,int count)
{

	if (!strcmp(base,"A"))
		(ct->numseq[count] = 1);
	else if(!strcmp(base,"B")) {
		(ct->numseq[count] = 1);
	} else if(!strcmp(base,"a")) {
		ct->numseq[count]=1;
		ct->nnopair++;
		ct->nopair[ct->nnopair] = count;
	} else if(!strcmp(base,"C"))
		(ct->numseq[count] = 2);
	else if(!strcmp(base,"Z")) {
		(ct->numseq[count] = 2);
	} else if(!strcmp(base,"c")) {
		ct->numseq[count] = 2;
		ct->nnopair++;
		ct->nopair[ct->nnopair] = count;
	} else if(!strcmp(base,"G"))
		(ct->numseq[count] = 3);
	else if(!strcmp(base,"H")) {
		(ct->numseq[count] = 3);
	} else if(!strcmp(base,"g")) {
		ct->numseq[count] = 3;
		ct->nnopair++;
		ct->nopair[ct->nnopair] = count;
	} else if(!strcmp(base,"U")||!strcmp(base,"T"))
		(ct->numseq[count] = 4);
	else if(!strcmp(base,"V")||!strcmp(base,"W")) {
		(ct->numseq[count] = 4);
	} else if(!strcmp(base,"u")||!strcmp(base,"t")) {
		ct->numseq[count] = 4;
		ct->nnopair++;
		ct->nopair[ct->nnopair] = count;
	} else if(!strcmp(base,"I")) {
		ct->numseq[count] = 5;
		ct->intermolecular= true;
	} else
		(ct->numseq[count]=0);  //this is for others, like X
	return;
}

int tonumi(char *base)
{
	int	a;

	if (!strcmp(base,"A")||!strcmp(base,"B"))
		(a = 1);
	else if(!strcmp(base,"C")||!strcmp(base,"Z"))
		(a = 2);
	else if(!strcmp(base,"G")||!strcmp(base,"H"))
		(a = 3);
	else if(!strcmp(base,"U")||!strcmp(base,"V"))
		(a = 4);
	else if(!strcmp(base,"T")||!strcmp(base,"W"))
		(a = 4);
	else
		(a=0);  //this is for others, like X
	return a;
}

void push(stackstruct *stack,int a,int b,int c,int d)
{
	(stack->sp)++;
	stack->stk[stack->sp][0]= a;
	stack->stk[stack->sp][1]= b;
	stack->stk[stack->sp][2]= c;
	stack->stk[stack->sp][3]= d;
}

void pull(stackstruct *stack,int *i,int *j,int *open,int *null,int *stz)
{
	if (stack->sp==0) {
		*stz = 1;
		return;
	}else{
		*stz = 0;
		*i = stack->stk[stack->sp][0];
		*j = stack->stk[stack->sp][1];
		*open= stack->stk[stack->sp][2];
		*null= stack->stk[stack->sp][3];
		stack->sp--;
	}
}

// calculate the energy of stacked base pairs
int erg1(int i,int j,int ip,int jp,structure *ct, datatable *data)
{
	int energy;

	if ((i==(ct->numofbases))||(j==((ct->numofbases)+1))) {
      		// this is not allowed because n and n+1 are not
		// covalently attached
		energy = INFINITY;
	} else {
		energy = data->stack[(ct->numseq[i])][(ct->numseq[j])]
			[(ct->numseq[ip])][(ct->numseq[jp])]+data->eparam[1];
	}
	return energy;
}

// calculate the energy of a bulge/internal loop
// where i is paired to j; ip is paired to jp; ip > i; j > jp
int erg2(int i,int j,int ip,int jp,structure *ct, datatable *data,
	int a, int b)
{
	//tlink,count,key,e[4]
	int energy,size,size1,size2,loginc, lopsid,energy2;
	/* size,size1,size2 = size of a loop
	 * energy = energy calculated
	 * loginc = the value of a log used in large hairpin loops
	 */

   	if (((i<=(ct->numofbases))&&(ip>(ct->numofbases)))||
		(( jp<=(ct->numofbases))&&(j>(ct->numofbases)))) {
		// A loop cannot contain the ends of the sequence
		energy=INFINITY;
		return energy;
	}

	size1 = ip-i-1;
	size2 = j - jp - 1;

	if ((a!=0)||(b!=0)) {
		if ((a==1)||(b==1)){
			// the loop contains a nuc that
			// should be double stranded
			return INFINITY;
		}else if ((a==5)) {
			// the loop is actually between two strands
			// (ie: intermolecular)

			if (size2>1) {
				// free energy is that of two terminal
				// mismatches and the intermolecular initiation
	energy = data->init +
		data->tstack[ct->numseq[i]][ct->numseq[j]]
		[ct->numseq[i+1]][ct->numseq[j-1]] +
		data->tstack[ct->numseq[jp]][ct->numseq[ip]]
		[ct->numseq[jp+1]][ct->numseq[ip-1]];

		       } else if (size2==1) {
				// find the best terminal mismatch and terminal
				//stack free energies combination

	energy = data->init +
		data->tstack[ct->numseq[i]][ct->numseq[j]]
		[ct->numseq[i+1]][ct->numseq[j-1]] +
		erg4 (jp,ip,ip-1,2,ct,data,false)+penalty(jp,ip,ct,data);
	energy2 = data->init + data->tstack[ct->numseq[jp]][ct->numseq[ip]]
		[ct->numseq[jp+1]][ct->numseq[ip-1]] +
		erg4 (i,j,i+1,1,ct,data,false)+penalty(i,j,ct,data);

				energy = min (energy,energy2);
			  //if ((ct->numseq[i+1]!=5)&&(ct->numseq[ip-1]!=5)) {
			     //now consider if coaxial stacking is better:
			     energy2 = data->init + data->tstackcoax[ct->numseq[jp]]
				[ct->numseq[ip]][ct->numseq[jp+1]][ct->numseq[ip-1]]
				+ data->coaxstack[ct->numseq[jp+1]][ct->numseq[ip-1]]
				[ct->numseq[j]][ct->numseq[i]]+penalty(i,j,ct,data)+penalty(jp,ip,ct,data);
			     energy = min(energy,energy2);
			     energy2 = data->init + data->tstackcoax[ct->numseq[jp]]
				[ct->numseq[ip]][ct->numseq[j-1]][ct->numseq[ip-1]]
				+ data->coaxstack[ct->numseq[j-1]][ct->numseq[ip-1]]
				[ct->numseq[j]][ct->numseq[i]]+penalty(i,j,ct,data)+penalty(jp,ip,ct,data);
			     energy = min(energy,energy2);
			  //}
		       }
		       else if (size2==0) {//just have dangling ends or flush stacking
			energy = data->init + erg4 (jp,ip,ip-1,2,ct,data,false) +
				erg4 (i,j,i+1,1,ct,data,false)+penalty(i,j,ct,data)+penalty(jp,ip,ct,data);
			  energy2 = data->init + data->coax[ct->numseq[ip]][ct->numseq[jp]]
				[ct->numseq[j]][ct->numseq[i]]+penalty(i,j,ct,data)+penalty(jp,ip,ct,data);
			  energy = min(energy,energy2);
		       }


				return energy;
		} else if (b==5) {
			// the loop is actually between two strands
			// (ie: intermolecular)

			if (size1>1) {
				//free energy is that of two terminal mismatches
				//and the intermolecular initiation
			  energy = data->init + data->tstack[ct->numseq[i]][ct->numseq[j]]
				[ct->numseq[i+1]][ct->numseq[j-1]] +
			     data->tstack[ct->numseq[jp]][ct->numseq[ip]]
				[ct->numseq[jp+1]][ct->numseq[ip-1]];
			}else if (size1==1) {
				//find the best terminal mismatch and terminal
				//stack free energies combination

			  energy = data->init + data->tstack[ct->numseq[i]][ct->numseq[j]]
					[ct->numseq[i+1]][ct->numseq[j-1]] +
				erg4 (ip,jp,jp+1,1,ct,data,false)+penalty(ip,jp,ct,data);
			  energy2 = data->init + data->tstack[ct->numseq[jp]][ct->numseq[ip]]
					[ct->numseq[jp+1]][ct->numseq[ip-1]] +
				erg4 (i,j,j-1,2,ct,data,false)+penalty(i,j,ct,data);

			  energy = min (energy,energy2);
			  //if ((ct->numseq[i+1]!=5)&&(ct->numseq[ip-1]!=5)) {
			     //now consider if coaxial stacking is better:
			     energy2 = data->init + data->tstackcoax[ct->numseq[i]]
				[ct->numseq[j]][ct->numseq[i+1]][ct->numseq[j-1]]
				+ data->coaxstack[ct->numseq[i+1]][ct->numseq[j-1]]
				[ct->numseq[ip]][ct->numseq[jp]]+penalty(i,j,ct,data)+penalty(jp,ip,ct,data);
			     energy = min(energy,energy2);
			     energy2 = data->init + data->tstackcoax[ct->numseq[i]]
				[ct->numseq[j]][ct->numseq[ip-1]][ct->numseq[j-1]]
				+ data->coaxstack[ct->numseq[ip-1]][ct->numseq[j-1]]
				[ct->numseq[ip]][ct->numseq[jp]]+penalty(i,j,ct,data)+penalty(jp,ip,ct,data);
			     energy = min(energy,energy2);
			  //}
			}else if (size1==0) {
				//just have dangling ends or flush stacking
			energy = data->init + erg4 (jp,ip,jp+1,1,ct,data,false) +
				erg4 (i,j,j-1,2,ct,data,false)+penalty(i,j,ct,data)+penalty(jp,ip,ct,data);
			  energy2 = data->init + data->coax[ct->numseq[j]][ct->numseq[i]]
				[ct->numseq[ip]][ct->numseq[j]]+penalty(i,j,ct,data)+penalty(jp,ip,ct,data);
			  energy = min(energy,energy2);
			}
			return energy;
		}
	}

	// a typical internal or bulge loop:
	size1 = ip-i-1;
	size2 = j - jp - 1;
	if (size1==0||size2==0) {
		//bulge loop
		size = size1+size2;
		if (size==1) {
			energy =
				 data->stack[ct->numseq[i]][ct->numseq[j]]
				[ct->numseq[ip]][ct->numseq[jp]]
				+ data->bulge[size] + data->eparam[2];
		} else if (size>30) {
			loginc = int((data->prelog)*log(double ((size)/30.0)));
			energy = data->bulge[30] + loginc + data->eparam[2];
			energy = energy +
				penalty(i,j,ct,data) + penalty(jp,ip,ct,data);
		} else {
			energy = data->bulge[size] + data->eparam[2];
			energy = energy + penalty(i,j,ct,data) +
				penalty(jp,ip,ct,data);
		}
	}else{
		//internal loop
		size = size1 + size2;
		lopsid = abs(size1-size2);
		if (size>30) {
			loginc = int((data->prelog)*log((double((size))/30.0)));
			if ((size1==1||size2==1)&&data->gail) {

				energy =
	data->tstki[ct->numseq[i]][ct->numseq[j]][1][1] +
	data->tstki[ct->numseq[jp]][ct->numseq[ip]][1][1] +
	data->inter[30] + loginc + data->eparam[3] +
	min(data->maxpen,(lopsid*data->poppen[min(2,min(size1,size2))]));

			}else{

				energy =
data->tstki[ct->numseq[i]][ct->numseq[j]][ct->numseq[i+1]][ct->numseq[j-1]] +
data->tstki[ct->numseq[jp]][ct->numseq[ip]][ct->numseq[jp+1]][ct->numseq[ip-1]]+
data->inter[30] + loginc + data->eparam[3] +
min(data->maxpen,(lopsid* data->poppen[min(2,min(size1,size2))]));

			}
		} else if ((size1==2)&&(size2==2)){
			//2x2 internal loop
			energy =
				data->iloop22[ct->numseq[i]][ct->numseq[ip]]
				[ct->numseq[j]][ct->numseq[jp]]
				[ct->numseq[i+1]][ct->numseq[i+2]]
				[ct->numseq[j-1]][ct->numseq[j-2]];


		}else if ((size1==1)&&(size2==2)) {
			//2x1 internal loop
			energy =
				data->iloop21[ct->numseq[i]][ct->numseq[j]]
				[ct->numseq[i+1]][ct->numseq[j-1]]
				[ct->numseq[jp+1]][ct->numseq[ip]]
				[ct->numseq[jp]];

		} else if ((size1==2)&&(size2==1)) {
			// 1x2 internal loop
			energy =
				data->iloop21[ct->numseq[jp]]
				[ct->numseq[ip]][ct->numseq[jp+1]]
				[ct->numseq[ip-1]][ct->numseq[i+1]]
				[ct->numseq[j]][ct->numseq[i]];

// cout << "2x1 internal loop energy = " << energy << "\n";

		}else if (size==2){
			// a single mismatch
			// energy = data->stack[ct->numseq[i]][ct->numseq[j]]
			//	[ct->numseq[i+1]][ct->numseq[j-1]] +
			//	data->stack[ct->numseq[jp]][ct->numseq[ip]]
			//	[ct->numseq[jp+1]][ct->numseq[ip-1]];
			energy =
	data->iloop11[ct->numseq[i]][ct->numseq[i+1]][ct->numseq[ip]]
		[ct->numseq[j]][ct->numseq[j-1]][ct->numseq[jp]];

        	}else if ((size1==1||size2==1)&&data->gail) {
			// this loop is lopsided
			// note, we treat this case as if we had a loop
			// composed of all As
			// if and only if the gail rule is set to 1 in
			// miscloop.dat

			energy =
		data->tstki[ct->numseq[i]][ct->numseq[j]][1][1] +
			data->tstki[ct->numseq[jp]][ct->numseq[ip]][1][1] +
			data->inter[size] + data->eparam[3] +
			min(data->maxpen,(lopsid*
			data->poppen[min(2,min(size1,size2))]));

         	}else{
         /*
	  * else if (size2==1) {
	  *	// this loop is lopsided - one side has a terminal mismatch
	  *	// and a dangle - the other side just a terminal mismatch
	  *
	  *
	  *	energy =
data->stack[ct->numseq[i]][ct->numseq[j]] [ct->numseq[i+1]][ct->numseq[j-1]] +
data->dangle[ct->numseq[jp]][ct->numseq[ip]][ct->numseq[ip-1]][2] +
penalty(ip,jp,ct,data);
		energy = min(energy,
data->stack[ct->numseq[jp]][ct->numseq[ip]][ct->numseq[jp+1]][ct->numseq[ip-1]]+
data->dangle[ct->numseq[i]] [ct->numseq[j]][ct->numseq[i+1]][1]) +
penalty(i,j,ct,data);
		energy =
	energy + data->inter[size] + data->eparam[3] +
	min(data->maxpen,(lopsid* data->poppen[min(2,min(size1,size2))]));
	  *
	  * }
	  */
			// debug:
			// if (i==54&&j==96&&ip==57&&jp==89){
			//	data->poppen[1] = data->poppen[2];
			// }

         		energy =
data->tstki[ct->numseq[i]][ct->numseq[j]][ct->numseq[i+1]][ct->numseq[j-1]] +
data->tstki[ct->numseq[jp]][ct->numseq[ip]][ct->numseq[jp+1]][ct->numseq[ip-1]]+
data->inter[size] + data->eparam[3] +
min(data->maxpen,(lopsid*data->poppen[min(2,min(size1,size2))]));

		}
	}
//
//      cout << "erg2: " << energy << "\n";
//	cout << "i: " << i << "\n";
//      cout << "j: " << j << "\n";
//      cout << "ip: "<< ip << "\n";
//      cout << "jp: "<< jp << "\n";

		return energy;
}

// calculate the energy of a hairpin loop:
int erg3(int i,int j,structure *ct, datatable *data,int dbl)
{
	int energy,size,loginc,tlink,count,key,k;
	/* size,size1,size2 = size of a loop
	 * energy = energy calculated
	 * loginc = the value of a log used in large hairpin loops
	 */

	// the loop contains a base that should be double stranded
	if (dbl==1)
		return INFINITY;
	else if (dbl==5) {
		// intermolecular interaction
		// intermolecular "hairpin" free energy is that of
		// intermolecular initiation plus the stacked mismatch
		uenergy = data->init +
			data->tstack[ct->numseq[i]][ct->numseq[j]]
	         	[ct->numseq[i+1]][ct->numseq[j-1]];
		return energy;
	}

	// Hairpin cannot contain the ends of the sequence
   	if ((i<=(ct->numofbases))&&(j>(ct->numofbases))) {
		energy = INFINITY;
		return energy;
	}

	size = j-i-1;
	if (size>30) {

// cout << "erg3:  i = " << i << "   j = " << j <<
//	"   " << log(double ((size)/30.0)) << "\n";

		loginc = int((data->prelog)*log((double ((size))/30.0)));
		energy = data->tstkh[ct->numseq[i]][ct->numseq[j]]
			[ct->numseq[i+1]][ct->numseq[j-1]]
			+ data->hairpin[30]+loginc+data->eparam[4];
	} else if (size<3) {
		energy = data->hairpin[size] + data->eparam[4];
		if (ct->numseq[i]==4||ct->numseq[j]==4)
			energy = energy+6;
	} else if (size==4) {
		tlink = 0;
		key = (ct->numseq[j])*3125 + (ct->numseq[i+4])*625 +
			(ct->numseq[i+3])*125 +
			(ct->numseq[i+2])*25+(ct->numseq[i+1])*5+
			(ct->numseq[i]);
		for (count=1;
			count<=data->numoftloops&&tlink==0;count++)
		{
			if (key==data->tloop[count][0])
				tlink = data->tloop[count][1];
		}
		energy = data->tstkh[ct->numseq[i]][ct->numseq[j]]
			[ct->numseq[i+1]][ct->numseq[j-1]]
			+ data->hairpin[size] + data->eparam[4] + tlink;
	}else if (size==3){
		tlink = 0;
		key = (ct->numseq[j])*625 +
			(ct->numseq[i+3])*125 + 
			ct->numseq[i+2])*25+(ct->numseq[i+1])*5+(ct->numseq[i]);
			for (count=1;
				ount<=data->numoftriloops&&tlink==0;count++)
			{
				if (key==data->triloop[count][0])
					tlink = data->triloop[count][1];
			}
			energy = data->tstkh[ct->numseq[i]][ct->numseq[j]]
				[ct->numseq[i+1]][ct->numseq[j-1]];
			energy = data->hairpin[size] + data->eparam[4] + tlink
         			+penalty(i,j,ct,data);
	}else{
		energy = data->tstkh[ct->numseq[i]][ct->numseq[j]]
			[ct->numseq[i+1]][ct->numseq[j-1]]
			+ data->hairpin[size] + data->eparam[4];
	}

//      cout << "erg3: "<< energy<<"\n";
//      cout << "i: " <<i<<"\n";
//      cout << "j: " <<j<<"\n";

	//check for GU closeure preceded by GG
	if (ct->numseq[i]==3&&ct->numseq[j]==4) {
		if ((i>2&&i<ct->numofbases)||(i>ct->numofbases+2)){
			if (ct->numseq[i-1]==3&&ct->numseq[i-2]==3) {

				energy = energy + data->gubonus;
				//if (ct->numseq[i+1]==4&&ct->numseq[j-1]==4)
				//	energy = energy - data->uubonus;
				//if (ct->numseq[i+1]==3&&ct->numseq[j-1]==1)
				//	energy = energy - data->uubonus;
			}
		}
	}

    	//check for a poly-c loop
	tlink = 1;
	for (k=1;(k<=size)&&(tlink==1);k++) {
		if (ct->numseq[i+k] != 2) tlink = 0;
	}
	if (tlink==1) {
		//this is a poly c loop so penalize
	       	if (size==3)
			energy = energy + data->c3;
		else
			energy = energy + data->cint + size*data->cslope;
	}

	return energy;
}

int erg4(int i,int j,int ip,int jp,structure *ct, datatable *data, bool lfce)
{
	int energy;

	// dangling base
	// jp = 1 => 3' dangle
	// jp = 2 => 5' dangle

	// stacked nuc should be double stranded
	if (lfce)
		return INFINITY;

	// dangling nuc is an intermolecular linker
	if (ip==5)
		return 0;

	energy = data->dangle[ct->numseq[i]][ct->numseq[j]][ct->numseq[ip]][jp];
	return energy;
}

//this function calculates whether a terminal pair i,j requires the end penalty
int penalty(int i,int j,structure* ct, datatable *data)
{

	/*
	 * if ((ct->numseq[i]==1)||(ct->numseq[j]==1)) {
   	 *	// this is an A-U pair
	 *	return data->auend;
	 * }else if ((ct->numseq[i]==4)&&(ct->numseq[j]==3)) {
	 * 	// this is a G-U pair with 3' terminal U
	 *	return data->auend;
	 * } else if ((ct->numseq[i]==4)&&(ct->numseq[j]==3)) {
	 *	// this is a G-U pair with 3' terminal G
	 *	return data->auend;
	 * }
	 */

	if (ct->numseq[i]==4||ct->numseq[j]==4)
   		return data->auend;
	else
		return 0;
}

//decide if a terminal pair i,j requires the end penalty
int penalty2(int i,int j, datatable *data)
{

	if (i==4||j==4)
   		return data->auend;
	else
		return 0;
}

// Energyout: writes to file a list of energys calculated by efn2
void energyout(structure *ct,char *energyfile)
{
	int i;
	ofstream out(energyfile);

	for (i=1;i<=ct->numofstructures;i++)
		out << "Structure: " << i << "   Energy = " <<
			(float (ct->energy[i])/100.0) << "   \n";
}

#define	SEQLINE		300	// maximum line length in .seq file
int openseq (structure *ct,char *seqfile)
{
	char temp[SEQLINE],seq[SEQLINE],base[SEQLINE],test[SEQLINE];
	int i,j,length,nucs;
	FILE *se;

	ct->nnopair = 0;
	// nucs = 0;


	// read the sequence file to get the number of nucleotides
	se=fopen(seqfile,"r");

	do {
		fgets(temp,SEQLINE,se);
		strncpy(test,temp,1);
		strcpy(test+1,"\0");
	} while (!strcmp(test,";"));

	// fgets(ct->ctlabel[1],SEQLINE,se);
	strcpy(ct->ctlabel[1],temp);

	nucs = 1;
	while (1) {
		fgets(seq,SEQLINE,se);
		length = strlen (seq);
		// up to length-1 bcs of /n character
		for (j=0;j<length;j++) {
			strncpy (base,seq+j,1);
			strcpy (base+1,"\0");
			if (!strcmp(base,"1"))
				break;
			if (!(	!strcmp(base,"A") || !strcmp(base,"a")  ||
				!strcmp(base,"C") || !strcmp(base,"c")  ||
				!strcmp(base,"G") || !strcmp(base,"g")  ||
				!strcmp(base,"T") || !strcmp(base,"t")  ||
				!strcmp(base,"U") || !strcmp(base,"u")  ||
				!strcmp(base,"X") || !strcmp(base,"x")  ||
				!strcmp(base," ") || !strcmp(base,"\n") ||
				!strcmp(base,"N")))
			{
				return 0;
	
			}
			// tonum(base,ct,(i));
			if (strcmp(base," ") && strcmp(base,"\n"))
				nucs++;
		}
		if (!strcmp(base,"1")) break;
	}

	ct->numofbases = nucs - 1;
	nucs--;
	fclose (se);

	if (nucs==0)
		return 0;

	ct->allocate(nucs);

	// now read the file
	se=fopen(seqfile,"r");

	do {
		fgets(temp,SEQLINE,se);
		strncpy(test,temp,1);
		strcpy(test+1,"\0");
	} while (!strcmp(test,";"));

	// fgets(ct->ctlabel[1],SEQLINE,se);
	strcpy(ct->ctlabel[1],temp);

	i = 1;
	while (1) {
		fgets(seq,SEQLINE,se);
		length = strlen (seq);
		for (j=0;j<length;j++) {
			strncpy (base,seq+j,1);
			strcpy (base+1,"\0");
			if (!strcmp(base,"1"))
			break;
	      		if (!(	!strcmp(base,"A") || !strcmp(base,"a")  ||
				!strcmp(base,"C") || !strcmp(base,"c")  ||
				!strcmp(base,"G") || !strcmp(base,"g")  ||
				!strcmp(base,"T") || !strcmp(base,"t")  ||
				!strcmp(base,"U") || !strcmp(base,"u")  ||
				!strcmp(base,"X") || !strcmp(base,"x")  ||
				!strcmp(base," ") || !strcmp(base,"\n") ||
				!strcmp(base,"N")))
			{
				return 0;
			}
			tonum(base,ct,(i));
			ct->nucs[i]=base[0];
			ct->hnumber[i] = i;
			if (strcmp(base," ")&&strcmp(base,"\n"))
				i++;
		}
		if (!strcmp(base,"1")) break;
	}
	// ct->numofbases = i - 1;

	fclose (se);
	return 1;
}

// output a ct file
void ctout (structure *ct,char *ctoutfile)
{
	int count,i;
	char line[2*CTHEADERLENGTH],number[2*NUMLEN];//base[2]
	FILE *ctfile;

	ctfile=fopen(ctoutfile,"w");

	for (count=1;count<=(ct->numofstructures);count++) {
		strcpy(line,"");
		sprintf(line,"%5i",ct->numofbases);

		// itoa(ct->numofbases,number,10);
		// strcat(line,number);

   
		//this corrects a difference on the sgi computers
		sgifix 

		if (ct->energy[count]!=0) {
			strcat(line,"  dG = ");
			gcvt((float (ct->energy[count]))/100.0,8,number);
			strcat(line,number);
			strcat(line,"  ");
		} else
			strcat(line,"  ");
			strcat(line,ct->ctlabel[count]);
			fputs (line,ctfile);
			for (i=1;i<ct->numofbases;i++) {
	/*
	 * if (ct->numseq[i]==1)
	 *	sprintf(line,"%5i A%8i%5i%5i%5i\n",
	 *		i,(i-1),(i+1),ct->basepr[count][i],ct->hnumber[i]);
	 * else if (ct->numseq[i]==2)
	 *	sprintf(line,"%5i C%8i%5i%5i%5i\n",
	 *		i,(i-1),(i+1),ct->basepr[count][i],ct->hnumber[i]);
	 * else if (ct->numseq[i]==3)
	 *	sprintf(line,"%5i G%8i%5i%5i%5i\n",
	 *		i,(i-1),(i+1),ct->basepr[count][i],ct->hnumber[i]);
 	 * else if (ct->numseq[i]==4)
	 *	sprintf(line,"%5i U%8i%5i%5i%5i\n",
	 *		i,(i-1),(i+1),ct->basepr[count][i],ct->hnumber[i]);
	 * else if (ct->numseq[i]==0)
	 *	sprintf(line,"%5i X%8i%5i%5i%5i\n",
	 *		i,(i-1),(i+1),ct->basepr[count][i],ct->hnumber[i]);
	 * else if (ct->numseq[i]==5)
	 *	sprintf(line,"%5i I%8i%5i%5i%5i\n",
	 *		i,(i-1),(i+1),ct->basepr[count][i],ct->hnumber[i]);
	 */
				sprintf(line,"%5i%2c%8i%5i%5i%5i\n",
					i,ct->nucs[i],(i-1),(i+1),
					t->basepr[count][i],ct->hnumber[i]);
				fputs(line,ctfile);
			}

		i = ct->numofbases;
/*
 *	if (ct->numseq[i]==1)
 *  		sprintf(line,"%5i A%8i%5i%5i%5i\n",
 *  			i,(i-1),0,ct->basepr[count][i],ct->hnumber[i]);
 *	else if (ct->numseq[i]==2)
 *		sprintf(line,"%5i C%8i%5i%5i%5i\n",
 *			i,(i-1),0,ct->basepr[count][i],ct->hnumber[i]);
 *	else if (ct->numseq[i]==3)
 *		sprintf(line,"%5i G%8i%5i%5i%5i\n",
 *			i,(i-1),0,ct->basepr[count][i],ct->hnumber[i]);
 *	else if (ct->numseq[i]==4)
 *		sprintf(line,"%5i U%8i%5i%5i%5i\n",
 *			i,(i-1),0,ct->basepr[count][i],ct->hnumber[i]);
 *	else if (ct->numseq[i]==0)
 *		sprintf(line,"%5i X%8i%5i%5i%5i\n",
 *			i,(i-1),0,ct->basepr[count][i],ct->hnumber[i]);
 *	else if (ct->numseq[i]==5)
 *		sprintf(line,"%5i I%8i%5i%5i%5i\n",
 *			i,(i-1),0,ct->basepr[count][i],ct->hnumber[i]);
 */
		sprintf(line,"%5i %1c%8i%5i%5i%5i\n",
			i,ct->nucs[i],(i-1),0,
			t->basepr[count][i],ct->hnumber[i]);
		fputs(line,ctfile);
	}

	fclose (ctfile);
	return;
}

inline void swap(int *a,int *b)
{
	int temp;

	temp = *a;
	*a = *b;
	*b = temp;

	return;
}


// output a CT file to a structure form; Writes to file *file
void linout(structure *ct,char *file)
{

	char dash[2],bl[2],number[NUMLEN];
	int counter,xx,ip,jp,half,k,j,i,ll,kk,jj,l,stz,countr,ii,pos,p;
	stackstruct stack;
	arraystruct table;

	strcpy (dash,"-");
	strcpy (bl," ");

	// Open output file
	ofstream out(file);

	for (counter = 1;counter<=ct->numofstructures;counter++) {

		out << "Structure# " << counter << "\n";
		out << ct->ctlabel[counter] << "\n";
		gcvt((float (ct->energy[counter]))/100.0,8,number);
		if (ct->energy[counter]!=0)
			out << "energy = " << number << "\n";
	
		stz = 0;
		i = 1;
		j = ct->numofbases;
		countr = 0;
		xx = 0;
		stack.sp = 0;

		// empty the array
		for (k=1;k<=AMAX-2;k++) {
			for (p=1;p<=6;p++) {
				strcpy(table.array[p][k],bl);
			}
		}

		loop: ;
		// look for dangling ends
		ip = i;
		jp = j;
		while (ct->basepr[counter][ip]==0) {
			ip++;
			if (ip>=j) {

				if (i>=j)
					goto output;

				// this is a hairpin loop
				half = ((j-i+2)/(2));
				ii = i - 1;
				jj = j + 1;
				for (k=1;k<=half;k++) {
					ii++;
					jj--;
					countr++;
					if ((ii%10)==0)
						digit(1,ii,countr,& table);
					if ((jj%10)==0)
						digit(6,jj,countr,& table);
					if (k!=half) {
						strcpy(table.array[2][countr],
							obase(ct->numseq[ii]));
						strcpy(table.array[5][countr],
							obase(ct->numseq[jj]));
					}else{
						if (ii<jj) {
							strcpy(
							table.array[3][countr],
							obase(ct->numseq[ii]));
						}
						strcpy(table.array[4][countr],
							obase(ct->numseq[jj]));
					}
				}
				goto output;
			}
		}

		while (ct->basepr[counter][jp] ==0)
			jp--;

		k = ip - i;
		if ((j-jp)>k)
			k = j-jp;
		if (k!=0) {
			ii = ip;
			jj = jp;
			pos = countr + k + 1;
			for (kk=1;kk<=k;kk++) {
				pos--;
				ii--;
				jj++;
				if (ii>=i) {
					strcpy(table.array[2][pos],
						obase(ct->numseq[ii]));
					if ((ii%10)==0)
						digit(1,ii,pos,& table);
				}else
					strcpy(table.array[2][pos],dash);
				if (jj<=j) {
					strcpy(table.array[5][pos],
						obase(ct->numseq[jj]));
					if ((jj%10)==0)
						digit(6,jj,pos,& table);
				}else
					strcpy(table.array[5][pos],dash);
			}
			countr = countr + k;
		}

		// Stacking or bifuraction
		i = ip;
		j = jp;
		if (ct->basepr[counter][i]!=j) {
			//bifurcation has occured
			countr = countr+2;
			push(&stack,(ct->basepr[counter][i]+1),j,countr,0);

			j = ct->basepr[counter][i];
		}

		while (ct->basepr[counter][i]==j) {
			countr++;
			strcpy(table.array[3][countr],tobase(ct->numseq[i]));
			strcpy(table.array[4][countr],tobase(ct->numseq[j]));
			if ((i%10)==0) digit(1,i,countr,& table);
			if ((j%10)==0) digit(6,j,countr,& table);
	//?		if (i==iret&&j==jret) {
	//?		strcpy(table.array[2][countr],"|");
	//?		strcpy(table.array[5][countr],"^");
	//?		}
			i++;
			j--;
		}

		goto loop;

		// output portion of structure
		output: ;

		while (1) {
			out << "\n";
			ll = countr;
			if (COL<ll) ll=COL;
			for (k=1;k<=6;k++) {
				for (l=1;l<=ll;l++) {
					out << table.array[k][l];
					// cout << table.array[k][l];
				}
				out <<"\n";
			}
			if (countr<=COL)
				break;
			for (k=1;k<=5;k++) {
				for (j=1;j<=6;j++) {
					strcpy (table.array[j][k],bl);
					strcpy (table.array[j][k+5],bl);
				}
				strcpy(table.array[2][k+5],".");
				strcpy(table.array[5][k+5],".");
			}
			k = 10;
			ll = COL+1;
			for (i=ll;i<=countr;i++) {
				k++;
				for (j=1;j<=6;j++)
					strcpy(table.array[j][k],
						able.array[j][i]);
			}
			countr = k;
		}

		pull (&stack,&i,&j,&countr,&xx,&stz);
   
		if (stz==0) {
			// empty the array
			for (k=1;k<=AMAX-2;k++) {
				for (p=1;p<=6;p++) {
					strcpy(table.array[p][k],bl);
				}
			}
			goto loop;
		}
	}
	return;
}

void digit(int row,int column,int pos,arraystruct* table)
{
//puts the number Column in row Row
char number[10],chr[100],ch [7];
int length,i;

strcpy(number,"");
itoa(column,number,10);//convert column from integer to character
length = strlen(number);

if (pos-length<0) {
	strcpy(table->array[row][pos],".");
	return;
}

for (i=pos;i>=(pos-length+1);i--) {
	strcpy(chr,table->array[row][i]);
	if (strcmp(chr," ")) {
		strcpy(table->array[row][pos],".");
	return;
	}
}

for (i=0;i<=(length-1);i++) {
	strcpy(ch,"");
   strcpy(ch,number+i);
   strcpy(ch+1,"\0");
	strcpy((table->array[row][pos-length+1+i]),ch);
}
return;
}

// convert the base integer represention to a familiar character
char *tobase (int i)
{

	if (i==1)
		return "A";
	else if (i==2)
		return "C";
	else if (i==3)
		return "G";
	else if (i==4)
		return "U";
	else if (i==0
		 return "X";
	else if (i==5)
		return "I";
	else
		return "?";
}

// reorders the structures by the efn energies
void sortstructures (structure *ct)
{
	register int c;
	int cur,i;
	char ctheader[CTHEADERLENGTH];

	for (c = 2; c<=(ct->numofstructures);c++){
		cur = c;
		while (cur>1) {
			if ((ct->energy[cur])<(ct->energy[cur-1])) {
      				swap(&ct->energy[cur],&ct->energy[cur-1]);
				// also swap the ct labels:
				strcpy(ctheader, ct->ctlabel[cur]);
				strcpy(ct->ctlabel[cur],ct->ctlabel[cur-1]);
				strcpy(ct->ctlabel[cur-1],ctheader);
				for (i=1;i<=(ct->numofbases);i++) {
					swap(&ct->basepr[cur][i],
						ct->basepr[cur-1][i]);
				}
				cur--;
			}else{
				break;
			}
		}
	}
}

//this function deallocates the memory used
void de_allocate (int **v,int i)
{
	//in an array
	int a;

	for (a=0;a<i;a++) {
		delete[] v[a];
	}
	delete[] v;
}

// function is sets up the fce array for a base, x, that should be
// single-stranded
void forcesingle(int x,structure* ct,arrayclass *v)
{
	int i;

	for (i=x;i<x+(ct->numofbases);i++) {
		v->f(x,i)=1;
	}
	for (i=1;i<=x;i++) {
		v->f(i,x)=1;
	}
	for (i=x+1;i<=ct->numofbases;i++) {
		v->f(i,x+ct->numofbases) = 1;
	}
}

/*
 * void makefce(int **fce,int nb) {
 * int ir,jr;
 *
 * 	fce = new int *[(2*nb)];
 *	for (ir=0;ir<=(2*nb-1);ir++)
 *		fce[ir] = new int [(nb+1)];
 * 
 *	for (ir=0;ir<=nb;ir++) {
 *		for (jr=0;jr<=2*nb-1;jr++) {
 *			fce[ir][jr] = 0;
 *		}
 *	}
 * // [MAXBASES+1][2*MAXBASES]
 *
 * }
 */

// void endfce(int **fce, int nb)
// {
// int ar;
//	for (ar=0;ar<=nb;ar++) {
//		delete[] fce[ar];
//	}
// delete[] fce;
// }

void forcepair(int x,int y,structure *ct,arrayclass *v)
{
	int i,j;

	v->f(x,y) = 2;
	v->f(y,x+ct->numofbases)=2;
	for (i=y+1;i<=x-1+ct->numofbases;i++) {
		v->f(x,i) = 3;
	}
	for (i=x;i<=y-1;i++) {
		v->f(x,i) = 3;
	}
	for (i=1;i<=x-1;i++) {
		v->f(i,y) = 3;
	}
	for (i=x+1;i<=y;i++) {
		v->f(i,y) = 3;
	}
	for (i=1;i<=x-1;i++) {
		v->f(i,x) = 3;
	}
	for (i=y+1;i<=ct->numofbases;i++) {
		v->f(i,y+ct->numofbases)=3;
	}
	for (i=y;i<=x-1+(ct->numofbases);i++) {
		v->f(y,i) = 3;
	}
	for (i=(ct->numofbases)+x+1;i<=(ct->numofbases)+y-1;i++) {
		v->f(y,i) = 3;
	}
	for (i=x+1;i<=y-1;i++) {
		v->f(i,x+ct->numofbases) = 3;
	}
	for (i=y+1;i<=ct->numofbases;i++) {
		v->f(i,x+ct->numofbases) = 3;
	}
	for (i=1;i<=x-1;i++) {
		for (j = x+1;j<=y-1;j++){
			v->f(i,j) = 3;
		}
	}
	for (i=x+1;i<=y-1;i++) {
		for (j=y+1;j<=(ct->numofbases)+x-1;j++) {
			v->f(i,j) = 3;
		}
	}
	for (i=y+1;i<=ct->numofbases;i++) {
		for (j=(ct->numofbases)+x+1;j<=(ct->numofbases)+y-1;j++) {
			v->f(i,j) = 3;
		}
	}
}

void forcedbl(int dbl,structure* ct,int **w,bool *v)
{
	int i,j;

	// w[dbl][dbl] = 2;
	// w[dbl+ct->numofbases][dbl+ct->numofbases] = 2;
	v[dbl] = true;
	v[dbl+ct->numofbases] = true;

	for(i=dbl+1;i<=ct->numofbases;i++) {
		for (j=1;j<dbl;j++) {
			w[j][i] = 1;
		}
	}
	for(j=(dbl+(ct->numofbases)-1);j>ct->numofbases;j--) {
		for (i=dbl+1;i<=ct->numofbases;i++) {
			w[i][j] = 1;
		}
	}
}

void forceinter(int dbl,structure* ct,int **w)
{
	int i,j;

	for(i=dbl+1;i<=ct->numofbases;i++) {
		for (j=1;j<dbl;j++) {
			w[j][i] = 5;
		}
	}
	for(j=(dbl+(ct->numofbases)-1);j>ct->numofbases;j--) {
		for (i=dbl+1;i<=ct->numofbases;i++) {
			w[i][j] = 5;
		}
	}
}

void forceinterefn(int dbl,structure* ct,int **w)
{
	int i,j;

	for(i=dbl+1;i<=ct->numofbases;i++) {
		for (j=1;j<dbl;j++) {
			w[j][i-j] = 5;
		}
	}
}

void filter(structure* ct, int percent, int max, int window)
{
	i//structure temp;
	int i,j,k1,k2,crit,number;
	bool** mark;
	bool keep;

	mark = new bool *[ct->numofbases+1];
	for (i=0;i<=ct->numofbases;i++) {
  		mark[i] = new bool [ct->numofbases + 1];
	}
	for (i=1;i<=ct->numofbases;i++) {
		for (j=i;j<=ct->numofbases;j++) {
			mark[i][j] = false;
		}
	}

	crit = (ct->energy[1] + (abs(ct->energy[1]*((float (percent))/100))));

	// temp.numofstructures = 0;

	// check each structure:
	number = ct->numofstructures;
	ct->numofstructures = 0;
	for (i=1;i<=number;i++) {
		if (ct->energy[i] > crit) {
  			de_allocate(mark,ct->numofbases+1);
			return;
			// none of the remaining structures should be kept
			// bcs the free energy is > than % sort
		} else if (i>max) {
			de_allocate(mark,ct->numofbases+1);
		   	return;
			// none of the remaining structures should be kept
			// bcs the max # of structures has been reached
		}

		// now map the baspairs within size < window and verify
		// whether this structure should be kept or discarded
		keep = false;
		for (j=1;j<=ct->numofbases;j++) {
			if (ct->basepr[i][j]>j) {
				if (!mark[j][ct->basepr[i][j]]) {
					// this base has not been marked so keep
					// the structure:
					keep = true;
				}
				// now mark the basepairs:
				for (k1=j-window;k1<=j+window;k1++) {
					for(k2=ct->basepr[i][j]-window;
						2<=ct->basepr[i][j]+window;
						2++)
					{
						if ((k1>0)&&(k2>0) &&
							(k1<=ct->numofbases) &&
							(k2<=ct->numofbases))
						{
							mark[k1][k2] = true;
						}
					}
				}
			}
		}

		if (keep) {
			// structure needs to be kept, copy it over to temp
			(ct->numofstructures)++;
			ct->energy[ct->numofstructures] = ct->energy[i];
			for (j=1;j<=ct->numofbases;j++) {
				ct->basepr[ct->numofstructures][j] =		
					ct->basepr[i][j];
			}
		}
	}

	// now move the kept structures back to ct
	// ct->numofstructures = temp.numofstructures;
	// for (i=1;i<=temp.numofstructures;i++) {
	//	ct->energy[i] = temp.energy[i];
	//	for (j=1;j<=ct->numofbases;j++) {
	//		ct->basepr[i][j] = temp.basepr[i][j];
	//	}
	// }

	// clean up memory use:
	de_allocate(mark,ct->numofbases+1);
}

void cctout( structure *ct, char *filename)
{
	int i, j;
	ofstream out(filename);
    
	out << "-100\n";
	out << ct->numofbases << "\n";
	out << ct->numofstructures << "\n";
	// out << ct->ctlabel[1];
	for (i=1;i<=ct->numofbases;i++) {
		out << ct->numseq[i] << "\n";
	}
	for (i=1;i<=ct->numofstructures;i++) {
		for (j=1;j<=ct->numofbases;j++) {
			out << ct->basepr[i][j] << "\n";
		}
	}
}

//opensav opens a save file created by the fill algorithm (function dynamic)
void opensav(char* filename, structure* ct, arrayclass* w, arrayclass *w2,
	arrayclass* v, int *w3, int *w5,int *vmin, datatable *data)
{
	int i,j,k,l,m,n,o,p;
	output convert,sign;

	ifstream in(filename,ios::binary);
	strcpy(ct->ctlabel[1],"");


	// in.get(ct->ctlabel[1],CTHEADERLENGTH);
	in.read(ct->ctlabel[1],CTHEADERLENGTH);
	// strcat(ct->ctlabel[1],"\n");
	// in >> ct->numofbases;
	in.read(convert.ch,2);
	ct->numofbases = convert.i;

	for (i=1;i<=ct->numofbases;i++) {
		//in >> ct->numseq[i];
		in.read(convert.ch,2);
		ct->numseq[i] = convert.i;
		in.read(ct->nucs+i,1);
		in.read(convert.ch,2);
		ct->hnumber[i] = convert.i;
	}

	// in >> ct->npair;
	// in >> ct->ndbl;
	// in >> ct->nnopair;
	in.read(convert.ch,2);
	ct->npair = convert.i;
	in.read(convert.ch,2);
	ct->ndbl = convert.i;
	in.read(convert.ch,2);
	ct->nnopair = convert.i;
	in.read(convert.ch,2);
	ct->ngu = convert.i;

	for (i=1;i<=ct->npair;i++) {
		// in >> ct->pair[i][0];
		// in >> ct->pair[i][1];
		in.read(convert.ch,2);
		ct->pair[i][0] = convert.i;
		in.read(convert.ch,2);
		ct->pair[i][1] = convert.i;
	}
	for (i=1;i<=ct->ndbl;i++) {
		in.read(convert.ch,2);
		ct->dbl[i] = convert.i;
		// in >> ct->dbl[i];
	}
	for (i=1;i<=ct->nnopair;i++) {
		in.read(convert.ch,2);
		ct->nopair[i] = convert.i;
		// in >> ct->nopair[i];
	}
	for (i=0;i<ct->ngu;i++) {
		in.read(convert.ch,2);
		ct->gu[i] = convert.i;
	}

	// in >> intermolecular;
	convert.ch[0] = 0;
	convert.ch[1] = 0;
	convert.ch[2] = 0;
	convert.ch[3] = 0;
	in.read(convert.ch,1);

	if (convert.i==1) {
		ct->intermolecular = true;
		for (i=0;i<3;i++) {
			// in >> ct->inter[i];
			in.read(convert.ch,2);
			ct->inter[i] = convert.i;
		}
		w2 = new arrayclass(ct->numofbases);
	} else
		ct->intermolecular = false;

	sign.ch[0] = 0;
	sign.ch[1] = 0;
	sign.ch[2] = 0;
	sign.ch[3] = 0;
	for (i=0;i<=ct->numofbases;i++) {
		in.read(sign.ch,1);
		// in >> w5[i];
		in.read(convert.ch,2);
		if (sign.ch[0] == 0)
			w5[i] = -convert.i;
		else
			w5[i] = convert.i;
	}
	for (i=0;i<=ct->numofbases+1;i++) {
		// in >> w3[i];
		in.read(sign.ch,1);
		// in >> w5[i];
		in.read(convert.ch,2);
		if (sign.ch[0] == 0)
			w3[i] = -convert.i;
		else
			w3[i] = convert.i;
	}
	for(i=0;i<=ct->numofbases;i++) {
		for (j=0;j<=ct->numofbases;j++) {
			// in >> v->dg[i][j];
			// in >> w->dg[i][j];
			in.read(sign.ch,1);
			in.read(convert.ch,2);
			if (sign.ch[0] == 0)
				v->dg[i][j] = -convert.i;
			else
				v->dg[i][j] = convert.i;

			in.read(sign.ch,1);
			in.read(convert.ch,2);
			if (sign.ch[0] == 0)
				w->dg[i][j] = -convert.i;
			else
				w->dg[i][j] = convert.i;
			if (ct->intermolecular) {
				in.read(sign.ch,1);
				in.read(convert.ch,2);
				if (sign.ch[0] == 0)
					w2->dg[i][j] = -convert.i;
				else
					w2->dg[i][j] = convert.i;
			}
		}
	}
	// in >> (*vmin);
	in.read(sign.ch,1);
	in.read(convert.ch,2);
	if (sign.ch[0] == 0)
		(*vmin) = -convert.i;
	else
		(*vmin) = convert.i;

	// also save the thermodynamic parameters
	// convert.i = ct->pair[i][0];
	// sav.write(convert.ch,2);
	for (i=0;i<5;i++) {
		data->poppen[i] = readfile(&in);
	}
	data->maxpen = readfile(&in);

	for (i=0;i<11;i++) {
		data->eparam[i] = readfile(&in);
	}
	for (i=0;i<6;i++) {
		for (j=0;j<6;j++) {
			for (k=0;k<6;k++) {
				for (l=0;l<3;l++) {
					data->dangle[i][j][k][l]=readfile(&in);
				}
			}
		}
	}
	for (i=0;i<31;i++) {
   		data->inter[i] = readfile(&in);
		data->bulge[i] = readfile(&in);
		data->hairpin[i] = readfile(&in);
	}
	for (i=0;i<6;i++) {
		for (j=0;j<6;j++) {
			for (k=0;k<6;k++) {
				for (l=0;l<6;l++) {
					data->stack[i][j][k][l] = readfile(&in);
					data->tstkh[i][j][k][l] = readfile(&in);
					data->tstki[i][j][k][l] = readfile(&in);
					data->coax[i][j][k][l] = readfile(&in);
					data->tstackcoax[i][j][k][l] =
						readfile(&in);
					data->coaxstack[i][j][k][l] =
						readfile(&in);
					data->tstack[i][j][k][l] =
						readfile(&in);
					data->tstkm[i][j][k][l] = readfile(&in);
				}
			}
		}
	}
	for (i=0;i<MAXTLOOP+1;i++) {
		for (j=0;j<2;j++) {
			data->tloop[i][j] = readfile(&in);
		}
	}
	data->numoftloops = readfile(&in);

	for (i=0;i<6;i++) {
	    for (j=0;j<6;j++) {
      		for (k=0;k<6;k++) {
         	    for (l=0;l<6;l++) {
            		for (m=0;m<6;m++) {
               		    for (n=0;n<6;n++) {
                  		data->iloop11[i][j][k][l][m][n] = readfile(&in);
				for (o=0;o<6;o++) {
				    data->iloop21[i][j][k][l][m][n][o] =
					readfile(&in);
				    for (p=0;p<6;p++) {
					data->iloop22[i][j][k][l][m][n][o][p] =
					    readfile(&in);
				    }
				}
			    }
			}
		    }
		}
	    }
	}

	data->auend = readfile(&in);
	data->gubonus = readfile(&in);
	data->cint = readfile(&in);
	data->cslope = readfile(&in);
	data->c3 = readfile(&in);
	data->efn2a = readfile(&in);
	data->efn2b = readfile(&in);
	data->efn2c = readfile(&in);
	data->numoftriloops = readfile(&in);

	for (i=0;i<MAXTLOOP+1;i++) {
		for (j=0;j<2;j++) {
			data->triloop[i][j] = readfile(&in);
		}
	}
	in.read(convert.ch,4);
	data->prelog = convert.f;

	data->init = readfile(&in);

	in.close();
}

void dotefn2(structure *ct, datatable *data, arrayclass *v, arrayclass *w,
	arrayclass *w2, int *w3, int *w5, int **fce, bool *lfce, 
	int vmin,dotarray *dots, TProgressDialog* PD)
{
	int i,j,k,lfe;
	int *bp;

	// lfe will store the lowest free energy found
	// bp will be used to store the basepairs in the lowest free energy
	// structure
	bp = new int [ct->numofbases+1];

	if (vmin > 0) {
		// No favorable structure, return INFINITY
		// for (i=1;i<=ct->numofbases;i++) {
		// 	for (j=i;j<=ct->numofbases;j++) {
		//      	dots[i][j] = INFINITY;
		//	}
		// }
		return;
	}

	lfe = 0;
	ct->energy[1] = INFINITY;
	ct->numofstructures = 1;

	// fill in the table dots with the lowest energy possible
	// with a basepair at position i,j
	for (i=1;i<ct->numofbases;i++) {
	    if (PD!=0)
		PD->update((100*i)/(ct->numofbases));
	    for (j=i+1;j<=ct->numofbases;j++) {
		if ((v->f(i,j)+v->f(j,i+ct->numofbases))<0) {
		    //traceback a structure
		    for (k=1;k<=2;k++) {
			if (k==1) {
               		    trace(ct,data,i,j,v,w,w2, 1,fce,fce,w3,w5);
			}else{
               		    trace(ct,data,j, i+ct->numofbases,v,w,w2,
				1,lfce,fce,w3,w5);
			}
		    }
		    // calculate the efn2 energy for that structure
		    efn2(data,ct);
		    if (ct->energy[1]<lfe) {
			// this is the lowest free energy
			// structure found yet, store the						// basepairs
			for (k=1;k<=ct->numofbases;k++) {
				bp[k] = ct->basepr[1][k];
			}
			lfe = ct->energy[1];
		    }
		    // record this energy for each basepair in the
		    // structure for which this energy is lower
		    for (k=1;k<=ct->numofbases;k++) {
		        if (ct->basepr[1][k]>k) {
			    //there is a basepair here
               		    if (ct->energy[1] <
				dots->dot(k,ct->basepr[1][k]))
			    {
                  		dots->dot(k,ct->basepr[1][k]) = ct->energy[1];
			    }
		        }
		    }

		} //else
		  //	dots[i][j] = INFINITY;
	    }
	}

	// now place the lowest free energy structure in ct
	for (k=1;k<=ct->numofbases;k++) {
		ct->basepr[1][k] = bp[k];
	}
	ct->energy[1] = lfe;
}

void calcpnum(dotarray *dots, int *pnum, int increment, int numofbases,
	TProgressDialog *PD)
{
	int i,j;

	for (i=1;i<=numofbases;i++) {
		pnum[i] = 0;
		// count the dots in the ith column
		for (j=i+1;j<=numofbases;j++) {
			if (dots->dot(i,j)<=increment)
				pnum[i]++;
		}
		// count the dots in the ith row
		for (j=1;j<i;j++) {
			if (dots->dot(j,i)<=increment)
				pnum[i]++;
		}
	}
}

void savefile(int i, ofstream* sav)
{
	output convert;

	if (i<0) {
 		convert.i = 0;
   		sav->write(convert.ch,1);
   		convert.i = -i;
	}else{
		convert.i = 1;
   		sav->write(convert.ch,1);
   		convert.i = i;
	}
	sav->write(convert.ch,2);
}

int readfile(ifstream* in)
{
	output convert;

	convert.i = 0;
	in->read(convert.ch,1);
	if (convert.i) {
		in->read(convert.ch,2);
	}else{
		in->read(convert.ch,2);
		convert.i = -convert.i;
	}
	return convert.i;
}

//	dot array functions:

dotarray::dotarray(int size)
{
	int i,j;

	// initialize the array
	array = new int *[size+1];

	for (i=0;i<=(size);i++)  {
		array[i] = new int [i+1];
	}

	for (i=0;i<=size;i++) {
		for (j=0;j<=i;j++) {
			array[i][j] = INFINITY;
		}
	}

	store = size;
}

dotarray::~dotarray()
{
 	int i;

	for (i=0;i<=store;i++) {
		delete[] array[i];
	}
	delete[] array;
}

//save dot plot info
void savedot(dotarray *dots,structure *ct,char *filename)
{
	int i,j;
	output out;

	ofstream sav(filename,ios::binary);
	out.i = 0;
	out.i = ct->numofbases;
	sav.write(out.ch,2);

	// now output the sequence:
	for (i=1;i<=ct->numofbases;i++) {
		out.i = ct->numseq[i];
		sav.write(out.ch,1);
	}

	savefile(ct->energy[1],&sav);

	// now output the dot information
	for(i=1;i<=ct->numofbases;i++) {
		for (j=i;j<=ct->numofbases;j++) {
			savefile(dots->dot(i,j),&sav);
		}
	}

	sav.close();
}

void readdot(dotarray *dots, structure *ct, char *filename)
{

	int i,j;
	output convert;

	ifstream in(filename,ios::binary);
	convert.i = 0;
	// get past the number of bases, which we already know
	in.read(convert.ch,2);

	// input the sequence
	for (i=1;i<=ct->numofbases;i++) {
		convert.i=0;
		in.read(convert.ch,1);
		ct->numseq[i] = convert.i;
	}

	ct->energy[1] = readfile(&in);

	// now read the dot information
	for(i=1;i<=ct->numofbases;i++) {
		for (j=i;j<=ct->numofbases;j++) {
			dots->dot(i,j) = readfile(&in);
		}
	}
	in.close();
}

//This function will align two dot plots
void dpalign(dotarray *dots1,dotarray *dots2,structure* ct1,structure *ct2,
	int *align)
{
	int i,j,a,b;
	int temp,best,besti,bestj;
	int **array;

	// make a 2-d array
	array = new int *[ct1->numofbases+1];
	for (i=0;i<=ct1->numofbases;i++)
		array[i] = new int [ct2->numofbases+1];

	// fill the array:
	best = INFINITY;
	for (i=1;i<=ct1->numofbases;i++) {
		cout << "i = " << i << "\n";
		for (j=1;j<=ct2->numofbases;j++) {
			// array[i][j] = INFINITY;
			array[i][j] = getbestdot(dots1,dots2,ct1,ct2, i, j);
			if (array[i][j]<INFINITY) {
				temp = 0;
				// look for extension of the alignment
				for (a=1;a<i;a++) {
					for (b=1;b<j;b++) {
						if (temp>array[a][b])
							temp = array[a][b];
					}
				}
			}
			array[i][j] = array[i][j] + temp;
			if (best>array[i][j]) {
				best = array[i][j];
				besti = i;
				bestj = j;
			}
		}
	}

	// now trace back:
	// start at the best alignment position, besti-bestj and work back

	for (i=ct1->numofbases;i>besti;i--)
		align[i] = 0;
	align[besti] = bestj;

	i = besti;
	j = bestj;
	// look for how to make the energy of i an j
	trace: ;
	temp = getbestdot(dots1,dots2,ct1,ct2, i, j);
	if (temp==array[i][j]) {
		// the traceback is complete
		// finish up and return
		for (a=i-1;a>0;a--)
			array[a] = 0;
		return;
	}else{
		// find where the alignment continues
		for (a=i-1;a>=1;a--) {
			for (b=j-1;b>=1;b--) {
				if (array[i][j]==temp+array[a][b]) {
					align[a] = b;
					for (j=i-1;j>a;j--) {
						align[j] = 0;
					}
					i = a;
					j = b;
					goto trace;
				}
			}
		}
	}

	// clean up memory use
	for (i=0;i<=ct1->numofbases;i++)
		delete[] array[i];
	delete[] array;
}

int getbestdot(dotarray *dots1,dotarray *dots2, structure* ct1,
	structure *ct2, int i, int j)
{
	int k;
	// int inc[6][6] =
	//	{{0,0,0,0,0,0},
	//	 {0,0,0,0,1,0},
	//	 {0,0,0,1,0,0},
	//	 {0,0,1,0,1,0},
	//	 {0,1,0,1,0,0},
	//	 {0,0,0,0,0,0}
	// };
	int val,val2,value,value2;

	val = INFINITY;
	value = INFINITY;
	for (k=i+1;k<=ct1->numofbases;k++) {
		if (val>dots1->dot(i,k)) val = dots1->dot(i,k);
	}
	for (k=1;k<i;k++) {
		if (value>dots1->dot(k,i)) value = dots1->dot(k,i);
	}
	val2 = INFINITY;
	value2 = INFINITY;
	for (k=j+1;k<=ct2->numofbases;k++) {
		if (val2>dots2->dot(j,k)) val2 = dots2->dot(j,k);
	}
	for (k=1;k<j;k++) {
		if (val2>dots2->dot(k,j)) value2 = dots2->dot(k,j);
	}

	if ((val+val2) < (value+value2))
		return (val + val2);
	else
		return (value+value2);
}

// This function will give an energy breakdown for a structure, int n,
// stored in ct, it takes the arrays below as input
// note: this is not fully debugged
void energydump (structure *ct, arrayclass *v, int n,char *filename)
{
	int stack[500],stackpos,i,j,k,temp,count,helix;
	ofstream out;
	char number[6];

	out.open(filename);
	stackpos = 0;

	gcvt((float (ct->energy[n]))/100.0,8,number);
	out << "Structure:  "<< n << "\n";
	out << "\n# " << n << "  Total Energy = " << number << "\n\n";

	temp = ct->energy[n];

	// Analyze the exterior loop
	i = 0;
	while (i<ct->numofbases) {
		i++;
		if (ct->basepr[n][i]>0) {
			stackpos++;
			stack[stackpos] = i;
			temp = temp - v->f(i,ct->basepr[n][i]);
			i = ct->basepr[n][i];
		}
	}
	gcvt((float (temp))/100.0,8,number);
	out << "Exterior loop energy = " << number << "\n";

	while (stackpos>0) {
		i = stack[stackpos];
		stackpos--;

		helix = 0;
		// follow the helix:
		while (ct->basepr[n][i+1]==ct->basepr[n][i]-1) {
			// helix continues:
			temp = v->f(i,ct->basepr[n][i]) -
				v->f(i+1,ct->basepr[n][i+1]);
			gcvt((float (temp))/100.0,8,number);
			helix = helix + temp;
			out << "Stack energy = " << number << "  for " <<
				(i+1) << "-" << ct->basepr[n][i+1] <<
				" onto " << i << "-" <<
				ct->basepr[n][i] << "\n";
		i++;
		}
		gcvt((float (helix))/100.0,8,number);
		out << "\tHelix energy = " << number << "\n";

		if (i==37) {
			v->f(0,0) = 0;
		}

		// now we've come to a loop, what type?
		j = ct->basepr[n][i];
		temp = v->f(i,j);
		count = 0;
		while (i<j-1) {
			i++;
			if (ct->basepr[n][i]>0) {
				//we've found another helix
				count++;
				k = ct->basepr[n][i];
				temp = temp - v->f(i,k);
				stackpos++;
				stack[stackpos] = i;
				i = k+1;
			}
		}
		if (count ==0) {
			// hairpin:
			gcvt((float (temp))/100.0,8,number);
			out << "Hairpin energy = " << number <<
				"  for closure by "<< ct->basepr[n][j] <<
				"-" << j << "\n";
		} else if (count==1) {
			// bulge or internal loop:
			gcvt((float (temp))/100.0,8,number);
			out << "Bulge/Internal loop energy = " << number <<
				"  for closure by "<< ct->basepr[n][j] <<
				"-"<<j<<"\n";
		} else {
			// multi loop
			gcvt((float (temp))/100.0,8,number);
			out << "Multibranch loop energy = " << number <<
				"  for closure by "<< ct->basepr[n][j] <<
				"-"<<j<<"\n";
		}
	}

	out.close();
}

// This function will give an energy breakdown for a structure, int n,
// stored in ct, it takes the arrays below as input
// note:this is not fully debugged
void energydump (structure *ct, datatable *data,arrayclass *v,
	int n,char *filename)
{
	int stack[500],stackpos,i,j,k,temp,count,helix,auaddition;
	ofstream out;
	char number[6],auend[6];
	int inc[6][6] =
		{ {0,0,0,0,0,0},
		  {0,0,0,0,1,0},
		  {0,0,0,0,0,0},
		  {0,0,0,0,1,0},
		  {0,1,0,1,0,0},
		  {0,0,0,0,0,0}
		};
	bool sbulge;

	sbulge = false;
	out.open(filename);
	stackpos = 0;

	gcvt((float (data->auend))/100.0,8,auend);

	gcvt((float (ct->energy[n]))/100.0,8,number);
	out << "Structure:  "<< n << "\n";
	
	out << "\n# " << n << "  Total Energy = " << number << "\n\n";

	temp = ct->energy[n];

	// Analyze the exterior loop
	i = 0;
	while (i<ct->numofbases) {
		i++;
		if (ct->basepr[n][i]>0) {
			stackpos++;
			stack[stackpos] = i;
			if (inc[ct->numseq[i]][ct->numseq[ct->basepr[n][i]]]) {
				temp = temp - data->auend;
			}
			temp = temp - v->f(i,ct->basepr[n][i]);
			i = ct->basepr[n][i];
		}
	}

	gcvt((float (temp))/100.0,8,number);
	out << "Exterior loop energy = " << number << "\n";

	while (stackpos>0) {
		i = stack[stackpos];
		stackpos--;
		helix = 0;

		if (inc[ct->numseq[i]][ct->numseq[ct->basepr[n][i]]]&&!sbulge){
			out << "Non-GC end = " << auend<< "\n";
			helix = helix + data->auend;
		}
		sbulge = false;

		// follow the helix:
		while (ct->basepr[n][i+1]==ct->basepr[n][i]-1) {
			// helix continues:
			temp = v->f(i,ct->basepr[n][i]) -
				v->f(i+1,ct->basepr[n][i+1]);
			gcvt((float (temp))/100.0,8,number);
			helix = helix + temp;
			out << "Stack energy = " <<number << "  for " <<
				(i+1) << "-" << ct->basepr[n][i+1] << " onto "
				<< i << "-" << ct->basepr[n][i] << "\n";
			i++;
		}

		// now we've come to a loop, what type?
		auaddition = 0;
		j = ct->basepr[n][i];
		temp = v->f(i,j);
		count = 0;
		while (i<j-1) {
			i++;
			if (ct->basepr[n][i]>0) {
				//we've found another helix
				if (inc[ct->numseq[i]]
					[ct->numseq[ct->basepr[n][i]]])
				{
					auaddition = auaddition - data->auend;
				}
				count++;
				k = ct->basepr[n][i];
				temp = temp - v->f(i,k);
				stackpos++;
				stack[stackpos] = i;
				i = k+1;
			}
     		}

		//check for single bulge:
		if (count==1){
			if (ct->basepr[n][j-1]>0){
				if (ct->basepr[n][ct->basepr[n][j]+1]==0){
					if(ct->basepr[n][ct->basepr[n][j]+2]>0){
						sbulge = true;
						auaddition = 0;
					}
				}
			} else if (ct->basepr[n][ct->basepr[n][j]+1]>0){
				if (ct->basepr[n][j-1]==0){
					if (ct->basepr[n][j-2]>0) {
						sbulge = true;
						auaddition = 0;
					}
				}
			}
		}

		if (inc[ct->numseq[j]][ct->numseq[ct->basepr[n][j]]]&&!sbulge){
			out << "Non-GC end = " << auend << "\n";
			helix = helix + data->auend;
		}

		gcvt((float (helix))/100.0,8,number);
		out << "\tHelix energy = " << number << "\n";

		if (count ==0) {
			// hairpin:
			temp = temp + auaddition;
			gcvt((float (temp))/100.0,8,number);
			out << "Hairpin energy = " << number <<
				"  for closure by " << ct->basepr[n][j] <<
				"-" << j << "\n";
		}else if (count==1){
			// bulge or internal loop:
			temp = temp + auaddition;
			gcvt((float (temp))/100.0,8,number);
			out << "Bulge/Internal loop energy = " << number <<
				"  for closure by " << ct->basepr[n][j] <<
				"-"<<j<<"\n";
		}else{
			// multi loop
			temp = temp + auaddition;
			gcvt((float (temp))/100.0,8,number);
			out << "Multibranch loop energy = " << number <<
				"  for closure by " << ct->basepr[n][j] <<
				"-" << j << "\n";
		}
	}

	out.close();
}
