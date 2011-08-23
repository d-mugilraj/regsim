regsim: regulatory.h ieee80211.h main.c
	gcc -o regsim main.c

all: regsim

clean:
	rm -f regsim
