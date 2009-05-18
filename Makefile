CFLAGS		= -O2 -g -Wall -DACPI_APM -pthread
BINS		= sleepd sleepctl
PREFIX		= /
INSTALL_PROGRAM	= install
USE_HAL		= 1

# DEB_BUILD_OPTIONS suport, to control binary stripping.
ifeq (,$(findstring nostrip,$(DEB_BUILD_OPTIONS)))
INSTALL_PROGRAM += -s
endif

# And debug building.
ifneq (,$(findstring debug,$(DEB_BUILD_OPTIONS)))
CFLAGS += -g
endif

OBJS=sleepd.o acpi.o eventmonitor.o
LIBS=-lapm

all: $(BINS)

ifdef USE_HAL
LIBS+=$(shell pkg-config --libs hal)
OBJS+=simplehal.o
CFLAGS+=-DHAL
simplehal.o: simplehal.c
	$(CC) $(CFLAGS) $(shell pkg-config --cflags hal) -c simplehal.c -o simplehal.o
endif

sleepd: $(OBJS)
	$(CC) $(CFLAGS) -o sleepd $(OBJS) $(LIBS)

clean:
	rm -f $(BINS) *.o

install: $(BINS)
	install -d $(PREFIX)/usr/sbin/ $(PREFIX)/usr/share/man/man8/ \
		$(PREFIX)/usr/bin/ $(PREFIX)/usr/share/man/man1/
	$(INSTALL_PROGRAM) sleepd $(PREFIX)/usr/sbin/
	install -m 0644 sleepd.8 $(PREFIX)/usr/share/man/man8/
	$(INSTALL_PROGRAM) -m 4755 -o root -g root sleepctl $(PREFIX)/usr/bin/
	install -m 0644 sleepctl.1 $(PREFIX)/usr/share/man/man1/
