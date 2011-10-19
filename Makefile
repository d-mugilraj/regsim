regsim: regulatory.h ieee80211.h util.c reg.c
	gcc -Wall -o regsim util.c reg.c

all: regsim

clean:
	rm -f regsim
