/* Copyright (c) 2010: Michal Kottman */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
#include <acpi/acpi.h>
#include <linux/wait.h>
#include <linux/kthread.h>

MODULE_LICENSE("GPL");

#define BUFFER_SIZE 256

struct task_struct *ts;

extern struct proc_dir_entry *acpi_root_dir;
char result_buffer[BUFFER_SIZE];
u8 temporary_buffer[BUFFER_SIZE];

size_t get_avail_bytes(void) {
    return BUFFER_SIZE - strlen(result_buffer);
}

char *get_buffer_end(void) {
    return result_buffer + strlen(result_buffer);
}

/** Appends the contents of an acpi_object to the result buffer
  @param result: An acpi object holding result data
  @returns: 0 if the result could fully be saved, a higher value otherwise **/
int acpi_result_to_string(union acpi_object *result) {
    if (result->type == ACPI_TYPE_INTEGER) {
        snprintf(get_buffer_end(), get_avail_bytes(),
                "0x%x", (int)result->integer.value);
    } else if (result->type == ACPI_TYPE_STRING) {
        snprintf(get_buffer_end(), get_avail_bytes(),
                "\"%*s\"", result->string.length, result->string.pointer);
    } else if (result->type == ACPI_TYPE_BUFFER) {
        int i;
        // do not store more than data if it does not fit. The first element is
        // just 4 chars, but there is also two bytes from the curly brackets
        int show_values = min(result->buffer.length, get_avail_bytes() / 6);

        sprintf(get_buffer_end(), "{");
        for (i = 0; i < show_values; i++)
            sprintf(get_buffer_end(), i == 0 ? "0x%02x" : ", 0x%02x", 
                    result->buffer.pointer[i]);

        if (result->buffer.length > show_values) {
            // if data was truncated, show a trailing comma if there is space
            snprintf(get_buffer_end(), get_avail_bytes(), ",");
            return 1;
        } else {
            // in case show_values == 0, but the buffer is too small to hold
            // more values (i.e. the buffer cannot have anything more than "{")
            snprintf(get_buffer_end(), get_avail_bytes(), "}");
        }
    } else if (result->type == ACPI_TYPE_PACKAGE) {
        int i;
        sprintf(get_buffer_end(), "[");
        for (i=0; i < result->package.count; i++) {
            if (i > 0)
                snprintf(get_buffer_end(), get_avail_bytes(), ", ");

            // abort if there is no more space available
            if (!get_avail_bytes() || 
                    acpi_result_to_string(&result->package.elements[i]))
                return 1;
        }
        snprintf(get_buffer_end(), get_avail_bytes(), "]");
    } else {
        snprintf(get_buffer_end(), get_avail_bytes(),
                "Object type 0x%x\n", result->type);
    }

    // return 0 if there are still bytes available, 1 otherwise
    return !get_avail_bytes();
}

/**
  @param method   The full name of ACPI method to call
  @param argc     The number of parameters
  @param argv     A pre-allocated array of arguments of type acpi_object
  */
void do_battcheck(const char *method, int argc, union acpi_object *argv) {
    acpi_status status;
    acpi_handle handle;
    struct acpi_object_list arg;
    struct acpi_buffer buffer = { ACPI_ALLOCATE_BUFFER, NULL };

    printk(KERN_INFO "battcheck: Calling %s\n", method);

    // get the handle of the method, must be a fully qualified path
    status = acpi_get_handle(NULL, (acpi_string) method, &handle);

    if (ACPI_FAILURE(status)) {
        snprintf(result_buffer, BUFFER_SIZE, 
                "Error: %s", acpi_format_exception(status));
        printk(KERN_ERR "battcheck: Cannot get handle: %s\n", result_buffer);
        return;
    }

    // prepare parameters
    arg.count = argc;
    arg.pointer = argv;

    // call the method
    status = acpi_evaluate_object(handle, NULL, &arg, &buffer);
    if (ACPI_FAILURE(status)) {
        snprintf(result_buffer, BUFFER_SIZE, 
                "Error: %s", acpi_format_exception(status));
        printk(KERN_ERR "battcheck: Method call failed: %s\n", result_buffer);
        return;
    }

    // reset the result buffer
    *result_buffer = '\0';
    acpi_result_to_string(buffer.pointer);
    kfree(buffer.pointer);

    printk(KERN_INFO "battcheck: Call successful: %s\n", result_buffer);
}


/** procfs 'call' read callback.  
  Called when reading the content of /proc/battcheck
  Returns the last call status: "not called" when no call was 
  previously issued or "failed" if the call failed or "ok" if 
  the call succeeded. **/
int proc_read(char *page, char **s, off_t off, int c, int *eof, void *d) {
    int len = 0;

    if (off > 0) {
        *eof = 1;
        return 0;
    }

    len = strlen(result_buffer);
    memcpy(page, result_buffer, len + 1);
    strcpy(result_buffer, "not called");

    return len;
}

int thread(void *data)
{
    while(!kthread_should_stop())
    {
        do_battcheck("\\_SB_.BAT1._BST", 0, NULL);

        set_current_state(TASK_INTERRUPTIBLE);
        schedule_timeout(HZ);
    }

    return 1;
}

/** module initialization function */
int __init init_battcheck(void) {

    struct proc_dir_entry *proc_entry = create_proc_read_entry("battcheck", 0600, NULL, proc_read, NULL);

    if (proc_entry == NULL) {
        printk(KERN_ERR "battcheck: Couldn't create proc entry\n");
        return -ENOMEM;
    }

    strcpy(result_buffer, "not called");

    printk(KERN_INFO "battcheck: Module loaded successfully\n");
    ts=kthread_run(thread, NULL, "battcheck-thread");

    return 0;
}

void __exit unload_battcheck(void) {
    remove_proc_entry("battcheck", NULL);
    kthread_stop(ts);
    printk(KERN_INFO "battcheck: Module unloaded successfully\n");
}

module_init(init_battcheck);
module_exit(unload_battcheck);
