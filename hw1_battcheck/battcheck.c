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
    struct acpi_device *device;
    struct list_head list;
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

    status = acpi_evaluate_object(battery->device->handle, "_BST", NULL, &buffer);

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

            printk(KERN_ERR "battcheck: [BAT%d] %5d | %5d | %5d | %5d\n",
                    battery->number,
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


static int acpi_battery_add(struct acpi_device *device)
{
    printk(KERN_INFO "battcheck: Adding battery\n");

    static int battery_count = 0;
    struct acpi_battery *battery = NULL;
    acpi_handle handle;
    acpi_status status = 0;

    if (!device) return -EINVAL;

    battery = kzalloc(sizeof(struct acpi_battery), GFP_KERNEL);

    if (!battery) return -ENOMEM;

    battery->device = device;
    battery->number = battery_count;

    status = acpi_get_handle(battery->device->handle, "_BST", &handle);

	if (ACPI_FAILURE(status)) {
        printk(KERN_ERR "battcheck: Cannot get handle: %s\n", acpi_format_exception(status));
        return 1;
    }

    INIT_LIST_HEAD(&battery->list);
    list_add_tail(&(battery->list), &(acpi_battery_list.list));


    battery_count++;

    return 0;
}

static int acpi_battery_remove(struct acpi_device *device, int type)
{
    printk(KERN_INFO "battcheck: battery removed\n");

	struct acpi_battery *battery = NULL;

	if (!device || !acpi_driver_data(device))
		return -EINVAL;

	battery = acpi_driver_data(device);
	kfree(battery);
	return 0;
}

static struct acpi_driver battcheck_driver = {
    .name = "battcheck",
    .class = "battery",
    .ids = battery_device_ids,
	.flags = ACPI_DRIVER_ALL_NOTIFY_EVENTS,
    .ops = {
        .add = acpi_battery_add,
        .remove = acpi_battery_remove,
    },
};

static void __init init_battcheck_async(void *unused, async_cookie_t cookie)
{
    if (acpi_bus_register_driver(&battcheck_driver) < 0)
        printk(KERN_INFO "battcheck: Failed to register acpi driver\n");

    return;
}

static int __init init_battcheck(void) 
{
    async_schedule(init_battcheck_async, NULL);

    INIT_LIST_HEAD(&acpi_battery_list.list); // LIST_HEAD(acpi_battery_list)

    ts = kthread_run(thread, NULL, "battcheck_thread");

    printk(KERN_INFO "battcheck: Module loaded successfully\n");
    return 0;
}

static void __exit unload_battcheck(void) 
{
    struct acpi_battery *battery, *tmp;

    kthread_stop(ts);
    acpi_bus_unregister_driver(&battcheck_driver);

    list_for_each_entry_safe(battery, tmp, &acpi_battery_list.list, list) {
        printk(KERN_INFO "freeing node %d\n", battery->number);
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
