#  Edit the configuration variables below to match your system:

CC= gcc
CFLAGS= -O2
#CFLAGS= -g

#  the yacc compiler is typically "yacc" (for the *real* yacc), "byacc"
#  (for the Berkeley equivalent) or "bison -y" (to get yacc compatible output 
#  from bison).  Note that /bin/yacc is sometimes a symlink to bison; on
#  such systems you need the -y flag.

YACC = bison -y

#  The GNU version of lex is called flex.  So the following variable is 
#  either "lex" or "flex", depending upon your system.

LEX = flex

# Uncomment & use these 3 lines to use Genbank in addition to FASTN:

# GBHOME=/home/macke/gensearch
# GBLIB=$(GBHOME)/libgb.a
# CFLAGS=-O2 -DUSE_GENBANK

