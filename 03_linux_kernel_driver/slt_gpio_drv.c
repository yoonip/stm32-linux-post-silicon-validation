#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/gpio.h>

#define DEVICE_NAME "slt_gpio"
#define CLASS_NAME  "slt_gpio_class"
// Raspberry Pi 4 (BCM2711) Debian 13 커널 글로벌 GPIO 번호 (GPIO17 = base 512 + 17 = 529)
#define TARGET_GPIO 529

static dev_t dev_num;
static struct cdev slt_cdev;
static struct class *slt_class;
static int gpio_state = 0;

static ssize_t dev_read(struct file *filep, char __user *buffer, size_t len, loff_t *offset) {
    char status_msg[64];
    int msg_len;
    if (*offset > 0) return 0;
    gpio_state = gpio_get_value(TARGET_GPIO);
    msg_len = snprintf(status_msg, sizeof(status_msg), "KERNEL_GPIO17_STATE=%d (Voltage: %s)\n",
                       gpio_state, gpio_state ? "3.3V HIGH" : "0V LOW");
    if (copy_to_user(buffer, status_msg, msg_len)) return -EFAULT;
    *offset += msg_len;
    return msg_len;
}

static ssize_t dev_write(struct file *filep, const char __user *buffer, size_t len, loff_t *offset) {
    char cmd;
    if (len < 1) return -EINVAL;
    if (copy_from_user(&cmd, buffer, 1)) return -EFAULT;

    if (cmd == '1') {
        gpio_set_value(TARGET_GPIO, 1);
        pr_info("[SLT_GPIO_KDRV] Hardware Pin GPIO17 set to HIGH (1)\n");
    } else if (cmd == '0') {
        gpio_set_value(TARGET_GPIO, 0);
        pr_info("[SLT_GPIO_KDRV] Hardware Pin GPIO17 set to LOW (0)\n");
    }
    return len;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read  = dev_read,
    .write = dev_write,
};

static int __init slt_gpio_init(void) {
    int ret;
    ret = gpio_request(TARGET_GPIO, "slt_hw_pin");
    if (ret) {
        pr_err("[SLT_GPIO_KDRV] Failed to request GPIO %d (err=%d)\n", TARGET_GPIO, ret);
        return ret;
    }
    gpio_direction_output(TARGET_GPIO, 0);

    alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    cdev_init(&slt_cdev, &fops);
    cdev_add(&slt_cdev, dev_num, 1);
    slt_class = class_create(CLASS_NAME);
    device_create(slt_class, NULL, dev_num, NULL, DEVICE_NAME);

    pr_info("[SLT_GPIO_KDRV] Hardware GPIO Kernel Driver Loaded!\n");
    return 0;
}

static void __exit slt_gpio_exit(void) {
    gpio_set_value(TARGET_GPIO, 0);
    gpio_free(TARGET_GPIO);
    device_destroy(slt_class, dev_num);
    class_destroy(slt_class);
    cdev_del(&slt_cdev);
    unregister_chrdev_region(dev_num, 1);
    pr_info("[SLT_GPIO_KDRV] Hardware GPIO Kernel Driver Unloaded!\n");
}

module_init(slt_gpio_init);
module_exit(slt_gpio_exit);
MODULE_LICENSE("GPL");
