#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/jiffies.h>
#include <linux/async.h>
#include <linux/suspend.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
#include <acpi/acpi.h>
#include <linux/wait.h>
#include <linux/kthread.h>
#include <linux/list.h>

#include <acpi/acpi_bus.h>
#include <acpi/acpi_drivers.h>
#include <linux/power_supply.h>

static const struct acpi_device_id battery_device_ids[] = {
	{"PNP0C0A", 0},
	{"", 0},
};

MODULE_DEVICE_TABLE(acpi, battery_device_ids);

ACPI_MODULE_NAME("battcheck");

struct acpi_battery {
    int number;
    int state;
    int present_rate;
    int remaining_capacity;
    int present_voltage;
    acpi_handle handle;
    struct acpi_device *device;
    struct list_head list;
    struct acpi_buffer name;
};

struct acpi_battery acpi_battery_list;

size_t info_offsets[] = {
    offsetof(struct acpi_battery, state),
    offsetof(struct acpi_battery, present_rate),
    offsetof(struct acpi_battery, remaining_capacity),
    offsetof(struct acpi_battery, present_voltage),
};

struct task_struct *ts;

/*
 * modified from: drivers/acpi/battery.c
 */
static int extract_package(struct acpi_battery *battery,
			   union acpi_object *package,
			   size_t *offsets, int num)
{
    int i;
	union acpi_object *element;
	if (package->type != ACPI_TYPE_PACKAGE)
		return -EFAULT;
	for (i = 0; i < num; ++i) {
		if (package->package.count <= i)
			return -EFAULT;
		element = &package->package.elements[i];
        int *x = (int *)((u8 *)battery + offsets[i]);
        *x = (element->type == ACPI_TYPE_INTEGER) ?
            element->integer.value : -1;
	}
	return 0;
}

static int acpi_battery_get_state(struct acpi_battery *battery)
{
    int result = 0;
    acpi_status status = 0;
    struct acpi_buffer buffer = { ACPI_ALLOCATE_BUFFER, NULL };

    status = acpi_evaluate_object(battery->handle, NULL, NULL, &buffer);

	if (ACPI_FAILURE(status)) {
        printk(KERN_ERR "battcheck: Error evaluating _BST");
		return -ENODEV;
	}

    result = extract_package(battery, buffer.pointer, 
            info_offsets, ARRAY_SIZE(info_offsets));

    kfree(buffer.pointer);

    return result;
}

int thread(void *data)
{
    struct acpi_battery *battery = NULL;
    battery = kzalloc(sizeof(struct acpi_battery), GFP_KERNEL);

    while(!kthread_should_stop())
    {
        list_for_each_entry(battery, &acpi_battery_list.list, list) {

            int result = 0;
            
            result = acpi_battery_get_state(battery);

            printk(KERN_ERR "battcheck: [%s] %5d | %5d | %5d | %5d\n",
                    (char*)(battery->name).pointer,
                    battery->state,
                    battery->present_rate,
                    battery->remaining_capacity,
                    battery->present_voltage
                  );

        }

        set_current_state(TASK_INTERRUPTIBLE);
        schedule_timeout(HZ);
    }

    return 0;
}

int find_batteries(void)
{
    struct acpi_battery *battery = NULL;

    acpi_handle phandle;
    acpi_handle chandle = NULL;
    acpi_handle rethandle;
    acpi_status status;

    /* get the handle for the system bus */
    status = acpi_get_handle(NULL, "\\_SB_", &phandle);

    if (ACPI_FAILURE(status)) {
        printk(KERN_ERR "battcheck: Cannot get handle: %s\n", acpi_format_exception(status));
        return -1;
    }

    /*
     * walk through all devices under the root node
     * and look for devices that have a battery
     * status (_BST) entry
     */
    while(1) {

        /* get the next child under the parent node */
        status = acpi_get_next_object(ACPI_TYPE_DEVICE,
                phandle, chandle, &rethandle);

        /* if there are no more devices left stop searching */
        if (status == AE_NOT_FOUND || rethandle == NULL)
            break;

        /* current child is now the old child */
        chandle = rethandle;

        /* try to get a handle for _BST under this node */
        status = acpi_get_handle(chandle, "_BST", &rethandle);
       
        /* add a new battery if we found a _BST entry*/
        if (ACPI_SUCCESS(status)) {

            /* allocate and zero out memory for the new battery */
            battery = kzalloc(sizeof(struct acpi_battery), GFP_KERNEL);

            if (!battery) return -ENOMEM;

            /* set the handle for this battery */
            battery->handle = rethandle;

            /* get the battery name */
            battery->name.length = ACPI_ALLOCATE_BUFFER;
            battery->name.pointer = NULL;
            acpi_get_name(chandle, ACPI_SINGLE_NAME, &(battery->name));

            /* add it to the list */
            INIT_LIST_HEAD(&battery->list);
            list_add_tail(&(battery->list), &(acpi_battery_list.list));

            printk(KERN_INFO "Found battery: %s\n", (char*)(battery->name).pointer);
        }
    }

    return 0;
}

static int __init init_battcheck(void) 
{
    /* set the head of our linked list to store the battery structs */
    INIT_LIST_HEAD(&acpi_battery_list.list); // LIST_HEAD(acpi_battery_list)

    /* find all batteries and add them to the linked list */
    find_batteries();

    /* start the thread that prints out battery status */
    ts = kthread_run(thread, NULL, "battcheck_thread");

    printk(KERN_INFO "battcheck: Module loaded successfully\n");

    return 0;
}

static void __exit unload_battcheck(void) 
{
    struct acpi_battery *battery, *tmp;

    /* stop the print thread */
    kthread_stop(ts);

    /* deallocate each battery struct in the linked list */
    list_for_each_entry_safe(battery, tmp, &acpi_battery_list.list, list) {
        printk(KERN_INFO "Freeing battery: %s\n", (char*)(battery->name).pointer);
        list_del(&battery->list);
        kfree(battery);
    }

    printk(KERN_INFO "battcheck: Module unloaded successfully\n");
}

module_init(init_battcheck);
module_exit(unload_battcheck);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Max Thrun");
MODULE_DESCRIPTION("Battery checker");
