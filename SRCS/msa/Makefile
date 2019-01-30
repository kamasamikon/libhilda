
LIBS += -Wl,-rpath=.

STATIC_LIBS += -lhilda
STATIC_LIBS += -lrt
STATIC_LIBS += -ldl

DYN_LIBS += -lgcc_s

LDFLAGS := ${LIBS} -Wl,-Bstatic ${STATIC_LIBS} -Wl,-Bdynamic ${DYN_LIBS}

all:
	# gcc msa.c  -g3 -I ../../inc -L ../core/ ${LDFLAGS} -o msa
	gcc msa.c  -g3 -I ../../inc -L ../core/ -l:libhilda.a -ldl -lpthread -lrt -o msa

clean:
	rm msa
