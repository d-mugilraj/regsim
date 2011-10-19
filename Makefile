regsim: regulatory.h ieee80211.h core.c reg.c util.c
	gcc -Wall -I./ -Wall -o regsim core.c reg.c util.c

all: regsim

clean:
	rm -f regsim
