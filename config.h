#  Edit the configuration variables below to match your system.  The default
#  values given here should work on GNU/Linux systems.

CC= gcc
CFLAGS= -O2
#CFLAGS= -g

#  The yacc compiler is typically "yacc" (for the *real* yacc), "byacc"
#  (for the Berkeley equivalent) or "bison -y" (to get yacc compatible output 
#  from bison).  Note that /bin/yacc is sometimes a symlink to bison; on
#  such systems you need the -y flag.

#  Note that version of bison (like 1.35 that is packaged with a number of
#  Linux distributions) will not work; you need to get version 1.875 or later.
#  Go to http://www.gnu.org/software/bison/bison.html for more information.

YACC = bison -y

#  The GNU version of lex is called flex.  So the following variable is 
#  either "lex" or "flex", depending upon your system.

LEX = flex

# Uncomment & use these 3 lines to use Genbank in addition to FASTN:

# GBHOME=/home/macke/gensearch
# GBLIB=$(GBHOME)/libgb.a
# CFLAGS=-O2 -DUSE_GENBANK

