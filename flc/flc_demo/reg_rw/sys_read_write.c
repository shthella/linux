#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/io.h>

#define DRIVER_NAME "reg_rw"
#define BASE_ADDR 0x10000000  // Define as needed

static struct kobject *kobj;
static unsigned long reg_addr;
static int op;       // 0 for read, 1 for write
static int size;     
static u32 reg_value; 

// `reg` node - setting register address
static ssize_t reg_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    return sprintf(buf, "0x%lx\n", reg_addr);
}

static ssize_t reg_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
    if (kstrtoul(buf, 16, &reg_addr)) {
        pr_err("Invalid register address\n");
        return -EINVAL;
    }
    return count;
}

static struct kobj_attribute reg_attr = __ATTR(reg, 0660, reg_show, reg_store);

// `op` node - 0 for read, 1 for write
static ssize_t op_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    return sprintf(buf, "%d\n", op);
}

static ssize_t op_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
    if (kstrtoint(buf, 10, &op) || (op != 0 && op != 1)) {
        pr_err("Invalid operation\n");
        return -EINVAL;
    }
    return count;
}

static struct kobj_attribute op_attr = __ATTR(op, 0660, op_show, op_store);

// `size` node - set size of access
static ssize_t size_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    return sprintf(buf, "%d\n", size);
}

static ssize_t size_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
    if (kstrtoint(buf, 10, &size) || (size != 1 && size != 2 && size != 4)) {
        pr_err("Invalid size\n");
        return -EINVAL;
    }
    return count;
}

static struct kobj_attribute size_attr = __ATTR(size, 0660, size_show, size_store);

// `data` node - performs read or write based on op and reg_addr
static ssize_t data_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    void __iomem *addr;

    if (op != 0) {
        pr_err("Set op to 0 for read\n");
        return -EINVAL;
    }

    addr = ioremap(reg_addr, size);
    if (!addr) {
        pr_err("Failed to map register address\n");
        return -EFAULT;
    }

    if (size == 1)
        reg_value = ioread8(addr);
    else if (size == 2)
        reg_value = ioread16(addr);
    else
        reg_value = ioread32(addr);

    iounmap(addr);
    return sprintf(buf, "0x%x\n", reg_value);
}

static ssize_t data_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
    void __iomem *addr;

    if (op != 1) {
        pr_err("Set op to 1 for write\n");
        return -EINVAL;
    }

    if (kstrtouint(buf, 16, &reg_value)) {
        pr_err("Invalid data format\n");
        return -EINVAL;
    }

    addr = ioremap(reg_addr, size);
    if (!addr) {
        pr_err("Failed to map register address\n");
        return -EFAULT;
    }

    if (size == 1)
        iowrite8((u8)reg_value, addr);
    else if (size == 2)
        iowrite16((u16)reg_value, addr);
    else
        iowrite32(reg_value, addr);

    iounmap(addr);
    return count;
}

static struct kobj_attribute data_attr = __ATTR(data, 0660, data_show, data_store);

// Initialize the driver and create sysfs entries
static int __init reg_rw_init(void) {
    int ret;

    kobj = kobject_create_and_add(DRIVER_NAME, kernel_kobj);
    if (!kobj)
        return -ENOMEM;

    ret = sysfs_create_file(kobj, &reg_attr.attr);
    if (ret) goto reg_error;

    ret = sysfs_create_file(kobj, &op_attr.attr);
    if (ret) goto op_error;

    ret = sysfs_create_file(kobj, &size_attr.attr);
    if (ret) goto size_error;

    ret = sysfs_create_file(kobj, &data_attr.attr);
    if (ret) goto data_error;

    pr_info("Register R/W module loaded\n");
    return 0;

data_error:
    sysfs_remove_file(kobj, &size_attr.attr);
size_error:
    sysfs_remove_file(kobj, &op_attr.attr);
op_error:
    sysfs_remove_file(kobj, &reg_attr.attr);
reg_error:
    kobject_put(kobj);
    return ret;
}

// Cleanup the driver and remove sysfs entries
static void __exit reg_rw_exit(void) {
    sysfs_remove_file(kobj, &data_attr.attr);
    sysfs_remove_file(kobj, &size_attr.attr);
    sysfs_remove_file(kobj, &op_attr.attr);
    sysfs_remove_file(kobj, &reg_attr.attr);
    kobject_put(kobj);
    pr_info("Register R/W module unloaded\n");
}

module_init(reg_rw_init);
module_exit(reg_rw_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("FLC");
MODULE_VERSION("1.8");
MODULE_DESCRIPTION("Register Read/Write Kernel Driver");

