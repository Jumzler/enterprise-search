include ../../mk/setup.mk

CFLAGS+=	$(PERL_EMBED_INC) 
#CFLAGS+=	-D NO_64_BIT_DIV -D__USE_BSD -std=c99 -D_REENTRANT -D_GNU_SOURCE -fno-strict-aliasing -pipe -Wdeclaration-after-statement -I/usr/local/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -I/usr/include/gdbm -D_REENTRANT -D_GNU_SOURCE -I/usr/local/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -I/usr/include/gdbm -g -DATTRIBUTES -I/home/eirik/.root/include -D_FILE_OFFSET_BITS=64 -O2 -DIIACL -DWITH_OPENLDAP -BLACK_BOX  -I/usr/include/db4 -lpthread -BLACK_BOX -DIIACL -DWITH_CONFIG


LIB=	libperlembed.a
OBJS=	perlxsi.o perlembed.o

include ../../mk/lib.mk
