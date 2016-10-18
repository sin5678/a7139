TOP_DIR 	:= $(CURDIR)/../..

include $(TOP_DIR)/Env.make


RFP_TARGET	:= rfrepeater
RFC_TARGET	:= rfcli
RFP_SHELL	:= rfrepeater.sh
RFP_S_SHELL	:= S90rfrepeater
VRFP_TEST	:= rf433_udp_test
ERFP_TEST	:= rf433_se433_test
RD_TEST		:= rf433_read
TARGET 		:= $(RFP_TARGET) $(VRFP_TEST) $(ERFP_TEST) $(RD_TEST) $(RFC_TARGET) 
INST_DIR  	:= $(INSTALL_ROOTFS_DIR)/rf433
INST_ETC_D  := $(INST_DIR)/etc
INST_BIN_D	:= $(INST_DIR)/usr/sbin

RFP_OBJS	:= rfrepeater.o rf433lib.o common.o buffer.o
RFC_OBJS	:= rfcli.o common.o rf433lib.o buffer.o
VRFP_OBJS	:= rf433_udp_test.o rf433lib.o common.o buffer.o
ERFP_OBJS	:= rf433_se433_test.o rf433lib.o common.o buffer.o
RD_OBJS		:= rf433_read.o rf433lib.o common.o buffer.o
RF_HEADER	:= rfrepeater.h rf433lib.h rf433pkg.h common.h rfcli.h buffer.h list.h


CFLAGS  	+= -I$(FS_DIR)/nvram -I$(FS_DIR)/applog -I./ -I$(LINUXKERNEL_DIR) -g -Werror -Wall -DUSE_APPLOG $(CFLAGS_EXTEND)
LDFLAGS 	+= -L$(FS_DIR)/nvram -L$(FS_DIR)/applog -lpthread -lnvram -lapplog

all: $(TARGET) SKCLI_ALL


$(RFP_TARGET): $(RFP_OBJS) $(RF_HEADER)
	$(ECHO)echo LD $@
	$(ECHO)$(CC) $(LDFLAGS) -o $(RFP_TARGET) $(RFP_OBJS)

$(RFC_TARGET): $(RFC_OBJS) $(RF_HEADER)
	$(ECHO)echo LD $@
	$(ECHO)$(CC) $(LDFLAGS) -o $(RFC_TARGET) $(RFC_OBJS)

$(VRFP_TEST): $(VRFP_OBJS) $(RF_HEADER)
	$(ECHO)echo LD $@
	$(ECHO)$(CC) $(LDFLAGS) -o $@ $(VRFP_OBJS)

$(ERFP_TEST): $(ERFP_OBJS) $(RF_HEADER)
	$(ECHO)echo LD $@
	$(ECHO)$(CC) $(LDFLAGS) -o $@ $(ERFP_OBJS)

$(RD_TEST): $(RD_OBJS) $(RF_HEADER)
	$(ECHO)echo LD $@
	$(ECHO)$(CC) $(LDFLAGS) -o $@ $(RD_OBJS)

clean: SKCLI_CLEAN
	$(ECHO)echo clean $(TARGET)
	$(ECHO)rm -rf $(TARGET) *.o *.bak

install: SKCLI_INSTALL
	$(ECHO)echo INSTALL $(RFP_TARGET)
	$(ECHO)install -p -D -m 0755 $(RFP_TARGET) $(INST_BIN_D)/$(RFP_TARGET)
	$(ECHO)$(STRIP) $(INST_BIN_D)/$(RFP_TARGET)
	#$(ECHO)echo INSTALL $(RFC_TARGET)
	#$(ECHO)install -p -D -m 0755 $(RFC_TARGET) $(INST_BIN_D)/$(RFC_TARGET)
	#$(ECHO)$(STRIP) $(INST_BIN_D)/$(RFC_TARGET)
	$(ECHO)echo INSTALL $(RFP_SHELL)
	$(ECHO)install -p -D -m 0755 $(RFP_SHELL) $(INST_ETC_D)/init.d/$(RFP_TARGET)
	$(ECHO)echo INSTALL $(RFP_S_SHELL)
	$(ECHO)install -d $(INST_ETC_D)/rc5.d
	$(ECHO)ln -sf ../init.d/$(RFP_TARGET) $(INST_ETC_D)/rc5.d/$(RFP_S_SHELL)

%.o: %.c $(RF_HEADER)
	$(ECHO)echo CC $@
	$(ECHO)$(CC) $(CFLAGS) -c -o $@ $< 2>&1 > /dev/null

include Makefile.sockcli

.PHONY: all install clean release SKCLI_ALL SKCLI_INSTALL SKCLI_CLEAN
