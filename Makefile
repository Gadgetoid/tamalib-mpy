# Location of top-level MicroPython directory
MPY_DIR = ../../..

# Name of module
MOD = tamalib

# Source files (.c or .py)
SRC = main.c lib/tamalib.c lib/cpu.c lib/hw.c lib1funcs.S

# Architecture to build for (x86, x64, armv7m, xtensa, xtensawin, rv32imc)
ARCH = armv6m

CFLAGS = -Wno-discarded-qualifiers -DL_thumb1_case_uqi -DL_thumb1_case_shi -DL_thumb1_case_sqi -DL_udivsi3 -DL_dvmd_tls

# Include to get the rules for compiling and linking the module
include $(MPY_DIR)/py/dynruntime.mk
