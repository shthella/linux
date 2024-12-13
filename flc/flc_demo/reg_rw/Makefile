
obj-m := sys_read_write.o

KDIR := /lib/modules/6.5.0-18-generic/build

PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

