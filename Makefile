regsim: regulatory.h ieee80211.h util.c main.c
	gcc -Wall -o regsim util.c main.c

all: regsim

clean:
	rm -f regsim
