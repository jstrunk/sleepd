CFLAGS=-g -Wall -DACPI_APM
BINS=sleepd sleepctl
PREFIX=/

all: $(BINS)

sleepd: sleepd.o acpi.o
	$(CC) -o sleepd sleepd.o acpi.o -lapm

clean:
	rm -f $(BINS) *.o

install: $(BINS)
	install -d $(PREFIX)/usr/sbin/ $(PREFIX)/usr/share/man/man8/ \
		$(PREFIX)/usr/bin/ $(PREFIX)/usr/share/man/man1/
	install -s sleepd $(PREFIX)/usr/sbin/
	install -m 0644 sleepd.8 $(PREFIX)/usr/share/man/man8/
	install -m 4755 -o root -g root -s sleepctl $(PREFIX)/usr/bin/
	install -m 0644 sleepctl.1 $(PREFIX)/usr/share/man/man1/
