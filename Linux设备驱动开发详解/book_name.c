/*
 * 带模块参数的内核模块
 */
#include <linux/init.h>
#include <linux/module.h>

MODULE_AUTHOR("BARRET REN");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("A simaple module with params");
MODULE_ALIAS("a simplest module");

//添加模块参数定义
static char* book_name = "Linux device Driver";
module_param(book_name, charp, S_IRUGO);

static int book_num = 4000;
module_param(book_num, int, S_IRUGO);

static int __init hello_init(void)
{
    printk(KERN_INFO "book name :%s\n", book_name);
    printk(KERN_INFO "book num :%d\n", book_num);
    return 0;
}

static void __exit hello_exit(void)
{
    printk(KERN_INFO "Hello world exit\n");
}

//模块初始化和注销函数指定
module_init(hello_init);
module_exit(hello_exit);