#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>

#define DEVICE_NAME "slt_kdrv"
#define CLASS_NAME  "slt_class"

static dev_t dev_num;
static struct cdev slt_cdev;
static struct class *slt_class;
static char kernel_buf[128] = "Hello from Raspberry Pi Linux Kernel Space!\n";
static size_t buf_len = 44;

static int dev_open(struct inode *inodep, struct file *filep) {
    pr_info("[SLT_KDRV] Device opened by user process\n");
    return 0;
}

static int dev_release(struct inode *inodep, struct file *filep) {
    pr_info("[SLT_KDRV] Device closed\n");
    return 0;
}

static ssize_t dev_read(struct file *filep, char __user *buffer, size_t len, loff_t *offset) {
    if (*offset >= buf_len) return 0;
    if (len > buf_len - *offset) len = buf_len - *offset;
    if (copy_to_user(buffer, kernel_buf + *offset, len)) return -EFAULT;
    *offset += len;
    pr_info("[SLT_KDRV] Sent %zu bytes to user space\n", len);
    return len;
}

static ssize_t dev_write(struct file *filep, const char __user *buffer, size_t len, loff_t *offset) {
    if (len > sizeof(kernel_buf) - 1) len = sizeof(kernel_buf) - 1;
    if (copy_from_user(kernel_buf, buffer, len)) return -EFAULT;
    kernel_buf[len] = '\0';
    buf_len = len;
    pr_info("[SLT_KDRV] Received from user space: %s\n", kernel_buf);
    return len;
}

static struct file_operations fops = {
    .owner   = THIS_MODULE,
    .open    = dev_open,
    .read    = dev_read,
    .write   = dev_write,
    .release = dev_release,
};

static int __init slt_kdrv_init(void) {
    alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    cdev_init(&slt_cdev, &fops);
    cdev_add(&slt_cdev, dev_num, 1);
    slt_class = class_create(CLASS_NAME);
    device_create(slt_class, NULL, dev_num, NULL, DEVICE_NAME);
    pr_info("[SLT_KDRV] Kernel Driver Loaded! Major=%d, Minor=%d\n", MAJOR(dev_num), MINOR(dev_num));
    return 0;
}

static void __exit slt_kdrv_exit(void) {
    device_destroy(slt_class, dev_num);
    class_destroy(slt_class);
    cdev_del(&slt_cdev);
    unregister_chrdev_region(dev_num, 1);
    pr_info("[SLT_KDRV] Kernel Driver Unloaded!\n");
}

module_init(slt_kdrv_init);
module_exit(slt_kdrv_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("PostSiliconValidation");
MODULE_DESCRIPTION("Custom Linux Character Device Driver");
