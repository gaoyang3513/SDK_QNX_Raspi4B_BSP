ifndef QCONFIG
QCONFIG=qconfig.mk
endif
include $(QCONFIG)
include $(MKFILES_ROOT)/qmacros.mk

NAME=app_th

USE_INSTALL_ROOT  = 1
INSTALL_ROOT_nto  = $(PROJECT_ROOT)/../../../../install
INSTALLDIR        = sbin

#This has to be included last
include $(MKFILES_ROOT)/qtargets.mk
