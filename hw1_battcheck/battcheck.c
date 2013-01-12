#include <linux/module.h>
#include <linux/kthread.h>
#include <acpi/acpi_drivers.h>

struct acpi_battery {
    int state;
    int present_rate;
    int remaining_capacity;
    int present_voltage;
    struct acpi_buffer name;
    struct list_head list;
    acpi_handle handle;
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
 * Unpacks the acpi_object into the battery struct
 * Modified from: drivers/acpi/battery.c
 */
static int extract_package(struct acpi_battery *battery,
        union acpi_object *package,
        size_t *offsets, int num)
{
    int i;
    int *x;
    union acpi_object *element;
    if (package->type != ACPI_TYPE_PACKAGE)
        return -EFAULT;
    for (i = 0; i < num; ++i) {
        if (package->package.count <= i)
            return -EFAULT;
        element = &package->package.elements[i];
        x = (int *)((u8 *)battery + offsets[i]);
        *x = (element->type == ACPI_TYPE_INTEGER) ?
            element->integer.value : -1;
    }
    return 0;
}

/*
 * Gets updated battery status (_BST) information
 */
static int get_battery_status(struct acpi_battery *battery)
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

int check_thread(void *data)
{
    int result;

    /* allocate a battery struct to use in the for_each loop */
    struct acpi_battery *battery = NULL;
    battery = kzalloc(sizeof(struct acpi_battery), GFP_KERNEL);

    /* continue until the module_exit function tells us to stop */
    while(!kthread_should_stop())
    {
        /* loop through each battery */
        list_for_each_entry(battery, &acpi_battery_list.list, list) {

            /* get updated battery information */
            result = get_battery_status(battery);

            printk(KERN_ERR "battcheck: [%s] %5d | %5d | %5d | %5d\n",
                    (char*)(battery->name).pointer,
                    battery->state,
                    battery->present_rate,
                    battery->remaining_capacity,
                    battery->present_voltage
                  );

        }

        /* delay */
        set_current_state(TASK_INTERRUPTIBLE);
        schedule_timeout(HZ);
    }

    return 0;
}

/*
 * Walk through the ACPI device tree starting at the system bus (_SB_). 
 * For each device check whether it has a battery status (_BST) entry.
 * If it does allocate a new battery struct and add it to the 
 * battery_list linked list
 */
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
        printk(KERN_ERR "battcheck: Cannot get system bus handle: %s\n", 
                acpi_format_exception(status));
        return -1;
    }

    /*
     * Walk through all devices (children) under the system bus (parent) 
     * and look for devices that have a battery status (_BST) entry
     */
    while(1) {

        /* get the next child under the parent node */
        status = acpi_get_next_object(ACPI_TYPE_DEVICE, phandle, chandle, &rethandle);

        /* if there are no more devices left stop searching */
        if (status == AE_NOT_FOUND || rethandle == NULL)
            break;

        /* current child is now the old child */
        chandle = rethandle;

        /* try to get a handle for _BST under this node */
        status = acpi_get_handle(chandle, "_BST", &rethandle);

        /* if we found a _BST entry add a new battery */
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

/*
 * Module entry point
 */
static int __init init_battcheck(void) 
{
    int status;

    /* set the head of our linked list to store the battery structs */
    INIT_LIST_HEAD(&acpi_battery_list.list); // LIST_HEAD(acpi_battery_list)

    /* find all batteries and add them to the linked list */
    status = find_batteries();

    /* start the thread that prints out battery status */
    ts = kthread_run(check_thread, NULL, "battcheck_thread");

    if (status < 0)
        printk(KERN_INFO "battcheck: Module did not loaded successfully\n");
    else
        printk(KERN_INFO "battcheck: Module loaded successfully\n");

    return status;
}

/*
 * Module exit point
 */
static void __exit exit_battcheck(void) 
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
module_exit(exit_battcheck);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Max Thrun");
MODULE_DESCRIPTION("Battery checker");
