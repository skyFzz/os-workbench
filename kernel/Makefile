# adjust number of cores in qemu.mk
NAME           := kernel
#SRCS           := framework/main.c $(shell find -L ./src/ -name "*.c")
#SRCS           := framework/stress_test/random_op.c $(shell find -L ./src/ -name "*.c")
SRCS           := framework/simple_test/one_page.c $(shell find -L ./src/ -name "*.c")
INC_PATH       := include/ framework/ src/

include $(AM_HOME)/Makefile
#include ../oslabs.mk
