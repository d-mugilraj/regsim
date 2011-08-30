regsim: regulatory.h ieee80211.h main.c
	gcc -Wall -o regsim main.c

all: regsim

clean:
	rm -f regsim
