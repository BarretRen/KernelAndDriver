/*
* procdev_rw2.c - read and write "file" in /proc
*/
#include <linux/kernel.h> /* We're doing kernel work */
#include <linux/module.h> /* Specifically, a module */
#include <linux/proc_fs.h> /* Necessary because we use the proc fs */
#include <linux/uaccess.h> /* for copy_from_user */
#include <linux/version.h>
#include <linux/sched.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
#define HAVE_PROC_OPS
#endif

#define PROCFS_MAX_SIZE 2048
#define procfs_name "procdev3"

static struct proc_dir_entry *our_proc_file;
/* The buffer used to store character for this module */
static char procfs_buffer[PROCFS_MAX_SIZE];

//ops函数
static ssize_t procfile_read(struct file* filePointer, char __user *buffer,
                             size_t buffer_length, loff_t* offset)
{
    unsigned long count = buffer_length;
    unsigned long p = *offset;
    ssize_t ret = 0;
    //判断文件偏移量
    if (p >= PROCFS_MAX_SIZE)
        return 0;//偏移量不能超过buffer容量

    if (count > (PROCFS_MAX_SIZE - p))
        count = PROCFS_MAX_SIZE - p;

    if(copy_to_user(buffer, procfs_buffer + p, count))
    {
        pr_info("copy_to_user failed\n");
        ret = 0;
    }
    else
    {
        pr_info("procfile_read file %s\n", filePointer->f_path.dentry->d_name.name);
        *offset += count;
        ret = count;
    }
    return ret;
}

/* This function is called with the /proc file is written. */
static ssize_t procfile_write(struct file *file, const char __user *buff,
                              size_t len, loff_t *off)
{
    unsigned long p = *off;
    unsigned int count = len;
    int ret = 0;

    if (p >= PROCFS_MAX_SIZE)
        return 0;//偏移位置不能超过空间容量

    if (count > (PROCFS_MAX_SIZE - p))
        count = PROCFS_MAX_SIZE - p;//字节数不能超过容量

    if (copy_from_user(procfs_buffer + p, buff, count))
    {
        pr_info("copy_from_user failed\n");
        return -EFAULT;
    }
    else
    {
        pr_info("copy_from_user success, write length:%d\n", count);
        *off += count;
        ret = count;
    }

    return ret;
}

static int procfile_open(struct inode *inode, struct file *file)
{
    try_module_get(THIS_MODULE);
    return 0;
}

static int procfile_close(struct inode *inode, struct file *file)
{
    module_put(THIS_MODULE);
    return 0;
}

//定义ops结构体
#ifdef HAVE_PROC_OPS
static const struct proc_ops proc_file_ops = {
    proc_read:procfile_read,
    proc_write:procfile_write,
    proc_open:procfile_open,
    proc_release:procfile_close,
};
#else
static const struct file_operations proc_file_ops = {
    read:procfile_read,
    write:procfile_write,
    open:procfile_open,
    release:procfile_close,
};
#endif

//模块加载
static int __init profdev3_init(void)
{
    //创建proc文件,绑定ops
    our_proc_file = proc_create(procfs_name, 0644, NULL, &proc_file_ops);
    if (NULL == our_proc_file)
    {
        proc_remove(our_proc_file);
        pr_alert("Error: Could not initialize /proc/%s\n", procfs_name);
        return -ENOMEM;//没有内存空间了，创建失败
    }
    proc_set_size(our_proc_file, 80);
    proc_set_user(our_proc_file, GLOBAL_ROOT_UID, GLOBAL_ROOT_GID);

    pr_info("/proc/%s created\n", procfs_name);
    return 0;
}

//模块卸载
static void __exit procdev3_exit(void)
{
    //删除文件
    proc_remove(our_proc_file);
    pr_info("/proc/%s removed\n", procfs_name);
}

module_init(profdev3_init);
module_exit(procdev3_exit);

MODULE_LICENSE("GPL");

