obj-m := vmm.o

KVERSION := $(shell uname -r)
KDIR := /lib/modules/$(KVERSION)/build
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm buddy_test

install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install

buddy_test: buddy_test.c buddy.c buddy.h
	gcc $^ -Wall -g -o $@ -DNONKERNEL
