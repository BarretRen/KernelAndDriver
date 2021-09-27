//hello-sysfs.c sysfs example
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/sysfs.h>

static struct kobject *mymodule;

//定义一个将要用sysfs修改的变量
static int myvariable = 0;

static ssize_t myvariable_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", myvariable);
}

static ssize_t myvariable_store(struct kobject *kobj, struct kobj_attribute *attr, char *buf, size_t count)
{
    sscanf(buf, "%du", &myvariable);
    return count;
}
//绑定上面两个函数
static struct kobj_attribute myvariable_attribute = __ATTR(myvariable, 0660, myvariable_show, (void*)myvariable_store);

//模块加载卸载
static int __init my_init(void)
{
    int error = 0;
    pr_info("mymodule: init\n");

    mymodule = kobject_create_and_add("mymodule", kernel_kobj);
    if (!mymodule)
        return -ENOMEM;

    error = sysfs_create_file(mymodule, &myvariable_attribute.attr);
    if (error)
    {
        pr_info("failed to create the file in /sys/kernel/mymodule\n");
    }
    return error;
}

static void __exit my_exit(void)
{
    pr_info("mymodule: exit success\n");
    kobject_put(mymodule);
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");

