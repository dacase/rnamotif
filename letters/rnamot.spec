The input to RNAMOT consists of two parts:

	1. A decription of the 2d structure that is to be matched.

	2. An optional set of scoring rules that allow users to set rank
	   matches and to discard matches with a score below some threshhold.

	   The default scoring rule is:

		If every descriptor in the pattern is matched, the sequence
		is acceptable.

Descriptor Language Spec:

1.	Components of descriptors:

	Structure elements including single stranded or otherwise unstructured.
	Identifiers or labels of the structure elements.
	Literals including numbers (ints, floats?) and strings.
	Addressing operators. (Used for connectivity)
	Some set of punctuation characters to make it all work.

----

2.	Lexical	Level.

	Comments:

	Lines beginning with '#' are ignored.

	White space:

	Descriptor input is free format where any run of unquoted spaces, tabs
	and newlines is considered a single space.

	Strings are sequences of characters enclosed in quotes (", '). 
	Unescaped newlines are not permitted in strings.
	The usual C-escapes (\",\t,\n, etc) are used

	Strings represent sequence constraints and ALWAYS are read left->right
	indicating 5'->3'.

	Integers are sequences of one or more digit (0-9). Integer are
	always base TEN and are >= 0.

	Floats if we need them are the usual CS float rep: N. N.N, .N, NeE,
	N.eE, N.NeE & .NeE.  Agains floats are always >= 0 and always in 
	base TEN.

	Identifiers are begin with a letter followed by zero of more letters
	and digits. The underscore is a letter.  Case counts?  ID's must be
	distinct in their first 32? 256? chars.

	Structure descriptors are special IDs. 

	Suggestions:

	s		single stranded
	h5		5' strand of a WC-helix
	h3		3' strand of a WC-helix
	p1		5' strand of a parallel helix
	p2		3' strand of a parallel helix
	t1, t2, t3	The three strands of a triples, from 5'->3'
	q1, q2, q3, q4	The four strands of a quadruplex, from 5'->3'

	Labels are either ints or IDs.  Structure descriptors can not be
	used as labels.

----

	Addressing should be relative from the start of each sub structure.
	Bases are numbered from 1 to $ where 1 is the 5' base and $ is the 3'
	base of that structure.

	Symbolic addressing including addressing from the 3' end should be
	supported:  Example, $ represents the position of the last (3') base
	of a sequence, the $-1, $-2, ... represent the 2nd from last, 3rd
	from last etc.

----

	Structure descriptors are also numbered beginning with 1.
	Labels are not required if only WC helices are properly nested:

		stem:h5("gcgc") loop:s("uucg") stem:h3("gcgc")

	could be shortened to 

		h5("gcgc") s("uucg") h3("gcgc")
