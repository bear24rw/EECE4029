#include <linux/module.h>
#include <linux/kthread.h>
#include <acpi/acpi_drivers.h>

/* _BST 'state' entry bit fields */
#define STATE_DISCHARGING   (1 << 0)
#define STATE_CHARGING      (1 << 1)

/*
 * For each battery we keep a struct containing
 * various control method results as well as
 * the acpi_handle for the device
 */
struct acpi_battery {

    /* _BST */
    int state;
    int present_rate;
    int remaining_capacity;
    int present_voltage;

    /* _BIF */
    int power_unit;
    int design_capacity;
    int full_charge_capacity;
    int technology;
    int design_voltage;
    int design_capacity_warning;
    int design_capacity_low;
    int capacity_granularity_1;
    int capacity_granularity_2;
    char model_number[32];
    char serial_number[32];
    char type[32];
    char oem_info[32];

    /* used to detect changes */
    int last_state;
    int last_remaining_capacity;
    int percent;

    acpi_handle handle;
    struct acpi_buffer name;
    struct list_head list;
};

/* head of battery linked list */
struct acpi_battery acpi_battery_list;

/*
 * When extracting a control method object we need to
 * be able to lookup where to put the results in our
 * acpi_battery struct. The following tables are modified
 * from: drivers/acpi/battery.c
 */
struct acpi_offsets {
    size_t offset;      /* offset inside struct acpi_battery */
    u8 mode;            /* int or string? */
};

/* _BST offsets */
static struct acpi_offsets bst_offsets[] = {
    {offsetof(struct acpi_battery, state), 0},
    {offsetof(struct acpi_battery, present_rate), 0},
    {offsetof(struct acpi_battery, remaining_capacity), 0},
    {offsetof(struct acpi_battery, present_voltage), 0},
};

/* _BIF offsets */
static struct acpi_offsets bif_offsets[] = {
    {offsetof(struct acpi_battery, power_unit), 0},
    {offsetof(struct acpi_battery, design_capacity), 0},
    {offsetof(struct acpi_battery, full_charge_capacity), 0},
    {offsetof(struct acpi_battery, technology), 0},
    {offsetof(struct acpi_battery, design_voltage), 0},
    {offsetof(struct acpi_battery, design_capacity_warning), 0},
    {offsetof(struct acpi_battery, design_capacity_low), 0},
    {offsetof(struct acpi_battery, capacity_granularity_1), 0},
    {offsetof(struct acpi_battery, capacity_granularity_2), 0},
    {offsetof(struct acpi_battery, model_number), 1},
    {offsetof(struct acpi_battery, serial_number), 1},
    {offsetof(struct acpi_battery, type), 1},
    {offsetof(struct acpi_battery, oem_info), 1},
};

/* kthread id */
struct task_struct *ts;

/*
 * Unpacks the acpi_object into the battery struct
 * Taken from: drivers/acpi/battery.c
 */
static int extract_package(struct acpi_battery *battery,
        union acpi_object *package,
        struct acpi_offsets *offsets, int num)
{
    int i;
    union acpi_object *element;
    if (package->type != ACPI_TYPE_PACKAGE)
        return -EFAULT;
    for (i = 0; i < num; ++i) {
        if (package->package.count <= i)
            return -EFAULT;
        element = &package->package.elements[i];
        if (offsets[i].mode) {
            u8 *ptr = (u8 *)battery + offsets[i].offset;
            if (element->type == ACPI_TYPE_STRING ||
                    element->type == ACPI_TYPE_BUFFER)
                strncpy(ptr, element->string.pointer, 32);
            else if (element->type == ACPI_TYPE_INTEGER) {
                strncpy(ptr, (u8 *)&element->integer.value,
                        sizeof(u64));
                ptr[sizeof(u64)] = 0;
            } else
                *ptr = 0; /* don't have value */
        } else {
            int *x = (int *)((u8 *)battery + offsets[i].offset);
            *x = (element->type == ACPI_TYPE_INTEGER) ?
                element->integer.value : -1;
        }
    }
    return 0;
}

/*
 * Evaluates a control method and stores the result into the battery struct
 */
static int get_battery_control_method(struct acpi_battery *battery, const char *method, 
                                        struct acpi_offsets *offsets, size_t num_offsets)
{
    int result;
    acpi_status status;
    acpi_handle handle;
    struct acpi_buffer buffer = { ACPI_ALLOCATE_BUFFER, NULL };

    /* get method handle */
    status = acpi_get_handle(battery->handle, (acpi_string)method, &handle);

    if (ACPI_FAILURE(status)) {
        printk(KERN_ERR "battcheck: Error get handle\n");
        return -ENODEV;
    }

    /* get method object */
    status = acpi_evaluate_object(handle, NULL, NULL, &buffer);

    if (ACPI_FAILURE(status)) {
        printk(KERN_ERR "battcheck: Error evaluating\n");
        return -ENODEV;
    }

    /* extract the object into the battery struct */
    result = extract_package(battery, buffer.pointer, 
            offsets, num_offsets);

    kfree(buffer.pointer);

    return result;
}

/*
 * Monitor battery status periodically (default 1 second)
 * and report the following conditions:
 * 1) the battery begins to discharge
 * 2) when it has begun to charge or 
 * 3) when it has reached full charge 
 * 4) when the battery level has become low (only one report) 
 * 5) when the battery level becomes critical (report every 30 seconds)
 */
int check_thread(void *data)
{
    int result;
    float delay;

    /* allocate a battery struct to use in the for_each loop */
    struct acpi_battery *battery = NULL;
    battery = kzalloc(sizeof(struct acpi_battery), GFP_KERNEL);

    /* continue until the module_exit function tells us to stop */
    while(!kthread_should_stop())
    {
        /* delay for 1 HZ unless overwritten */
        delay = 1;

        /* loop through each battery */
        list_for_each_entry(battery, &acpi_battery_list.list, list) {

            /* get updated battery information */
            result = get_battery_control_method(battery, "_BST", bst_offsets, ARRAY_SIZE(bst_offsets));

            /* calculate percent remaining */
            battery->percent = ((float)battery->remaining_capacity / (float)battery->full_charge_capacity) * 100;

            /*
             * 1) if battery is currently discharging but it wasn't last time then
             * it has 'begun to discharge'
             */
            if ((battery->state & STATE_DISCHARGING) &&
                (battery->state != battery->last_state))
            {
                printk(KERN_INFO "battcheck: [%s] Battery has begun to discharge\n", (char*)(battery->name).pointer);
            }

            /*
             * 2) if battery is currently charging but it wasn't last time then
             * it has 'begun to charge'
             */
            if ((battery->state & STATE_CHARGING) &&
                (battery->state != battery->last_state))
            {
                printk(KERN_INFO "battcheck: [%s] Battery has begun to charge\n", (char*)(battery->name).pointer);
            }

            /*
             * 3) if battery was charging but now it is not and the capacity is
             * greater than the last 'full' capacity then we just 'reached full charge'
             */
            if ((battery->last_state & STATE_CHARGING) &&
                !(battery->state & STATE_CHARGING) &&
                (battery->remaining_capacity >= battery->full_charge_capacity))
            {
                printk(KERN_INFO "battcheck: [%s] Battery fully charged\n", (char*)(battery->name).pointer);
            }

            /*
             * 4) if current capacity is less than 'warning' but it was higher before
             * then the battery has 'become low'
             */
            if ((battery->remaining_capacity <= battery->design_capacity_warning) &&
                (battery->last_remaining_capacity > battery->design_capacity_warning))
            {
                printk(KERN_INFO "battcheck: [%s] Battery is low\n", (char*)(battery->name).pointer);
            }

            /*
             * 5) if current capacity is less than 'low' it has 'become crital'
             * report every 30 seconds
             */
            if (battery->remaining_capacity <= battery->design_capacity_low) {
                delay = 0.5;
                printk(KERN_INFO "battcheck: [%s] Battery is critical\n", (char*)(battery->name).pointer);
            }

            printk(KERN_INFO "battcheck: [%s] state: %5d | remaining: %5d (%d%%) | full: %5d | warn: %5d | low: %5d\n",
                    (char*)(battery->name).pointer,
                    battery->state,
                    battery->remaining_capacity,
                    battery->percent,
                    battery->full_charge_capacity,
                    battery->design_capacity_warning,
                    battery->design_capacity_low
                  );

            /* keep track of the last state so we can detect changes */
            battery->last_state = battery->state;
            battery->last_remaining_capacity = battery->remaining_capacity;
        }

        /* delay */
        set_current_state(TASK_INTERRUPTIBLE);
        schedule_timeout(delay*HZ);
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

    struct acpi_device *device;
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

        /* get device object for this handle */
        acpi_bus_get_device(chandle, &device);

        /* get the current status of the device */
        if (acpi_bus_get_status(device)) {
            printk(KERN_INFO "battcheck: Error evaluating _STA");
            return -ENODEV;
        }

        /* we only want to check batteries that are present */
        if (device->status.battery_present) {

            /* allocate and zero out memory for the new battery */
            battery = kzalloc(sizeof(struct acpi_battery), GFP_KERNEL);

            if (!battery) return -ENOMEM;

            /* set the handle for this battery */
            battery->handle = chandle;

            /* get the battery name */
            battery->name.length = ACPI_ALLOCATE_BUFFER;
            battery->name.pointer = NULL;
            acpi_get_name(chandle, ACPI_SINGLE_NAME, &(battery->name));

            /* get the static information from _BIF */
            get_battery_control_method(battery, "_BIF", bif_offsets, ARRAY_SIZE(bif_offsets));

            /* add it to the list */
            INIT_LIST_HEAD(&battery->list);
            list_add_tail(&(battery->list), &(acpi_battery_list.list));

            printk(KERN_INFO "Adding battery: %s\n", (char*)(battery->name).pointer);
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
    INIT_LIST_HEAD(&acpi_battery_list.list);

    /* find all batteries and add them to the linked list */
    status = find_batteries();

    if (status < 0) {
        printk(KERN_INFO "battcheck: Module did not loaded successfully\n");
        return status;
    }

    /* start the thread that prints out battery status */
    ts = kthread_run(check_thread, NULL, "battcheck_thread");

    printk(KERN_INFO "battcheck: Module loaded successfully\n");

    return 0;
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
