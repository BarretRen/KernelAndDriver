//假设有个globalmem的字符设备，如何编写字符设备驱动
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/ioctl.h>

//定义宏
#define GLOBALMEM_SIZE 0X1000
#define GLOBALMEM_MAGIC 'g'
#define MEM_CLEAR _IO(GLOBALMEM_MAGIC,0)
#define GLOBALMEM_MAJOR 230 //默认主设备号

//添加模块参数，主设备号
static int globalmem_major = GLOBALMEM_MAJOR;
module_param(globalmem_major, int, S_IRUGO);

//定义字符驱动结构体
struct globalmem_dev
{
    struct cdev cdev;//系统字符设备结构体
    unsigned char mem[GLOBALMEM_SIZE];//模拟设备占用的内存
    struct mutex mutex; //增加互斥量
};
struct globalmem_dev* globalmem_devp;//指针，指向申请的设备空间

//file_operation成员函数
static int globalmem_open(struct inode* inode, struct file* filp)
{
    filp->private_data = globalmem_devp;
    return 0;
}
static int globalmem_release(struct inode* inode, struct file* filp)
{
    return 0;//释放文件，没什么特殊操作
}
static ssize_t globalmem_read(struct file* filp, char __user * buf, size_t size, loff_t* ppos)
{
    unsigned long p = *ppos;
    unsigned int count = size;
    int ret = 0;
    //文件的私有数据一般指向设备结构体,在open函数中设置
    struct globalmem_dev* dev = filp->private_data;
    
    if (p >= GLOBALMEM_SIZE)
        return 0;//偏移位置不能超过空间容量

    if (count > (GLOBALMEM_SIZE - p))
        count = GLOBALMEM_SIZE - p;//字节数不能超过容量

    //获取互斥量
    mutex_lock(&(dev->mutex));
    if (copy_to_user(buf, dev->mem + p, count))//复制内容到用户空间
    {
        ret = -EFAULT;
    }
    else
    {
        *ppos += count;
        ret = count;
        printk(KERN_INFO"read %u bytes from %lu", count, p);
    }
    //释放互斥量
    mutex_unlock(&(dev->mutex));
    return ret;
}
static ssize_t globalmem_write(struct file* filp, const char __user * buf, size_t size, loff_t* ppos)
{
    unsigned long p = *ppos;
    unsigned int count = size;
    int ret = 0;
    struct globalmem_dev* dev = filp->private_data;

    if (p >= GLOBALMEM_SIZE)
        return 0;//偏移位置不能超过空间容量

    if (count > (GLOBALMEM_SIZE - p))
        count = GLOBALMEM_SIZE - p;//字节数不能超过容量

    //获取互斥量
    mutex_lock(&(dev->mutex));
    if (copy_from_user(dev->mem + p, buf, count))//从用户空间复制内容
    {
        ret = -EFAULT;
    }
    else
    {
        *ppos += count;
        ret = count;
        printk(KERN_INFO"write %u bytes from %lu", count, p);
    }
    //释放互斥量
    mutex_unlock(&(dev->mutex));
    return ret;
}
static loff_t globalmem_llseek(struct file* filp, loff_t offset, int orig)
{
    loff_t ret = 0;

    switch(orig)
    {
        case 0://从文件开头seek
            if (offset < 0 || (unsigned int)offset > GLOBALMEM_SIZE)
            {
                ret = -EINVAL;
                break;
            }
            filp->f_pos = (unsigned int)offset;//设置文件对象新位置
            ret = filp->f_pos;
            break;
        case 1://从文件当前位置seek
            if ((filp->f_pos + offset) > GLOBALMEM_SIZE || (filp->f_pos + offset) < 0)
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
static long globalmem_ioctl(struct file* filp, unsigned int cmd, unsigned long arg)
{
    struct globalmem_dev* dev = filp->private_data;

    switch(cmd)
    {
        case MEM_CLEAR: //本示例里我们只支持clear命令
            //获取互斥量
            mutex_lock(&(dev->mutex));
            memset(dev->mem, 0, GLOBALMEM_SIZE);
            //释放互斥量
            mutex_unlock(&(dev->mutex));
            printk(KERN_INFO"globalmem is set to zero\n");
            break;
        default:
            return -EINVAL;
    }
    return 0;
}
//定义文件操作结构体
static const struct file_operations globalmem_fops = 
{
    .owner = THIS_MODULE,
    .llseek = globalmem_llseek,
    .read = globalmem_read,
    .write = globalmem_write,
    .unlocked_ioctl = globalmem_ioctl,
    .open = globalmem_open,
    .release = globalmem_release
};

//驱动模块加载函数
static void globalmem_setup_cdev(struct globalmem_dev* dev, int index)
{
    int err, devno = MKDEV(globalmem_major, index); //获得dev_t对象
    cdev_init(&dev->cdev, &globalmem_fops);//初始化设备
    dev->cdev.owner = THIS_MODULE;
    //注册设备
    err = cdev_add(&dev->cdev, devno, 1);
    if (err)
    {
        printk(KERN_NOTICE"Error %d adding globalmem %d", err, index);
    }
}
static int __init globalmem_init(void)
{
    int ret;
    dev_t devno = MKDEV(globalmem_major, 0);
    //申请设备号
    if (globalmem_major)
    {
        ret = register_chrdev_region(devno, 1, "globalmem");
    }
    else
    {
        ret = alloc_chrdev_region(&devno, 0, 1, "globalmem");
        globalmem_major = MAJOR(devno);
    }

    if (ret < 0)
        return ret;

    globalmem_devp = kzalloc(sizeof(struct globalmem_dev), GFP_KERNEL);
    if (!globalmem_devp)
    {
        //空间申请失败
        ret = -ENOMEM;
        goto fail_malloc;
    }

    //初始化互斥量
    mutex_init(&(globalmem_devp->mutex));
    globalmem_setup_cdev(globalmem_devp, 0);
    return 0;

fail_malloc:
    unregister_chrdev_region(devno, 1);
    return ret;
}
module_init(globalmem_init);

//驱动模块卸载函数
static void __exit globalmem_exit(void)
{
    cdev_del(&globalmem_devp->cdev);//注销设备
    kfree(globalmem_devp);
    unregister_chrdev_region(MKDEV(globalmem_major, 0), 1);//释放设备号
}
module_exit(globalmem_exit);

//模块声明
MODULE_AUTHOR("BARRET REN <barret.ren@outlook.com>");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("A driver for virtual globalmem charactor device");
MODULE_ALIAS("globalmem device driver");
