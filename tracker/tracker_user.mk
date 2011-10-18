#
# User makefile.
# Edit this file to change compiler options and related stuff.
#

# Programmer interface configuration, see http://dev.bertos.org/wiki/ProgrammerInterface for help
tracker_PROGRAMMER_TYPE = none
tracker_PROGRAMMER_PORT = none

# Files included by the user.
tracker_USER_CSRC = \
	$(tracker_SRC_PATH)/main.c \
	$(tracker_SRC_PATH)/ad.c \
	$(tracker_SRC_PATH)/da.c \
	$(tracker_SRC_PATH)/stm32f10x_adc.c \
	$(tracker_SRC_PATH)/stm32f10x_tim.c \
	$(tracker_SRC_PATH)/nmea.c \
	$(tracker_SRC_PATH)/sb.c \
	#$(tracker_SRC_PATH)/b91.c \
	#$(tracker_SRC_PATH)/stm32f10x_rcc.c \
	#

# Files included by the user.
tracker_USER_PCSRC = \
	#

# Files included by the user.
tracker_USER_CPPASRC = \
	#

# Files included by the user.
tracker_USER_CXXSRC = \
	#

# Files included by the user.
tracker_USER_ASRC = \
	#

# Flags included by the user.
tracker_USER_LDFLAGS = \
	#

# Flags included by the user.
tracker_USER_CPPAFLAGS = \
	#

# Flags included by the user.
tracker_USER_CPPFLAGS = \
	-fno-strict-aliasing \
	-mfix-cortex-m3-ldrd \
	-fwrapv \
	-DSTM32F10X_MD_VL \
	-Wno-strict-prototypes \
	#
