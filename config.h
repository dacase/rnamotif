#  Edit the configuration variables below to match your system:

CC= cc
YACC = yacc
LEX = lex

# Uncomment & use these 3 lines to use Genbank in addition to FASTN:
#GBHOME=		/home/macke/gensearch
#GBLIB=		$(GBHOME)/libgb.a
#CFLAGS=		-O2 -DUSE_GENBANK $(LOCAL_CFLAGS)

# Uncomment & use this line for FASTN only
CFLAGS= -O2
