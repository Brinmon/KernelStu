#include <linux/init.h>  
#include <linux/module.h>  
#include <linux/kernel.h>  
  
MODULE_LICENSE("GPL");  
MODULE_AUTHOR("Your Name");  
MODULE_DESCRIPTION("A simple Hello World Module");  
MODULE_VERSION("0.01");  
  
static int __init hello_init(void) {  
    printk(KERN_INFO "Hello, world!\n");  
    return 0;  
}  
  
static void __exit hello_exit(void) {  
    printk(KERN_INFO "Goodbye, world!\n");  
}  
  
module_init(hello_init);  
module_exit(hello_exit);