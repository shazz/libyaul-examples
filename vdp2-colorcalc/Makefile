ifeq ($(strip $(INSTALL_ROOT)),)
  $(error Undefined INSTALL_ROOT (install root directory))
endif

include $(INSTALL_ROOT)/share/yaul/pre.common.mk

PROJECT:= vdp2-colorcalc

IP_VERSION:= V1.000
IP_RELEASE_DATE:= 20160101
IP_AREAS:= JTUBKAEL
IP_PERIPHERALS:= JAMKST
IP_TITLE:= vdp2-colorcalc
IP_MASTER_STACK_ADDR:= 0x06100000
IP_SLAVE_STACK_ADDR:= 0x00000000
IP_1ST_READ_ADDR:= 0x06004000

LIBRARIES:= langam
OBJECTS:= vdp2-colorcalc.o

include $(INSTALL_ROOT)/share/yaul/post.common.mk
