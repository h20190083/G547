#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/usb.h>

#define CARD_READER_VID  0x14cd
#define CARD_READER_PID  0x125d

#define SANDISK_VID  0x0781   
#define SANDISK_PID  0x5567

#define READ_CAPACITY_LENGTH 0x0008
#define USB_ENDPOINT_IN    0x80
#define USB_ENDPOINT_INT   0x00

#define be_to_int32(buf) (((buf)[0]<<24)|((buf)[1]<<16)|((buf)[2]<<8)|(buf)[3])

#define RETRY_MAX  5

struct usbdev_private
{
	struct usb_device *udev;
	unsigned char class;
	unsigned char subclass;
	unsigned char protocol;
	unsigned char ep_in;
	unsigned char ep_out;
};

//struct usbdev_private *p_usbdev_info;

static void usbdev_disconnect(struct usb_interface *interface)
{
	printk(KERN_INFO "USBDEV Device Removed\n");
	return;
}

static struct usb_device_id usbdev_table [] = {
	{USB_DEVICE(CARD_READER_VID, CARD_READER_PID)},
	{USB_DEVICE(SANDISK_VID, SANDISK_PID)},
	{} /*terminating entry*/	
};

static int usbdev_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
	int i;
	unsigned char epAddr, epAttr;
	struct usb_endpoint_descriptor *ep_desc;
	
	if(id->idProduct == CARD_READER_PID && id->idVendor ==  CARD_READER_VID)
	{
		printk(KERN_INFO "Camera Plugged in\n");
	}
	
	else if(id->idProduct == SANDISK_PID && id->idVendor ==  SANDISK_VID)
	{
		printk(KERN_INFO "Known USB drive detected\n");
	}

	for(i=0;i<interface->cur_altsetting->desc.bNumEndpoints;i++)
	{
		ep_desc = &interface->cur_altsetting->endpoint[i].desc;
		epAddr = ep_desc->bEndpointAddress;
		epAttr = ep_desc->bmAttributes;
	
		if((epAttr & USB_ENDPOINT_XFERTYPE_MASK)==USB_ENDPOINT_XFER_BULK)
		{
			if(epAddr & 0x80)
				printk(KERN_INFO "EP %d is Bulk IN\n", i);
			else
				printk(KERN_INFO "EP %d is Bulk OUT\n", i);
	
		}

	}
	//this line causing error
	//p_usbdev_info->class = interface->cur_altsetting->desc.bInterfaceClass;
	printk(KERN_INFO "VID : %x", id->idVendor);
	printk(KERN_INFO "PID : %x", id->idProduct);
	printk(KERN_INFO "USB DEVICE CLASS : %x", interface->cur_altsetting->desc.bInterfaceClass);
	printk(KERN_INFO "USB DEVICE SUB CLASS : %x", interface->cur_altsetting->desc.bInterfaceSubClass);
	printk(KERN_INFO "USB DEVICE Protocol : %x", interface->cur_altsetting->desc.bInterfaceProtocol);
	printk(KERN_INFO "USB DEVICE no.of endpoints : %x", interface->cur_altsetting->desc.bNumEndpoints);

return 0;
}

// Section 5.1: Command Block Wrapper (CBW)
struct command_block_wrapper {
	uint8_t dCBWSignature[4];
	uint32_t dCBWTag;
	uint32_t dCBWDataTransferLength;
	uint8_t bmCBWFlags;
	uint8_t bCBWLUN;
	uint8_t bCBWCBLength;
	uint8_t CBWCB[16];
};

// Section 5.2: Command Status Wrapper (CSW)
struct command_status_wrapper {
	uint8_t dCSWSignature[4];
	uint32_t dCSWTag;
	uint32_t dCSWDataResidue;
	uint8_t bCSWStatus;
};

static uint8_t cdb_length[256] = {
//	 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
	06,06,06,06,06,06,06,06,06,06,06,06,06,06,06,06,  //  0
	06,06,06,06,06,06,06,06,06,06,06,06,06,06,06,06,  //  1
	10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,  //  2
	10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,  //  3
	10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,  //  4
	10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,  //  5
	00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,  //  6
	00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,  //  7
	16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,  //  8
	16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,  //  9
	12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,  //  A
	12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,  //  B
	00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,  //  C
	00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,  //  D
	00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,  //  E
	00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,  //  F
};



static int send_mass_storage_command(struct usb_device *udev, uint8_t endpoint, uint8_t lun,
	uint8_t *cdb, uint8_t direction, int data_length, uint32_t *ret_tag)
{
	static uint32_t tag = 1;
	uint8_t cdb_len;
	int i, r, size;
	
	struct command_block_wrapper cbw;

	if (cdb == NULL) {
		return -1;
	}

	if (endpoint & USB_ENDPOINT_IN) {
		printk("send_mass_storage_command: cannot send command on IN endpoint\n");
		return -1;
	}

	cdb_len = cdb_length[cdb[0]];

	if ((cdb_len == 0) || (cdb_len > sizeof(cbw.CBWCB))) {
		printk("send_mass_storage_command: don't know how to handle this command (%02X, length %d)\n",cdb[0], cdb_len);
		return -1;
	}

	memset(&cbw, 0, sizeof(cbw));
	cbw.dCBWSignature[0] = 'U';
	cbw.dCBWSignature[1] = 'S';
	cbw.dCBWSignature[2] = 'B';
	cbw.dCBWSignature[3] = 'C';
	*ret_tag = tag;
	cbw.dCBWTag = tag++;
	cbw.dCBWDataTransferLength = data_length;
	cbw.bmCBWFlags = direction;
	cbw.bCBWLUN = lun;
	// Subclass is 1 or 6 => cdb_len
	cbw.bCBWCBLength = cdb_len;
	memcpy(cbw.CBWCB, cdb, cdb_len);

	i = 0;
	do {
		// The transfer length must always be exactly 31 bytes.
		r =  usb_bulk_msg (udev, endpoint, (unsigned char*)&cbw, 31, &size, 1000);
	   
		if (r == 0) {
			usb_clear_halt(udev, endpoint);
		}
		i++;
	} while ((r == 0) && (i<RETRY_MAX));
	if (r != 0) {
		printk(KERN_INFO"send_mass_storage_command:\n");
		return -1;
	}

	//printk(" sent %d CDB bytes\n", cdb_len);
	return 0;
}


static int get_mass_storage_status(struct usb_device *udev, uint8_t endpoint, uint32_t expected_tag)
{
	int i, r, size;
	struct command_status_wrapper csw;

	i = 0;
	do {
		r = usb_bulk_msg(udev, endpoint, (unsigned char*)&csw, 13, &size, 1000);
		if (r == 0) {
			usb_clear_halt(udev, endpoint);
		}
		i++;
	} while ((r == 0) && (i<RETRY_MAX));

	if (r != 0) {
		printk(KERN_INFO"get_mass_storage_status: %d\n",r);
		return -1;
	}

	if (size != 13) {
		printk(KERN_INFO"get_mass_storage_status: received %d bytes (expected 13)\n", size);
		return -1;
	}

	if (csw.dCSWTag != expected_tag) {
		printk(KERN_INFO"get_mass_storage_status: mismatched tags (expected %08X, received %08X)\n",expected_tag, csw.dCSWTag);
		return -1;
	}
	

}

/* actual transfer of data*/
int mass_storage(struct usb_device *udev, uint8_t endpoint_in, uint8_t endpoint_out)
{
	int  size;
	uint8_t lun=0;
	uint32_t expected_tag;
	long max_lba, block_size;
	long device_size;
	uint8_t cdb[16];	// SCSI Command Descriptor Block the SCSI command that we want to send
	uint8_t *buffer=NULL;

//if ( !(buffer = (uint8_t *)vmalloc(sizeof(uint8_t)*64, GFP_KERNEL)) ) {
		//printk(KERN_INFO"Cannot allocate memory for rcv buffer\n");
	//return -1;
	//}


// Read capacity
	printk(KERN_INFO "Reading Capacity:\n");
	memset(cdb, 0, sizeof(cdb));
	cdb[0] = 0x25;	// Read Capacity
//r = usb_control_msg(device, LIBUSB_ENDPOINT_IN|LIBUSB_REQUEST_TYPE_CLASS|LIBUSB_RECIPIENT_INTERFACE,BOMS_GET_MAX_LUN, 0, 0, &lun, 1, 1000);


 send_mass_storage_command(udev,endpoint_out, lun, cdb,USB_ENDPOINT_IN, READ_CAPACITY_LENGTH, &expected_tag); {
		printk(KERN_INFO"error in command\n");
		return -1;
	}

    usb_bulk_msg (udev, endpoint_in, (unsigned char*)&buffer, READ_CAPACITY_LENGTH, &size, 1000);
	printk("bytes received %d \n", size);
	max_lba = be_to_int32(&buffer[0]);
	block_size = be_to_int32(&buffer[4]);
	device_size = ((double)(max_lba+1))*block_size/(1024*1024*1024);
	
	printk(KERN_INFO "Size of Device : %ld GB\n",device_size);
	//vfree(buffer);

	get_mass_storage_status(udev, endpoint_in, expected_tag);
	return 0;
	
}

/*Operations structure*/
static struct usb_driver usbdev_driver = {
	name: "usbdev",  //name of the device
	probe: usbdev_probe, // Whenever Device is plugged in
	disconnect: usbdev_disconnect, // When we remove a device
	id_table: usbdev_table, //  List of devices served by this driver
};

/*** registration of the device ***/
__init int device_init(void)
{
	printk(KERN_NOTICE "UAS READ Capacity Driver Inserted\n");
	usb_register(&usbdev_driver);
	
	return 0;
}

__exit void device_exit(void)
{
	usb_deregister(&usbdev_driver);
	printk(KERN_NOTICE "Leaving Kernel\n");
	 
	//return 0;
	
}

module_init(device_init);
module_exit(device_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("SURBHI <kocharsurbhi1996@gmail.com>");
