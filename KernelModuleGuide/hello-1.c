#include <linux/kernel.h> //for pr_info()
#include <linux/module.h> // for all modules

int init_module(void)
{
    pr_info("hello world 1\n");
    return 0;
}

void cleanup_module(void)
{
    pr_info("goodbye world 1\n");
}

MODULE_AUTHOR("BARRET REN");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("A simaple hello world module");
