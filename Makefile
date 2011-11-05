regsim: \
	include/os/spinlock.h \
	include/os/workqueue.h \
	c-hacks.h \
	reglib.h ieee80211.h reg.h \
	testreg.h \
	kernel/spinlock.c \
	kernel/workqueue.c \
	core.c \
	reglib.c reg.c \
	drivers/acme.c
	gcc -Wall -I./ -I./include/ -Wall -pthread \
	-o regsim \
	kernel/spinlock.c \
	kernel/workqueue.c \
	testreg.c \
	reglib.c core.c reg.c \
	drivers/acme.c

all: regsim

clean:
	rm -f regsim
