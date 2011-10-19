regsim: regulatory.h ieee80211.h core.c reg.c util.c \
	drivers/acme.c
	gcc -Wall -I./ -Wall -o regsim core.c reg.c util.c \
	drivers/acme.c

all: regsim

clean:
	rm -f regsim
