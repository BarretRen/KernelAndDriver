/*
 * chardev.c: create a read-only char device that says how many times
 * you have read from the dev file
 */
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/poll.h>

#define DEVICE_NAME "chardev" //dev name as it appears in /proc/devices
#define BUF_LEN 80

//函数声明
static int device_open(struct inode*, struct file*);
static int device_release(struct inode*, struct file*);
static ssize_t device_read(struct file*, char __user*, size_t, loff_t*);
static ssize_t device_write(struct file*, const char __user*, size_t, loff_t*);

//static全局变量
static int major;
static atomic_t already_open = ATOMIC_INIT(0);//原子类型,避免竞争
static char msg[BUF_LEN];
static char* msg_ptr;//代表chardev设备中的内容
static struct class *cls;

//定义file_operations
static struct file_operations chardev_fops = {
    read:device_read,
    write:device_write,
    open:device_open,
    release:device_release,
};

//模块加载函数
static int __init chardev_init(void)
{
    major = register_chrdev(0, DEVICE_NAME, &chardev_fops);
    if (major < 0) {
        pr_alert("Registering char device failed with %d\n", major);
        return major;
    }
    pr_info("I was assigned major number %d.\n", major);
    //创建设备文件
    cls = class_create(THIS_MODULE, DEVICE_NAME);
    device_create(cls, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
    pr_info("Device created on /dev/%s\n", DEVICE_NAME);
    
    return 0;
}
//模块卸载函数
static void __exit chardev_exit(void)
{
    //删除设备文件
    device_destroy(cls, MKDEV(major, 0));
    class_destroy(cls);

    unregister_chrdev(major, DEVICE_NAME);
}

//fops各个函数的实现
static int device_open(struct inode *inode, struct file *file)
{
    static int counter = 0;//静态变量,第一次打开文件初始化为0,再次打开自增
    if (atomic_cmpxchg(&already_open, 0, 1))
        return -EBUSY;

    sprintf(msg, "I already told you %d times Hello world!\n", counter++);
    msg_ptr = msg;
    try_module_get(THIS_MODULE);//增加模块使用计数
    return 0;
}

static int device_release(struct inode *inode, struct file *file)
{
    atomic_set(&already_open, 0); //设置原子值为0,可以再次打开设备`
    module_put(THIS_MODULE);//减少模块使用计数
    return 0;
}

static ssize_t device_read(struct file *filp,
                           char __user *buffer, /* buffer to fill with data */
                           size_t length,
                           loff_t *offset)
{
    /* Number of bytes actually written to the buffer */
    int bytes_read = 0;
    /* If we are at the end of message, return 0 signifying end of file. */
    if (*msg_ptr == 0)
        return 0;

    /* Actually put the data into the buffer */
    while (length && *msg_ptr) {
        /* The buffer is in the user data segment, not the kernel
         * segment so "*" assignment won't work. We have to use
         * put_user which copies data from the kernel data segment to
         * the user data segment.
         */
        put_user(*(msg_ptr++), buffer++);
        length--;
        bytes_read++;
    }
    /* Most read functions return the number of bytes put into the buffer. */
    return bytes_read;
}

static ssize_t device_write(struct file *filp, const char __user *buff,
                            size_t len, loff_t *off)
{
    pr_alert("Sorry, this operation is not supported.\n");
    return -EINVAL;
}

module_init(chardev_init);
module_exit(chardev_exit);

MODULE_LICENSE("GPL");

