#  Edit the configuration variables below to match your system.  The default
#  values given here should work on GNU/Linux systems.

CC= gcc
CFLAGS= -O2 -Wall

#  Notes: clang can replace gcc above;
#         for icc (version 13.0.1) you need to use -O1 in cflags.

#  The yacc compiler is typically "yacc" (for the *real* yacc), "byacc"
#  (for the Berkeley equivalent) or "bison -y" (to get yacc compatible output 
#  from bison).  Note that /bin/yacc is sometimes a symlink to bison; on
#  such systems you need the -y flag.

#  Note that you need to get version 1.875 or later of bison.  Since this 
#  was released in 2003, that should not be much of a problem....
#  Go to http://www.gnu.org/software/bison/bison.html for more information.

YACC = bison -y

#  The GNU version of lex is called flex.  So the following variable is 
#  either "lex" or "flex", depending upon your system.

LEX = flex

# Uncomment & use these 3 lines to use Genbank in addition to FASTN:

# GBHOME=/home/macke/gensearch
# GBLIB=$(GBHOME)/libgb.a
# CFLAGS=-O2 -DUSE_GENBANK

