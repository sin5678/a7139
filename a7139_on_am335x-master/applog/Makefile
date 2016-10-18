
TOP_DIR = $(shell /bin/pwd)/../../

include $(TOP_DIR)/Env.make

INSTALLROOTFSDIR = $(INSTALL_ROOTFS_DIR)/applog
INSTALLDATAFSDIR = $(INSTALL_DATAFS_DIR)/applog
SYSLOG_SCRIPT 	= syslogd.busybox
SYSLOG_CONF 	= syslog.conf.default

.PHONY: all install clean

CFLAGS  += -Werror -Wall -fPIC -g

all: libapplog.so

libapplog.so: applog.o
	$(LD) -fPIC -shared -o $@ $^

applog.o: applog.h applog.c
	$(CC) $(CFLAGS) -c $*.c

install:
	install -p -D -m 0755 libapplog.so $(INSTALLROOTFSDIR)/lib/libapplog.so
	$(STRIP) $(INSTALLROOTFSDIR)/lib/libapplog.so
	install -d $(INSTALLDATAFSDIR)/syslog
	install -p -D -m 0755 $(SYSLOG_SCRIPT) $(INSTALLROOTFSDIR)/etc/init.d/$(SYSLOG_SCRIPT)
	install -p -D -m 0644 $(SYSLOG_CONF) $(INSTALLROOTFSDIR)/etc/$(SYSLOG_CONF)

clean:
	rm -rf *.o libapplog.so
