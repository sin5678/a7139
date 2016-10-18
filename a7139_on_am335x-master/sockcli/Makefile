TOP_DIR 		:= $(CURDIR)/../..
include $(TOP_DIR)/Env.make

INST_DIR  		:= $(INSTALL_ROOTFS_DIR)/sockcli
INST_BIN_D		:= $(INST_DIR)/usr/sbin

SKCLI_OBJEXT  	:= .skcli
SKCLI_SRCEXT	:= .c

MAIN_SRCS 		:= sockcli.c
SKCLI_HEAD		:= sockcli.h list.h
MAIN_OBJS 		:= $(basename $(MAIN_SRCS))
TEST_SRCS		:= sockcli_daemon_test.c sockcli_daemon_noplugin_test.c
TEST_OBJS 		:= $(basename $(TEST_SRCS))

CFLAGS  		+= -g -Werror -Wall

all: $(MAIN_OBJS) $(TEST_OBJS) SKCLI_ALL

$(MAIN_OBJS): $(MAIN_SRCS) $(SKCLI_HEAD)
	$(ECHO)echo CC $@
	$(ECHO)$(CC) $(CLFAGS) $(MAIN_SRCS) -o $@ -ldl

$(TEST_OBJS): $(TEST_SRCS)
	$(ECHO)echo HOSTCC $@
	$(ECHO)$(HOSTCC) $(CFLAGS) $(addsuffix .c, $@) -o $@

install: SKCLI_INSTALL
	$(ECHO)echo INSTALL $(MAIN_OBJS)
	$(ECHO)install -p -D -m 0755 $(MAIN_OBJS) $(INST_BIN_D)/$(MAIN_OBJS)
	$(ECHO)$(STRIP) $(INST_BIN_D)/$(MAIN_OBJS)

clean: SKCLI_CLEAN
	$(ECHO)echo clean $(MAIN_OBJS) $(TEST_OBJS)
	$(ECHO)rm -rf *~ $(MAIN_OBJS) $(TEST_OBJS)

include Makefile.sockcli

.PHONY: all clean $(MAIN_OBJS) $(TEST_OBJS) SKCLI_ALL SKCLI_INSTALL SKCLI_CLEAN

