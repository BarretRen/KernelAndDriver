//在驱动8 globalfifo的基础上，实现async异步信号通知
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/ioctl.h>
#include <linux/wait.h>
#include <linux/sched/signal.h>
#include <linux/poll.h>

//定义宏
#define GLOBALFIFO_SIZE 0X1000
#define globalfifo_MAGIC 'g'
#define MEM_CLEAR _IO(globalfifo_MAGIC,0)
#define globalfifo_MAJOR 231 //默认主设备号

//添加模块参数，主设备号
static int globalfifo_major = globalfifo_MAJOR;
module_param(globalfifo_major, int, S_IRUGO);

//定义字符驱动结构体
struct globalfifo_dev
{
    struct cdev cdev;//系统字符设备结构体
    unsigned char mem[GLOBALFIFO_SIZE];//模拟设备占用的内存
    struct mutex mutex; //增加互斥量
    //添加阻塞队列
    wait_queue_head_t r_wait; //读队列
    wait_queue_head_t w_wait; //写队列
    unsigned int current_len; //当前mem中数据长度
    struct fasync_struct* async_queue;//异步结构体指针
};
struct globalfifo_dev* globalfifo_devp;//指针，指向申请的设备空间

//file_operation成员函数
static int globalfifo_fasync(int fd, struct file* filp, int mode)
{
    struct globalfifo_dev* dev = filp->private_data;
    return fasync_helper(fd, filp, mode, &(dev->async_queue));
}
static int globalfifo_open(struct inode* inode, struct file* filp)
{
    filp->private_data = globalfifo_devp;
    return 0;
}
static int globalfifo_release(struct inode* inode, struct file* filp)
{
    globalfifo_fasync(-1, filp, 0);//将文件filp从异步通知列表删除
    return 0;//释放文件，没什么特殊操作
}
static ssize_t globalfifo_read(struct file* filp, char __user * buf, size_t size, loff_t* ppos)
{
    int ret = 0;
    unsigned int count = size;
    //文件的私有数据一般指向设备结构体,在open函数中设置
    struct globalfifo_dev* dev = filp->private_data;
    //声明一个阻塞队列元素
    DECLARE_WAITQUEUE(wait, current);
    
    //获取互斥量
    mutex_lock(&(dev->mutex));
    //添加元素到读队列
    add_wait_queue(&(dev->r_wait), &wait);

    while(dev->current_len == 0)//等待有数据可读
    {
        if (filp->f_flags & O_NONBLOCK)
        {
            ret = -EAGAIN;//非阻塞模式，直接返回，不循环等待
            goto out;
        }
        //如果没有数据可读，则挂起读进程
        __set_current_state(TASK_INTERRUPTIBLE);//标记进程浅度睡眠，可抢占
        mutex_unlock(&(dev->mutex));//释放互斥量

        schedule();
        if (signal_pending(current))
        {
            ret = -ERESTARTSYS;
            goto out2;
        }

        mutex_lock(&(dev->mutex));
    }

    if (count > dev->current_len)
        count = dev->current_len;

    if (copy_to_user(buf, dev->mem, count))//复制内容到用户空间
    {
        ret = -EFAULT;
        goto out;
    }
    else
    {
        memcpy(dev->mem, dev->mem + count, dev->current_len - count);
        dev->current_len -= count;
        printk(KERN_INFO"read %u bytes, current_len:%d\n", count, dev->current_len);
        wake_up_interruptible(&(dev->w_wait));//读出了数据，可能有数据要写，唤醒写进程队列
        ret = count;
    }
out:
    mutex_unlock(&(dev->mutex));
out2:
    remove_wait_queue(&(dev->r_wait), &wait);
    set_current_state(TASK_RUNNING);

    return ret;
}
static ssize_t globalfifo_write(struct file* filp, const char __user * buf, size_t size, loff_t* ppos)
{
    unsigned int count = size;
    int ret = 0;
    struct globalfifo_dev* dev = filp->private_data;
    //声明一个阻塞队列元素
    DECLARE_WAITQUEUE(wait, current);

    //获取互斥量
    mutex_lock(&(dev->mutex));
    add_wait_queue(&(dev->w_wait), &wait);
    while (dev->current_len == GLOBALFIFO_SIZE)
    {
        if (filp->f_flags & O_NONBLOCK)
        {
            ret = -EAGAIN;
            goto out;
        }
        //如果数据区时满的，则挂起写进程
        __set_current_state(TASK_INTERRUPTIBLE);
        mutex_unlock(&(dev->mutex));

        schedule();
        if (signal_pending(current))
        {
            ret = -ERESTARTSYS;
            goto out2;
        }
        mutex_lock(&(dev->mutex));
    }

    if (count > (GLOBALFIFO_SIZE - dev->current_len))
        count = GLOBALFIFO_SIZE - dev->current_len;

    if (copy_from_user(dev->mem + dev->current_len, buf, count))//从用户空间复制内容
    {
        ret = -EFAULT;
        goto out;
    }
    else
    {
        dev->current_len += count;
        ret = count;
        printk(KERN_INFO"write %u bytes, current_len:%d\n", count, dev->current_len);
        wake_up_interruptible(&(dev->r_wait));
        //当写入数据是，触发异步通知
        if (dev->async_queue)
        {
            printk(KERN_DEBUG"driver send SIGIO\n");
            kill_fasync(&(dev->async_queue), SIGIO, POLL_IN);
        }
        ret = count;
    }
out:
    mutex_unlock(&(dev->mutex));
out2:
    remove_wait_queue(&(dev->w_wait), &wait);
    set_current_state(TASK_RUNNING);

    return ret;
}
static loff_t globalfifo_llseek(struct file* filp, loff_t offset, int orig)
{
    loff_t ret = 0;

    switch(orig)
    {
        case 0://从文件开头seek
            if (offset < 0 || (unsigned int)offset > GLOBALFIFO_SIZE)
            {
                ret = -EINVAL;
                break;
            }
            filp->f_pos = (unsigned int)offset;//设置文件对象新位置
            ret = filp->f_pos;
            break;
        case 1://从文件当前位置seek
            if ((filp->f_pos + offset) > GLOBALFIFO_SIZE || (filp->f_pos + offset) < 0)
            {
                ret = -EINVAL;
                break;
            }
            filp->f_pos += (unsigned int)offset;//设置文件对象新位置
            ret = filp->f_pos;
            break;
        default:
            ret = -EINVAL;
            break;
    }
    return ret;
}
static long globalfifo_ioctl(struct file* filp, unsigned int cmd, unsigned long arg)
{
    struct globalfifo_dev* dev = filp->private_data;

    switch(cmd)
    {
        case MEM_CLEAR: //本示例里我们只支持clear命令
            //获取互斥量
            mutex_lock(&(dev->mutex));
            memset(dev->mem, 0, GLOBALFIFO_SIZE);
            //释放互斥量
            mutex_unlock(&(dev->mutex));
            printk(KERN_INFO"globalfifo is set to zero\n");
            break;
        default:
            return -EINVAL;
    }
    return 0;
}
static unsigned int globalfifo_poll(struct file* filp, poll_table* wait)
{
    unsigned int mask = 0;
    struct globalfifo_dev* dev = filp->private_data;

    mutex_lock(&(dev->mutex));//获取互斥量
    //将设备中的读写队列加入到poll_table中，这样设备读写进程可以唤醒select中的进程
    poll_wait(filp, &(dev->r_wait), wait);
    poll_wait(filp, &(dev->w_wait), wait);

    if (dev->current_len != 0)
        mask |= POLLIN | POLLRDNORM; //有数据，返回可读状态

    if (dev->current_len != GLOBALFIFO_SIZE)
        mask |= POLLOUT | POLLWRNORM; //数据区未满，返回可写状态

    mutex_unlock(&(dev->mutex));
    return mask;
}
//定义文件操作结构体
static const struct file_operations globalfifo_fops = 
{
    .owner = THIS_MODULE,
    .llseek = globalfifo_llseek,
    .read = globalfifo_read,
    .write = globalfifo_write,
    .unlocked_ioctl = globalfifo_ioctl,
    .open = globalfifo_open,
    .release = globalfifo_release,
    .poll = globalfifo_poll,
    .fasync = globalfifo_fasync
};

//驱动模块加载函数
static void globalfifo_setup_cdev(struct globalfifo_dev* dev, int index)
{
    int err, devno = MKDEV(globalfifo_major, index); //获得dev_t对象
    cdev_init(&dev->cdev, &globalfifo_fops);//初始化设备
    dev->cdev.owner = THIS_MODULE;
    //注册设备
    err = cdev_add(&dev->cdev, devno, 1);
    if (err)
    {
        printk(KERN_NOTICE"Error %d adding globalfifo %d", err, index);
    }
}
static int __init globalfifo_init(void)
{
    int ret;
    dev_t devno = MKDEV(globalfifo_major, 0);
    //申请设备号
    if (globalfifo_major)
    {
        ret = register_chrdev_region(devno, 1, "globalfifo");
    }
    else
    {
        ret = alloc_chrdev_region(&devno, 0, 1, "globalfifo");
        globalfifo_major = MAJOR(devno);
    }

    if (ret < 0)
        return ret;

    globalfifo_devp = kzalloc(sizeof(struct globalfifo_dev), GFP_KERNEL);
    if (!globalfifo_devp)
    {
        //空间申请失败
        ret = -ENOMEM;
        goto fail_malloc;
    }

    globalfifo_setup_cdev(globalfifo_devp, 0);
    //初始化互斥量
    mutex_init(&(globalfifo_devp->mutex));
    //初始化阻塞队列
    init_waitqueue_head(&(globalfifo_devp->r_wait));
    init_waitqueue_head(&(globalfifo_devp->w_wait));
    return 0;

fail_malloc:
    unregister_chrdev_region(devno, 1);
    return ret;
}
module_init(globalfifo_init);

//驱动模块卸载函数
static void __exit globalfifo_exit(void)
{
    cdev_del(&globalfifo_devp->cdev);//注销设备
    kfree(globalfifo_devp);
    unregister_chrdev_region(MKDEV(globalfifo_major, 0), 1);//释放设备号
}
module_exit(globalfifo_exit);

//模块声明
MODULE_AUTHOR("BARRET REN <barret.ren@outlook.com>");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("A driver for virtual globalfifo charactor device");
MODULE_ALIAS("globalfifo device driver");
