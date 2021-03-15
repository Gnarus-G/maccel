// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  Copyright (c) 1999-2001 Vojtech Pavlik
 *
 *  USB HIDBP Mouse support
 */

/*
 *
 * Should you need to contact me, the author, you can do so either by
 * e-mail - mail your message to <vojtech@ucw.cz>, or by paper mail:
 * Vojtech Pavlik, Simunkova 1594, Prague 8, 182 00 Czech Republic
 */

// Config for acceleration in here
#include "leetmouse.h"
#include <linux/time.h>
#define BUFFER_SIZE 16												// Maximum number of packets allowed to be sent from the mouse at once. Linux's default value is 8, which at least causes EOVERFLOW for my mouse (SteelSeries Rival 600). Increase this, if 'dmesg -w' tells you to!

//Needed for kernel_fpu_begin/end
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(5,0,0)
    //Pre Kernel 5.0.0
    #include <asm/i387.h>
#else
    #include <asm/fpu/api.h>
#endif

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/usb/input.h>
#include <linux/hid.h>

/* for apple IDs */
#ifdef CONFIG_USB_HID_MODULE
//#include "../hid-ids.h"
#include "hid-ids.h"
#endif

/*
 * Version Information
 */
#define DRIVER_VERSION "v1.6"
#define DRIVER_AUTHOR "Vojtech Pavlik <vojtech@ucw.cz>"
#define DRIVER_DESC "USB HID Boot Protocol mouse driver with acceleration"

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");

struct usb_mouse {
	char name[128];
	char phys[64];
	struct usb_device *usbdev;
	struct input_dev *dev;
	struct urb *irq;

	signed char *data;
	dma_addr_t data_dma;
};

//Hack. Is this true for all mice and all kernels? At least works for me!
struct mouse_data {
	signed char btn;
	signed short rel_x;
	signed short rel_y;
	signed short rel_wheel;
};

static inline int Leet_round(float x)
{
    if (x >= 0) {
        return (int)(x + 0.5f);
    } else {
        return (int)(x - 0.5f);
    }
}

// What do we have here? Code from Quake 3, which is also GPL.
// https://en.wikipedia.org/wiki/Fast_inverse_square_root
// Copyright (C) 1999-2005 Id Software, Inc.
static inline float Q_sqrt(float number)
{
    long i;
    float x2, y;
    const float threehalfs = 1.5F;
    
    x2 = number * 0.5F;
    y  = number;
    i  = * ( long * ) &y;                       // evil floating point bit level hacking
    i  = 0x5f3759df - ( i >> 1 );               // what the fuck?
    y  = * ( float * ) &i;
    y  = y * ( threehalfs - ( x2 * y * y ) );   // 1st iteration
    //	y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed
    
    return 1 / y;
}

// Acceleration happens here
static void accelerate(int* x, int* y){    
	float delta_x, delta_y, ms, rate;
	float accel_sens = SENSITIVITY;
	static float carry_x = 0.0f;
    static float carry_y = 0.0f;
	static float last_ms = 1.0f;
	static ktime_t last;
	ktime_t now;

    //We are going to use the FPU within the kernel. So we need to safely switch context during all FPU processing in order to not corrupt the userspace FPU state
    kernel_fpu_begin();
    
	//PreScale raw data from mouse
    delta_x = (*x) * PRE_SCALE_X;
    delta_y = (*y) * PRE_SCALE_Y;

	//Calculate frametime to derive mouse rate & speed
	now = ktime_get();
    ms = (now - last)/(1000*1000);
	last = now;
	if(ms < 1) ms = last_ms; //Sometimes, urbs appear bunched -> Beyond µs resolution so the timing reading is plain wrong. Fallback to last known valid frametime
	if(ms > 200) ms = 200;
	last_ms = ms;
	
	//printk("MOUSE: %ld", (long) (1000*ms));

    rate = Q_sqrt(delta_x * delta_x + delta_y * delta_y);

	//Apply speedcap (is actually a "distance"-cap)
    if(SPEED_CAP != 0){
        if (rate >= SPEED_CAP) {
            delta_x *= SPEED_CAP / rate;
            delta_y *= SPEED_CAP / rate;
        }
    }

	//Calculate rate from travelled overall distance and add possible rate offsets
    rate /= ms;
    rate -= OFFSET;

	//Apply linear acceleration on the sensitivity if applicable and limit maximum value
    if(rate > 0){
        rate *= ACCELERATION;
        accel_sens += rate;
    }
    if(SENS_CAP > 0 && accel_sens >= SENS_CAP){
        accel_sens = SENS_CAP;
    }

	//Actually apply accelerated sensitivity, allow post-scaling and apply carry from previous round
    accel_sens /= SENSITIVITY;
    delta_x *= accel_sens;
    delta_y *= accel_sens;
    delta_x *= POST_SCALE_X;
    delta_y *= POST_SCALE_Y;
    delta_x += carry_x;
    delta_y += carry_y;

    //Cast back to ints
    *x = Leet_round(delta_x);
    *y = Leet_round(delta_y);

	//Save carry for next round
	carry_x = delta_x - *x;
    carry_y = delta_y - *y;
    
    //We stopped using the FPU: Switch back context again
    kernel_fpu_end();
}

static void usb_mouse_irq(struct urb *urb)
{
	struct usb_mouse *mouse = urb->context;
	signed char *raw = mouse->data;
	struct mouse_data *data = (struct mouse_data*) raw;
	struct input_dev *dev = mouse->dev;
	signed int btn,x,y,wheel;
	int status;

	switch (urb->status) {
	case 0:			/* success */
		break;
	case -ECONNRESET:	/* unlink */
	case -ENOENT:
	case -ESHUTDOWN:
		return;
	case -EOVERFLOW:
		printk("LEETMOUSE: EOVERFLOW. Try to increase BUFFER_SIZE from %d to %d in 'leetmouse.c'", BUFFER_SIZE, 2*BUFFER_SIZE);
		goto resubmit;
	/* -EPIPE:  should clear the halt */
	default:		/* error */
		goto resubmit;
	}

	btn = data->btn;
	x = le16_to_cpu(data->rel_x);
	y = le16_to_cpu(data->rel_y);
	wheel = data->rel_wheel;



	//printk("MOUSE: %d",x);
	x = raw[1];
	y = raw[3];

	accelerate(&x,&y);

	

	//printk("MOUSE: %d %d %d %d %d %d %d %d   %d %d %d %d %d %d %d %d", (raw[1] >> 0) & 1,(raw[1] >> 1) & 1,(raw[1] >> 2) & 1,(raw[1] >> 3) & 1,(raw[1] >> 4) & 1,(raw[1] >> 5) & 1,(raw[1] >> 6) & 1,(raw[1] >> 7),
	//(raw[2] >> 0) & 1,(raw[2] >> 1) & 1,(raw[2] >> 2) & 1,(raw[2] >> 3) & 1,(raw[2] >> 4) & 1,(raw[2] >> 5) & 1,(raw[2] >> 6) & 1,(raw[2] >> 7));
	
	//printk("%d", urb-> transfer_flags);
	//printk("MOUSE: %d %d %d", data[1], data[2], data[3]);

	input_report_key(dev, BTN_LEFT,   raw[0] & 0x01);
	input_report_key(dev, BTN_RIGHT,  raw[0] & 0x02);
	input_report_key(dev, BTN_MIDDLE, raw[0] & 0x04);
	input_report_key(dev, BTN_SIDE,   raw[0] & 0x08);
	input_report_key(dev, BTN_EXTRA,  raw[0] & 0x10);

	input_report_rel(dev, REL_X,     x);
	input_report_rel(dev, REL_Y,     y);
    
	input_report_rel(dev, REL_WHEEL, raw[5]);
    
	input_sync(dev);
resubmit:
	status = usb_submit_urb (urb, GFP_ATOMIC);
	
	if (status)
		dev_err(&mouse->usbdev->dev,
			"can't resubmit intr, %s-%s/input0, status %d\n",
			mouse->usbdev->bus->bus_name,
			mouse->usbdev->devpath, status);
}

static int usb_mouse_open(struct input_dev *dev)
{
	struct usb_mouse *mouse = input_get_drvdata(dev);

	mouse->irq->dev = mouse->usbdev;
	if (usb_submit_urb(mouse->irq, GFP_KERNEL))
		return -EIO;

	return 0;
}

static void usb_mouse_close(struct input_dev *dev)
{
	struct usb_mouse *mouse = input_get_drvdata(dev);

	usb_kill_urb(mouse->irq);
}

static int usb_mouse_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct usb_device *dev = interface_to_usbdev(intf);
	struct usb_host_interface *interface;
	struct usb_endpoint_descriptor *endpoint;
	struct usb_mouse *mouse;
	struct input_dev *input_dev;
	int pipe, maxp;
	int error = -ENOMEM;

	interface = intf->cur_altsetting;

	if (interface->desc.bNumEndpoints != 1)
		return -ENODEV;

	endpoint = &interface->endpoint[0].desc;
	if (!usb_endpoint_is_int_in(endpoint))
		return -ENODEV;

	pipe = usb_rcvintpipe(dev, endpoint->bEndpointAddress);
	maxp = usb_maxpacket(dev, pipe, usb_pipeout(pipe));

	mouse = kzalloc(sizeof(struct usb_mouse), GFP_KERNEL);
	input_dev = input_allocate_device();
	if (!mouse || !input_dev)
		goto fail1;

	mouse->data = usb_alloc_coherent(dev, BUFFER_SIZE, GFP_ATOMIC, &mouse->data_dma);
	if (!mouse->data)
		goto fail1;

	mouse->irq = usb_alloc_urb(0, GFP_KERNEL);
	if (!mouse->irq)
		goto fail2;

	mouse->usbdev = dev;
	mouse->dev = input_dev;

	if (dev->manufacturer)
		strlcpy(mouse->name, dev->manufacturer, sizeof(mouse->name));

	if (dev->product) {
		if (dev->manufacturer)
			strlcat(mouse->name, " ", sizeof(mouse->name));
		strlcat(mouse->name, dev->product, sizeof(mouse->name));
	}

	if (!strlen(mouse->name))
		snprintf(mouse->name, sizeof(mouse->name),
			 "USB HIDBP Mouse %04x:%04x",
			 le16_to_cpu(dev->descriptor.idVendor),
			 le16_to_cpu(dev->descriptor.idProduct));

	usb_make_path(dev, mouse->phys, sizeof(mouse->phys));
	strlcat(mouse->phys, "/input0", sizeof(mouse->phys));

	input_dev->name = mouse->name;
	input_dev->phys = mouse->phys;
	usb_to_input_id(dev, &input_dev->id);
	input_dev->dev.parent = &intf->dev;

	input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REL);
	input_dev->keybit[BIT_WORD(BTN_MOUSE)] = BIT_MASK(BTN_LEFT) |
		BIT_MASK(BTN_RIGHT) | BIT_MASK(BTN_MIDDLE);
	input_dev->relbit[0] = BIT_MASK(REL_X) | BIT_MASK(REL_Y);
	input_dev->keybit[BIT_WORD(BTN_MOUSE)] |= BIT_MASK(BTN_SIDE) |
		BIT_MASK(BTN_EXTRA);
	input_dev->relbit[0] |= BIT_MASK(REL_WHEEL);

	input_set_drvdata(input_dev, mouse);

	input_dev->open = usb_mouse_open;
	input_dev->close = usb_mouse_close;

	usb_fill_int_urb(mouse->irq, dev, pipe, mouse->data,
			 (maxp > BUFFER_SIZE ? BUFFER_SIZE : maxp),
			 usb_mouse_irq, mouse, endpoint->bInterval);
	mouse->irq->transfer_dma = mouse->data_dma;
	mouse->irq->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

	error = input_register_device(mouse->dev);
	if (error)
		goto fail3;

	usb_set_intfdata(intf, mouse);
	return 0;

fail3:	
	usb_free_urb(mouse->irq);
fail2:	
	usb_free_coherent(dev, BUFFER_SIZE, mouse->data, mouse->data_dma);
fail1:	
	input_free_device(input_dev);
	kfree(mouse);
	return error;
}

static void usb_mouse_disconnect(struct usb_interface *intf)
{
	struct usb_mouse *mouse = usb_get_intfdata (intf);

	usb_set_intfdata(intf, NULL);
	if (mouse) {
		usb_kill_urb(mouse->irq);
		input_unregister_device(mouse->dev);
		usb_free_urb(mouse->irq);
		usb_free_coherent(interface_to_usbdev(intf), BUFFER_SIZE, mouse->data, mouse->data_dma);
		kfree(mouse);
	}
}

static const struct usb_device_id usb_mouse_id_table[] = {
	{ USB_INTERFACE_INFO(USB_INTERFACE_CLASS_HID, USB_INTERFACE_SUBCLASS_BOOT,
		USB_INTERFACE_PROTOCOL_MOUSE) },
	{ }	/* Terminating entry */
};

MODULE_DEVICE_TABLE (usb, usb_mouse_id_table);

static struct usb_driver usb_mouse_driver = {
	.name		= "leetmouse",
	.probe		= usb_mouse_probe,
	.disconnect	= usb_mouse_disconnect,
	.id_table	= usb_mouse_id_table,
};

module_usb_driver(usb_mouse_driver);
