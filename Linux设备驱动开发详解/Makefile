#内核module的路径，如果是自己下载的kernel源码，需要直接指定路径
KVERS = $(shell uname -r)
# Kernel modules
obj-m := $(OBJ).o
# modulename-objs := file1.o file2.o  #多文件时指定该参数单文件不用加

# Specify flags for the module compilation.
#EXTRA_CFLAGS=-g -O0 包含调试信息

build: kernel_modules
kernel_modules:
	make -C /lib/modules/$(KVERS)/build M=$(CURDIR) modules
clean:
	make -C /lib/modules/$(KVERS)/build M=$(CURDIR) clean
