
   The "rnamotif" program searchs input sequences for portions that match
   a given descriptor or "motif".  Matching sequences can also be ranked by
   various scoring functions.  See doc/rnamotif.pdf for documentation.

==============================================================================

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any later
   version. The GNU General Public License should be in a file called
   LICENSE; if not, write to the Free Software Foundation, Inc., 59 Temple
   Place, Suite 330, Boston, MA  02111-1307  USA

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License 
   for more details.

   We thank Dave Ecker and Ranga Sampath for encouragement, and Isis
   Pharmaceuticals for financial support; Robin Gutell and Daniel Gautheret for
   suggestions; Vickie Tsui for serving as a beta tester, and for many helpful
   comments and suggestions.

==============================================================================

Quick-start guide to installation and testing of rnamotif:

(a)  Edit the "config.h" file in this directory to suit your local 
     environment.  This may require no change on many machines, but
     you may need to locate the C-compiler, lex or yacc, and set compiler
     flags.

(b)  Now "make" will make the executables, leaving them in the src
	 subdirectory.  There is no "install" target, but (after you run the
     test suite) you can manually move "rnamotif", "rmfmt", "rm2ct" and 
     "rmprune" to someplace in your PATH if you wish.

(c)  Set and export the environment variable EFNDATA to point to the efndata
     subdirectory under this one.  Then "make test" will run some test
     calculations and report results.  (Warning: running the tests
     may take some time.)

     For example: if you installed under /usr/local, and you use sh,
     bash, or zsh:

        export EFNDATA=/usr/local/rnamotif-3.1.1/efndata/

     If you use csh or tcsh:

        setenv EFNDATA /usr/local/rnamotif-3.1.1/efndata/

Please send comments and questions to Tom Macke <macke_tom@yahoo.com>.

==============================================================================

The full documentation is in the doc/rnamotif.pdf file.  A brief summary
is given here:

I. Installing rnamotif.

1. rnamotif can be cloned from github:

	https://github.com/dacase/rnamotif

2. This file contains a single top level directory, rnamotif, which contains
   several subdirectories which in turn contain the sources to rnamotif, three
   utility programs (rmprune, rmfmt and rm2ct), test data and data required to
   compute the free energy of the potential secondary structures found by
   rnamotif.

4. Change directory to where rnamotif was cloned, and edit the configuration 
   file config.h

5. make the binaries: 

	make

   The binaries will left in the subdirectory 'src'.  There are four:

*	rnamotif	The actual serach program.
*	rmprune		Remove hits that differ only by 'unzipped' base pairs.
*	rmfmt		Format the output.
*	rm2ct		Convert the output to ct-format suitable for drawing.

6. Test the binaries.  One test computes the effective energies of the 
   solutions.  To run this, rnamotif must read the files that are in 
   the subdirectory "efndata".  Please set this environment symbol EFNDATA 
   to point to the directory rnamotif/efndata and then run the tests:

	make test >& test.out &

7. This can take up to 20 minutes.  This runs rnamotif on 9 descriptors in
   the subdirectory "test".  These descriptors seaerch the database
   gbrna.111.0.fastn which contains all of the sequences of the (no longer
   broken out) RNA part of Genbank, release 111, from April 1999.

   Generally all tests pass, as indicated by the word PASSED in the file
   test.out following the lines that run each test.  However, the sort command
   in Red Hat 6.2 (Zoot) behaves differently than other Unixes, or even other
   versions of Red Hat and sorts the output of the score.1, mp.ends and sprintf
   tests differently.  These tests are not marked as PASSED, but the problem is
   due to the odd sort, so they do pass.

8. (Optional) Move the binaries and/or efndata.  Remember that the symbol
   EFNDATA must point to the directory that contains the energy data in order
   for rnamotif to compute the dG of the solutions.

II. Using the codes.

1. rnamotif.

1.1 rnamotif is run from the command line.  It takes two arguments: a descriptor
   that specifies the 2d strucutre to look for, and a file of fastn data to
   search for sequences that can adopt that 2d structure.  The output is
   written to stdout. The command
   
	rnamotif -descr test.descr test.fastn

   searches the fastn file 'test.fastn' for sequences that can fold up into the
   2d structures specified in the descriptor file 'test.descr'  If no fastn
   file is given, rnamotif uses stdin.  Note that in the above example, the
   files have suffixes .descr and .fastn; however, these suffixes are only a
   convention, as they can have any names.

1.2. The full form of the rnamotif command is shown below:

    rnamotif [ options ] descr [ seq-file ... ]

    options:
	-c			Compile only, no search
	-d			Dump internal data structures
	-h			Dump the strucure hierarchy
	-s			Show builtin variables
	-v			Print Version Infomation
	-Dvar=expr		Set the value of var to expr
	-Idir			Add include source directory, dir
	-xdfname file-name	Preprocessor output file
	-help			Print this message

    descr:	Use one:
	-descr descr-file	May have includes; use cmd-line defs
	-xdescr xdescr-file	May not have includes; ignore cmd-line defs

2. rmprune.

2.1 Many descriptors, eg, a variable length stem that contains a variable
   length loop have several solutions that differ only in that the stem has
   had bases on one or the other or both ends `unzipped'  rmprune takes the
   output of rnamotif and removes unzipped solutions, leaving only that
   solution with the longest possible stem.

   As an example consider this descriptor:

  descr
	h5( minlen=4, maxlen=6 ) ss( minlen=4, maxlen=6 ) h3

   This descriptor specifies a simple hairpin where the stem must be 4-6
   base pairs and the loop 4-6 bases.  When rnamotif searches the sequence

	aaaaaacccctttttt

   it finds 11 possible hairpins:

	 1	1 14 aaaa   aacccc   tttt
	 2	1 15 aaaaa  acccc   ttttt
	 3	1 16 aaaaa  acccct  ttttt	4
	 4	1 16 aaaaaa cccc   tttttt	
	 5	2 13 aaaa   acccc    tttt
	 6	2 14 aaaa   acccct   tttt
	 7	2 14 aaaaa  cccc    ttttt
	 8	2 15 aaaaa  cccct   ttttt
	 9	3 12 aaaa   cccc     tttt
	10	3 13 aaaa   cccct    tttt
	11	3 14 aaaa   cccctt   tttt

   Many of these hairpins are merely subsets of the base pairs of the others.
   rmprune removes these 'unzipped' subsets, leaving 5 unique pairings:

	 1	1 14 aaaa   aacccc   tttt
	 2	1 15 aaaaa  acccc   ttttt	contains 5
	 4	1 16 aaaaaa cccc   tttttt	contains 3,6,7,8
	 8 	2 15 aaaaa  cccct   ttttt	contains 10
	11	3 14 aaaa   cccctt   tttt
                                       
2.2. rmprune is run from the command line and takes an optional argument,
    a file containing the output of an rnamotif search.  If this file is
    not given, it reads from stdin, allowing it to be placed in a pipeline
    following rnamotif:

	rnamotif -descr hp.descr test.fastn | rmprune

3. rmfmt.

3.1. This program is used to format rnamotif output.  Two formats are supported.
    The first is a viewing option that removes the fastn definition line from
    the rnamotif output.  At the same time sequences that correspond to long
    fields in the descriptor (ie > 20 bases) are condensed, showing only the
    1st 3 and last 3 bases surroungind the total length of the string in parens.
    For example the string 'acgtacgtacgtacgtacgtacgt' (len=24) would be
    replaced by 'acg...(24)...cgt'.  The strings are also aligned so that
    strings that represent ss, h5, p5, t1, t3, q1 and q3 elements are left
    justified and those that correspond to h3, p3, t2, q2 and q4 fields are
    right justified.   The name of each solution is also simplified, using
    only the LOCUS name from the fastn definition line.  Finally, if at all
    possible the rnamotif solutions are sorted into descending order based on
    the value of the score column (#2) placing the best (or worst) solutions at
    the top.  This format option is specified by the -l option.

    The second format option is to convert the rnamotif output into a fastn
    format alignemnt.  Justification is as in -l, but the fill characater is
    the dash (-).  Fields are separated by a vertical bar (|).  Aligned output
    is not sorted and the fastn definition lines are retained.  This format
    option is specified by the -a option.

3.2. The full rmfmt command is show below:

	rmfmt [ -a ] [ -l ] [ -smax N ] [ -td dir ] [ rm-output-file ]

    The -smax option sets an upper bound on the size of the file of the output
    to sort, as this can be impractical for very large output.  The default
    sort limit is 30MB.  The -td option allows the user to specify the temp
    directory that rmfmt places its work files in.  These files are needed
    because the alignment can not be computed until all solutions have been
    seen.  The default temp directory is system dependent and is either /tmp
    or /var/tmp.

    As indicated by the command line, -a and -l can be used together in which
    case the file is formatted with dashes, unsorted and the names of the
    sequences are reduced to the last name in the bar separated name section
    of the fastn definition line.

4. rm2ct.

4.1. This program converts the rnamotif output to a ct-format structure file,
    which can used by various programs to create a graphical representation of
    the sequence and its 2d structure.

4.2. The rm2ct command takes a single optional argument, the name of the file
    containing the rnamotif output to convert.  The output is written to stdout.
    If no argument is given, rm2ct reads from stdin, allowing it to stand in a
    pipe:

	rnamotif -descr hp.descr test.fastn | rm2ct

4.3. ct-format can only represent duplexes, for this reason rm2ct exits with
    an error message if its input contains triples or quads.  However, all
    dumplexes, including parallel duplexes and pseudoknots are converted,
    although not it is likely that many ct-format plotting programs can not
    handle such structures.

