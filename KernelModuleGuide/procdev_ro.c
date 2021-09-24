//proc虚拟文件驱动 read only，与字符设备驱动类似
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/version.h>

//判断内核版本
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,6,0)
#define HAVE_PROC_OPS //5.6之后内核才支持proc_ops
#endif

#define procfs_name "procdev" //虚拟文件名

static struct proc_dir_entry* our_proc_file;

static ssize_t procfile_read(struct file* filePointer, char __user *buffer,
                             size_t buffer_length, loff_t* offset)
{
    char s[13] = "HelloWorld!\n";
    int len = sizeof(s);
    ssize_t ret = len;
    //判断文件偏移量
    if(*offset >= len || copy_to_user(buffer, s, len))
    {
        pr_info("copy_to_user failed\n");
        ret = 0;
    }
    else
    {
        pr_info("procfile_read file %s\n", filePointer->f_path.dentry->d_name.name);
        *offset += len;
    }
    return ret;
}

//定义ops结构体
#ifdef HAVE_PROC_OPS
static const struct proc_ops proc_file_ops = {
    proc_read:procfile_read,
};
#else
static const struct file_operations proc_file_ops = {
    read:procfile_read,
};
#endif

//模块加载
static int __init profdev_init(void)
{
    //创建proc文件,绑定ops
    our_proc_file = proc_create(procfs_name, 0644, NULL, &proc_file_ops);
    if (NULL == our_proc_file)
    {
        proc_remove(our_proc_file);
        pr_alert("Error: Could not initialize /proc/%s\n", procfs_name);
        return -ENOMEM;//没有内存空间了，创建失败
    }

    pr_info("/proc/%s created\n", procfs_name);
    return 0;
}

//模块卸载
static void __exit procdev_exit(void)
{
    //删除文件
    proc_remove(our_proc_file);
    pr_info("/proc/%s removed\n", procfs_name);
}

module_init(profdev_init);
module_exit(procdev_exit);

MODULE_LICENSE("GPL");

