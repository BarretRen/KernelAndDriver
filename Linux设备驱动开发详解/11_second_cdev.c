//秒字符设备
//在打开时初始化定时器并添加到内核的定时器链表中，每秒输出一次当前jiffies
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define SECOND_MAJOR 232

static int second_major = SECOND_MAJOR;
module_param(second_major, int, S_IRUGO);

struct second_dev
{
    struct cdev cdev;
    atomic_t counter;          //原子类型计数
    struct timer_list s_timer; //内核定时器
};
static struct second_dev *second_devp;

static void second_timer_handler(struct timer_list* arg)
{
    mod_timer(&(second_devp->s_timer), jiffies + HZ); //触发下一次定时
    atomic_inc(&(second_devp->counter));
    printk(KERN_INFO "current jiffies is %ld\n", jiffies);
}

static int second_open(struct inode *inode, struct file *filp)
{
    //打开设备时，初始化定时器，设置处理函数
    second_devp->s_timer.function = &second_timer_handler;
    second_devp->s_timer.expires = jiffies + HZ;
    add_timer(&(second_devp->s_timer)); //添加timer到内核

    atomic_set(&(second_devp->counter), 0);
    return 0;
}

static int second_release(struct inode *inode, struct file *filp)
{
    del_timer(&(second_devp->s_timer)); //释放设备时，从内核删除定时器
    return 0;
}

static ssize_t second_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
    //读操作
    int counter = atomic_read(&(second_devp->counter));
    if (put_user(counter, (int *)buf)) //复制counter到用户空间
        return -EFAULT;
    else
        return sizeof(unsigned int);
}

static const struct file_operations second_fops = 
{
    .owner = THIS_MODULE,
    .open = second_open,
    .release = second_release,
    .read = second_read
};

//驱动模块加载函数
static void second_setup_cdev(struct second_dev* dev, int index)
{
    int err, devno = MKDEV(second_major, index); //获得dev_t对象
    cdev_init(&(dev->cdev), &second_fops);//初始化设备
    dev->cdev.owner = THIS_MODULE;
    //注册设备
    err = cdev_add(&dev->cdev, devno, 1);
    if (err)
    {
        printk(KERN_NOTICE"Error %d adding second %d", err, index);
    }
}
static int __init second_init(void)
{
    int ret;
    dev_t devno = MKDEV(second_major, 0);
    //申请设备号
    if (second_major)
    {
        ret = register_chrdev_region(devno, 1, "second");
    }
    else
    {
        ret = alloc_chrdev_region(&devno, 0, 1, "second");
        second_major = MAJOR(devno);
    }

    if (ret < 0)
        return ret;

    second_devp = kzalloc(sizeof(struct second_dev), GFP_KERNEL);
    if (!second_devp)
    {
        //空间申请失败
        ret = -ENOMEM;
        goto fail_malloc;
    }

    second_setup_cdev(second_devp, 0);
    //初始化互斥量
    return 0;

fail_malloc:
    unregister_chrdev_region(devno, 1);
    return ret;
}
module_init(second_init);

//驱动模块卸载函数
static void __exit second_exit(void)
{
    cdev_del(&second_devp->cdev);//注销设备
    kfree(second_devp);
    unregister_chrdev_region(MKDEV(second_major, 0), 1);//释放设备号
}
module_exit(second_exit);

//模块声明
MODULE_AUTHOR("BARRET REN <barret.ren@outlook.com>");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("A driver for virtual second charactor device");
MODULE_ALIAS("second device driver");
