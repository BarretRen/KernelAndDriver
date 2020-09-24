/*
 * simple kernel module: hello
 */
#include <linux/init.h>
#include <linux/module.h>

MODULE_AUTHOR("BARRET REN");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("A simaple hello world module");
MODULE_ALIAS("a simplest module");

static int __init hello_init(void)
{
    printk(KERN_INFO "Hello world enter\n");
    return 0;
}

static void __exit hello_exit(void)
{
    printk(KERN_INFO "Hello world exit\n");
}

//模块初始化和注销函数指定
module_init(hello_init);
module_exit(hello_exit);