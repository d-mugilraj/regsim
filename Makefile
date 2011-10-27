regsim: reglib.h ieee80211.h reg.h testreg.h reglib.c core.c reg.c util.c \
	drivers/acme.c
	gcc -Wall -I./ -Wall -o regsim \
	testreg.c \
	reglib.c core.c reg.c util.c \
	drivers/acme.c \

all: regsim

clean:
	rm -f regsim
