#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/io.h> 
#include <linux/interrupt.h>
#include <linux/of_device.h>
#include <linux/slab.h> //kmalloc and kfree
#include <linux/of.h> //device tree information 
#include <linux/mutex.h>
#include <linux/wait.h>

#define DRIVER_NAME "VCONV"

// Constants and addresses
#define INPUT_ADDR        0x00000000 //1 
#define INPUT_SIZE        (224 * 224 * 4)
//
#define OUTPUT_ADDR       0x0000ffff //2
#define OUTPUT_SIZE       (112 * 112 * 4)


#define AXI_BASE_ADDR     0xB0000010  //send input to this address which we get from user in ddr 
 
//3 
#define CMD_START_ADDR    0x00000000 //commands start address 
#define CMD_SIZE          28     //total 28 bytes have the 7 commands 

// temp registers 
#define START              0x00000000
#define STATUS             0x0000000C
#define STOP               0x00000014
#define CLEAR              0x00000018

/* Control register 
#define RESET_ADDR        (0x00)
#define NUM_DESC_ADDR     (0x08)
#define COMMAND_ADDR      (0x10)
#define VALID_ADDR        (0x14)

// Status
#define INT_STATUS        (0xFF)
#define INT_CLR           (0x1F)
*/
// IOCTL Commands

#define IOCTL_INPUT_IMAGE 		_IOW('C', 1, unsigned long) 
#define IOCTL_CMD  		        _IOW('C', 2, unsigned long)
#define IOCTL_START_CONV 	        _IO('C', 3)
#define IOCTL_STOP_CONV  	        _IO('C', 4)

// Driver Structure
struct conv_ip {
    struct device *dev;//device 
    dev_t devt;
    struct cdev cdev;
    struct class *conv_class;
    struct work_struct output_data_work_queue; //schedule work for bg process of handling interrupt (bottom halves)
    struct mutex process_data_work;
    //void __iomem* = is used for specially for i/o operations (mmio)
    void __iomem *input_buffer; // For storing the file from user in DDR 
    void __iomem *output_buffer; // For storing the output in DDR
    void __iomem *cmd_buffer; // For command register base
    void __iomem *axi_base; // AXI base address
    int last_cmd;
    void *output_data;
    int irq;
};
//
bool output_data_work_wq;

// Global Device Pointer
static struct conv_ip *conv_dev;

// Function declararions
static int conv_open(struct inode *inode, struct file *file);
static int conv_release(struct inode *inode, struct file *file);
static ssize_t conv_write(struct file *file, const char __user *buf, size_t len, loff_t *off);
static ssize_t conv_read(struct file *file, char __user *buf, size_t len, loff_t *off);
static long conv_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static irqreturn_t irq_handler(int irq, void *dev_id);


// File Operations
static const struct file_operations conv_fops = {
    .owner = THIS_MODULE,
    .open = conv_open,
    .release = conv_release,
    .write = conv_write,
    .read = conv_read,
    .unlocked_ioctl = conv_ioctl,
};

// Device Open
static int conv_open(struct inode *inode, struct file *file) {
    struct conv_ip *conv_dev = container_of(inode->i_cdev, struct conv_ip, cdev);
    file->private_data = conv_dev;
    if(!file->private_data){
    	pr_err("device is not opened");
    	return -ENODEV;
    	}
    printk("Device opened\n");
    return 0;
}

// Device Release
static int conv_release(struct inode *inode, struct file *file) {
    printk("Device closed\n");
    return 0;
}

// Write Data to Input Buffer and store it in DDR on sucess it returns len
static ssize_t conv_write(struct file *file, const char __user *buf, size_t len, loff_t *off) {
    struct conv_device *conv_dev = file->private_data;
    if(!file->private_data){
        pr_err("device is NULL");
        return -ENODEV;
    }
	if(conv_dev->last_cmd==1){
		if(len > INPUT_SIZE){
       	 		printk("Input exceeds buffer size\n");
       	 		return -EINVAL;
    			}

	if(copy_from_user((void __iomem *)conv_dev->input_buffer, buf, len)){
        	printk("Failed to copy image  from userspace\n");
        	return -EFAULT;
    		}
	}

	else if(conv_dev->last_cmd==2){
		if(len > CMD_SIZE){
       			printk("commands exceeds buffer size\n");
        		return -EINVAL;
    			}
	if(copy_from_user((void __iomem *)conv_dev->cmd_buffer,buf ,len)){
		printk("Failed to copy commands  from userspace\n");
      		return -EFAULT;
      		}
	}

    printk("Input image  and commands written to input buffer and command buffer in DDR\n");
    return len;
}


// Read Data from Output Buffer after convolution and store it in DDR on success it returns len
static ssize_t conv_read(struct file *file, char __user *buf, size_t len, loff_t *off) {
        struct conv_device *conv_dev = file->private_data;
        output_data_work_wq = false;
	//lock the mutex while the output buffer for multiple access
	mutex_lock(&conv_dev->process_data_work);
   	if (len > OUTPUT_SIZE) {
        printk("Read size exceeds output buffer size\n");	//lock mechanism 
        return -EINVAL;
    }
	if (copy_to_user(buf, conv_dev->output_data, len)) {
        printk("Failed to copy output data to userspace\n");
        return -EFAULT;
    }	
    //unlock after sending data to user 
    mutex_unlock(&conv_dev->process_data_work);

    printk("Output data read from DDR and sent to userspace\n");
    return len;
}

//workqueue function 
static void workqueue_fn(struct work_struct *work){
	
	conv_dev->output_data = kmalloc(OUTPUT_SIZE,GFP_KERNEL);
	if(!conv_dev->output_data){
		printk("error while allocating memory");
	}
        //lock this for dont access the shared resource 
        mutex_lock(&conv_dev->process_data_work);

        if(output_data_work_wq == false){	
	//to read all data to output from output buffer 
	for(int i = 0; i < OUTPUT_SIZE / 4; i++){
		*((u32 *)conv_dev->output_data) = ioread32(conv_dev->output_buffer + (i*4));
	}
	}

	printk("output data read from output buffer to ouput_data\n");

	output_data_work_wq = true;
	mutex_unlock(&conv_dev->process_data_work);
	kfree(conv_dev->output_data);
}

// Start Convolution using command
static int start_conv(struct conv_ip *conv_dev) {
	mutex_lock(&conv_dev->process_data_work); //lock
        writel(0x42, (void __iomem *)START); // CMD1 to start the operation
    	//printk("errror while start\n");
    	mutex_unlock(&conv_dev->process_data_work); //unlock
    	
    printk("Convolution started\n");
    return 0;
    
}

// Stop Convolution using command
static int stop_conv(void) {
        writel(0x05, (void __iomem *)STOP); // CMD5 to stop the operation
    	//printk("errror while stop\n");
    	
    printk("Convolution stopped\n");
    return 0;
}

// IOCTL Handler
static long conv_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    struct conv_ip *conv_dev = file->private_data;
    switch (cmd) {
    	case IOCTL_INPUT_IMAGE: 
    		if (conv_dev->last_cmd == 0){
    			conv_dev->last_cmd = 1;
    			}
    	    break;
    
    	case IOCTL_CMD :
    		if(conv_dev->last_cmd == 1){
    			conv_dev->last_cmd = 2;
    			}
    	    break;

        case IOCTL_START_CONV:
            start_conv();
            break;

        case IOCTL_STOP_CONV:
            stop_conv();
            break;

        default:
            return -EINVAL;
    }
    return 0;
}

// Interrupt Handler to stop the convolution and check the status
static irqreturn_t irq_handler(int irq, void *dev_id) {
    uint32_t status;

    status = readl((void __iomem *)STATUS); // Check the status at CMD4 (DMA Read Status)
    if (status & 0x1) { 
        printk("Convolution operation completed\n");

        writel(0x1, (void __iomem *)CLEAR); // CMD7 to clear interrupt
        printk("Interrupt cleared\n");
    } else {
        printk("Unexpected interrupt status: 0x%x\n", status);
    }

    //schedule the work queue 
   schedule_work(&conv_dev->output_data_work_queue);

    printk("workqueue function is called to store output data in output buffer");

    return IRQ_HANDLED;
}

// Probe Function for platform driver
static int conv_probe(struct platform_device *pdev) {
    uint64_t input_addr,command_addr,output_addr;
    int ret;
    struct resource *res;
    struct conv_ip *conv_dev; 
    struct device_node *node = pdev->dev.of_node;
    
    
    if (!pdev) {
        pr_err("pdev is NULL\n");
        return -ENODEV;
    }

    conv_dev = devm_kzalloc(&pdev->dev, sizeof(*conv_dev), GFP_KERNEL);
    if (!conv_dev)
        return -ENODEV;

    conv_dev->dev = &pdev->dev;

    //flag for write for two write operations from user 
    conv_dev->last_cmd = 0;
    
    // Map AXI Base Address to get the memory resource from platform device node in device tree
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res) {
    pr_err("Failed to get memory resource\n");
    return -ENODEV;
}

    conv_dev->axi_base = ioremap(res->start, resource_size(res));
    if (IS_ERR(conv_dev->axi_base)) {
        ret = PTR_ERR(conv_dev->axi_base);
        return -ENOMEM;
    }
    
    // Map Input and Output Buffers
    conv_dev->input_buffer = ioremap(INPUT_ADDR, INPUT_SIZE);
    if (IS_ERR(conv_dev->input_buffer)) {
    	ret = PTR_ERR(conv_dev->input_buffer);
        return -ENOMEM;
    }

    // Store commands in DDR 
    conv_dev->cmd_buffer = ioremap(CMD_START_ADDR, CMD_SIZE);
    if (IS_ERR(conv_dev->cmd_buffer)) {
    	ret = PTR_ERR(conv_dev->cmd_buffer);
        return -ENOMEM;
    }

    conv_dev->output_buffer = ioremap(OUTPUT_ADDR, OUTPUT_SIZE);
    if (IS_ERR(conv_dev->output_buffer)) {
    	ret = PTR_ERR(conv_dev->output_buffer);
        return -ENOMEM;
    }

    if(!node){
	    printk("error from device to get node");
	    return -EINVAL;
    }

    //read a integer value from device-tree iff success returns 0 if it error returns -EINVAL,-E 
    ret = of_property_read_u64(node,"input-addr",&input_addr);
   	if(ret){
    		pr_err("Failed to read input-addr from device tree\n");
		return ret;
	}
   
    //command property from device-tree
    ret = of_property_read_u64(node,"command-addr",&command_addr);
   	if(ret){
    		pr_err("Failed to read cmd-addr from device tree\n");
		return ret;
	}
    //output property from device-tree
    ret = of_property_read_u64(node,"output-addr",&output_addr);
   	if(ret){
    		pr_err("Failed to read output-addr from device tree\n");
		return -EINVAL;
	}

   // printk("read from device-tree properties are input = %s , cmd = %s and out = %s\n ",); 

    // Work queue initialization (after mutex) (Dynamic initialization) 
    INIT_WORK(&conv_dev->output_data_work_queue, workqueue_fn);

     // Initialize mutex (to protect shared resources)
    mutex_init(&conv_dev->process_data_work);
    // Allocate and Register Character Device
    ret = alloc_chrdev_region(&conv_dev->devt, 0, 1, "conv_ip");
    if (ret)
       goto err_class_create;

    cdev_init(&conv_dev->cdev, &conv_fops);
    conv_dev->cdev.owner = THIS_MODULE;

    ret = cdev_add(&conv_dev->cdev, conv_dev->devt, 1);
    if (ret) {
        unregister_chrdev_region(conv_dev->devt, 1);
        return ret;
    }

    conv_dev->conv_class = class_create(THIS_MODULE, "conv_ip_class");
    if (IS_ERR(conv_dev->conv_class)) {
        ret = PTR_ERR(conv_dev->conv_class);
        goto err_class_create;
    }

    if(IS_ERR(device_create(conv_dev->conv_class, NULL, conv_dev->devt, NULL, "conv_ip"))){
	    printk("error for device creation\n");
	    goto err_unregister_cdev;
}

    // Request IRQ
    conv_dev->irq = platform_get_irq(pdev, 0);
    if (conv_dev->irq < 0) {
        return conv_dev->irq;
    }

    ret = request_irq(conv_dev->irq, irq_handler, IRQF_SHARED, "conv_irq", conv_dev);
    if (ret)
        goto err_irq;

    // Output data work queue initialization
    output_data_work_wq = false;

    pr_info("Convolution device probed successfully\n");
    return 0;

err_irq:
    free_irq(conv_dev->irq,conv_dev);
    return ret;
err_unregister_cdev:
    cdev_del(&conv_dev->cdev);
    unregister_chrdev_region(conv_dev->devt, 1);
err_class_create:
    class_destroy(conv_dev->conv_class);
    kfree(conv_dev);
    return ret;

}

// Remove Function for platform driver
static int conv_remove(struct platform_device *pdev) {
    free_irq(conv_dev->irq, conv_dev);
    iounmap(conv_dev->input_buffer);
    iounmap(conv_dev->cmd_buffer);
    iounmap(conv_dev->output_buffer);
    cdev_del(&conv_dev->cdev);
    device_destroy(conv_dev->conv_class, conv_dev->devt);
    class_destroy(conv_dev->conv_class);
    unregister_chrdev_region(conv_dev->devt, 1);
    return 0;
}

// Device Tree Match Table
static const struct of_device_id conv_of_match[] = {
    { .compatible = "xlnx,vconv_ip" },
    { /* sentinel */ },
};


MODULE_DEVICE_TABLE(of, conv_of_match);
// Platform Driver
static struct platform_driver conv_driver = {
    .driver = {
        	.name = DRIVER_NAME,
        	.of_match_table = conv_of_match,
        	},
    .probe = conv_probe,
    .remove = conv_remove,
};

module_platform_driver(conv_driver);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("c");
MODULE_DESCRIPTION("Convolution IP Driver");
MODULE_ALIAS("platform:conv");

