#include <linux/module.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include<linux/slab.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/random.h>  //for generating random numbers


#define MAGIC_NUM 100
#define channel_no _IOW(MAGIC_NUM,0,int32_t*)
#define align _IOW(MAGIC_NUM,1,char*)

int32_t channel_num=0;
char alignment;
 
static dev_t adc8; // variable for device number
static struct cdev c_dev;
static int __init adc8_init(void);
static struct class *cls;
static unsigned int a;

//unsigned char cpp[100] = "heythisisateststring";

//STEP 4 : Driver callback functions

//------------ OPEN ------------//
static int adc_open(struct inode *i, struct file *f)
{
	printk(KERN_INFO "mychar: open()\n");
	return 0;
}

//------------ CLOSE ------------//
static int adc_close(struct inode *i, struct file *f)
{
	printk(KERN_INFO "mychar: close()\n");
	return 0;
}

//------------ READ ------------//
static ssize_t adc_read(struct file *f, char __user *buf, size_t len, loff_t *off)
{
	get_random_bytes(&a,2);
	a=a%1024;
	printk(KERN_INFO "ADC8 reading is %d\n\n",a);
	copy_to_user(buf,&a,sizeof(a));
	return sizeof(a);
}

static long adc_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
{
	switch(ioctl_num)
	{	
		case channel_no:
				copy_from_user(&channel_num,(int32_t*)ioctl_param,sizeof(channel_num));
				printk(KERN_INFO"Selected channel no is %d\n",channel_num);
		break;
		case align:
				copy_from_user(&alignment,(char*)ioctl_param,sizeof(alignment));
				printk(KERN_INFO"Required alignemnt is %c\n",alignment);
		break;

	}
return 0;
}


 static const struct file_operations fops =
				{
					.owner 	= THIS_MODULE,
					.open 	= adc_open,
					.read	= adc_read,
					.release= adc_close,
					.unlocked_ioctl  = adc_ioctl,
					
				};	


static int __init adc8_init(void) 
{
	printk(KERN_INFO "Hello: mychar driver registered");
	
	//  reserve major & minor number
	if (alloc_chrdev_region(&adc8, 0, 1, "ADC") < 0)
	{
		return -1;
	}
	
	// creation of device file
	if((cls=class_create(THIS_MODULE, "chardrv"))==NULL)
	{
		unregister_chrdev_region(adc8,1);
		return -1;
	}

	if(device_create(cls, NULL, adc8, NULL, "adc8_device")==NULL)
	{
		class_destroy(cls);
		unregister_chrdev_region(adc8,1);
		return -1;
	}	
	
	// Link fops and cdev to the device node
	cdev_init(&c_dev, &fops);
	if(cdev_add(&c_dev, adc8, 1)==-1)
	{
		device_destroy(cls, adc8);
		class_destroy(cls);
		unregister_chrdev_region(adc8,1);
		return -1;
	}
	printk(KERN_INFO"adc Device is registered successfully\n\n");
	return 0;
}
 
static void __exit adc8_exit(void) 
{
	cdev_del(&c_dev);
	device_destroy(cls, adc8);
	class_destroy(cls);
	unregister_chrdev_region(adc8, 1);
	printk(KERN_INFO "adc driver unregistered\n\n");
}
 
module_init(adc8_init);
module_exit(adc8_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Surbhi Kochar");
MODULE_DESCRIPTION("ADC Character Driver");


