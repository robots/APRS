#
# User makefile.
# Edit this file to change compiler options and related stuff.
#

# Programmer interface configuration, see http://dev.bertos.org/wiki/ProgrammerInterface for help
discovery_aprs_PROGRAMMER_TYPE = none
discovery_aprs_PROGRAMMER_PORT = none

# Files included by the user.
discovery_aprs_USER_CSRC = \
	$(discovery_aprs_SRC_PATH)/main.c \
	$(discovery_aprs_SRC_PATH)/ad.c \
	$(discovery_aprs_SRC_PATH)/da.c \
	$(discovery_aprs_SRC_PATH)/stm32f10x_adc.c \
	$(discovery_aprs_SRC_PATH)/stm32f10x_dac.c \
	$(discovery_aprs_SRC_PATH)/stm32f10x_tim.c \
	#

# Files included by the user.
discovery_aprs_USER_PCSRC = \
	#

# Files included by the user.
discovery_aprs_USER_CPPASRC = \
	#

# Files included by the user.
discovery_aprs_USER_CXXSRC = \
	#

# Files included by the user.
discovery_aprs_USER_ASRC = \
	#

# Flags included by the user.
discovery_aprs_USER_LDFLAGS = \
	#

# Flags included by the user.
discovery_aprs_USER_CPPAFLAGS = \
	#

# Flags included by the user.
discovery_aprs_USER_CPPFLAGS = \
	-fno-strict-aliasing \
	-mfix-cortex-m3-ldrd \
	-fwrapv \
	-DSTM32F10X_MD_VL \
	-Wno-strict-prototypes \
	#
