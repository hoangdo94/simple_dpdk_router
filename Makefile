include $(RTE_SDK)/mk/rte.vars.mk

ifeq ($(CONFIG_RTE_LIBRTE_PIPELINE),y)

#
# library name
#
APP = simple_router

CFLAGS += -O3
CFLAGS += $(WERROR_FLAGS)

#
# all source are stored in SRCS-y
#
SRCS-y := main.c
SRCS-y += config.c
SRCS-y += init.c
SRCS-y += receive.c
SRCS-y += transmit.c
SRCS-y += forward.c

# this application needs libraries first
DEPDIRS-y += lib drivers

include $(RTE_SDK)/mk/rte.app.mk

endif
