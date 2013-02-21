obj-m += vmm_dev.o
vmm_dev-objs += buddy.o vmm.o

KVERSION := $(shell uname -r)
KDIR := /lib/modules/$(KVERSION)/build
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f buddy_test vmm_test

install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install

buddy_test: buddy_test.c buddy.c buddy.h
	gcc $^ -Wall -g -o $@ -DNONKERNEL

vmm_test: vmm_test.c
	gcc $^ -Wall -g -o $@
