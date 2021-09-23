/*
* hello-5.c - Demonstrates command line argument passing to a module.
*/
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/stat.h>

MODULE_LICENSE("GPL");

//定义变量并指定默认值
static short int myshort = 1;
static int myint = 420;
static long int mylong = 9999;
static char* mystring = "blah";
static int myarray[2] = {420, 420};
static int arr_argc = 0;

//声明获取命令行value

module_param(myshort, short, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(myshort, "A short integer");

module_param(myint, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(myshort, "A integet");

module_param(mylong, long, S_IRUSR);
MODULE_PARM_DESC(myshort, "A long integer");

module_param(mystring, charp, 0000);
MODULE_PARM_DESC(myshort, "A character string");

module_param_array(myarray, int, &arr_argc, 0000);
MODULE_PARM_DESC(myarray, "A array of integer");

static int __init hello_5_init(void)
{
    int i;
    pr_info("Hello, world 5\n=============\n");
    pr_info("myshort is a short integer: %hd\n", myshort);
    pr_info("myint is an integer: %d\n", myint);
    pr_info("mylong is a long integer: %ld\n", mylong);

    pr_info("mystring is a string: %s\n", mystring);
    for (i = 0; i < ARRAY_SIZE(myarray); i++)
        pr_info("myintarray[%d] = %d\n", i, myarray[i]);

    pr_info("got %d arguments for myintarray.\n", arr_argc);
    return 0;
}

static void __exit hello_5_exit(void)
{
    pr_info("Goodbye, world 5\n");
}

module_init(hello_5_init);
module_exit(hello_5_exit);
