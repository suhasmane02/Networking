/*
	Sample test driver.
	This sends layer 2 ECHO_REQ
	And recdeives ECHO_REPLY
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include "test.h"

static int __init test_module_init(void);
static void __exit test_module_exit(void);
static int test_rcv(struct sk_buff *skb, struct net_device *dev,
		struct packet_type *pt, struct net_device *orig_dev);
static int send_test(str_test *ptest_data);
static int process_data(struct sk_buff *skb);

static int major;
static struct sk_buff_head skb_q;

/* send data to NIC driver */
static int send_test(str_test *ptest_data)
{
	struct sk_buff *skb;
	/* destination mac hard coded for now
	   sender should put receivers MAC, and  receiver should put 
	   senders MAC 
	ToDO: get the destination MAC from user space.
	 */
	const unsigned char dest_hw[6] = {0x08,0x00,0x27,0x31,0x38,0xb0};
	/* const unsigned char dest_hw[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};*/
	const unsigned char *src_hw = NULL;
	struct net_device *dev;
	int hlen; 
	int tlen; 
	unsigned char *data;

	/* get device by index, every NIC interface is identified with an index
	   can be found in /sys/class/net/<interface_name>/ifindex
	ToDO: From user space, get the network interface name on which to send 
	packet.
	 */
	dev = dev_get_by_index(&init_net, 2);
	if(dev == NULL)
	{
		printk("failed to get device at index 2\n");
		return -1;
	}

	hlen = LL_RESERVED_SPACE(dev);
	tlen = dev->needed_tailroom;

	skb = alloc_skb(hlen + tlen, GFP_ATOMIC);
	if (!skb)
	{
		printk("skb allocation failed\n");
		return -2;
	}

	skb->dev = dev;
	skb_reserve(skb, hlen);
	skb_reset_network_header(skb);

	skb->dev = dev;
	skb->protocol = htons(ETH_P_TEST);
	if (!src_hw)
		src_hw = dev->dev_addr;

	data = skb_put(skb, sizeof(str_test));

	/*	printk("ptest_data count = %d\ntype = %d\ndata=%s\n", ptest_data->count, ptest_data->type, ptest_data->data);*/
	memcpy(data, ptest_data, sizeof(str_test));

	if (dev_hard_header(skb, dev, ETH_P_TEST, dest_hw, src_hw, skb->len) < 0)
		goto out;

	dev_queue_xmit(skb);
	return 0;
out: 
	kfree_skb(skb);
	return 0;
}

/* process the received data */
static int process_data(struct sk_buff *skb)
{
	str_test *ptemp_test = (str_test *)skb->data;
	str_test temp_test;
	/* if echo request(ECHO_REQ) received. Then revert the same (ECHO_REPLY) */
	if(ptemp_test->type == ECHO_REQ)
	{
		memset(&temp_test, 0, sizeof(str_test));
		memcpy(&temp_test, skb->data, sizeof(str_test));
		kfree_skb(skb);
		temp_test.type = ECHO_REPLY;
		send_test(&temp_test);
	}
	else /* otherwise queue it, so that user space app will consume it */
	{
		skb_queue_tail(&skb_q, skb);
	}

	return 0;
}
/* Receive callback, called by netif_recv_skb() if protocol is ETH_P_TEST */
static int test_rcv(struct sk_buff *skb, struct net_device *dev,
		struct packet_type *pt, struct net_device *orig_dev)
{
	str_test* test_data;
	printk("skb->proto = %02x\n", ntohs(skb->protocol));
	test_data = (str_test *)skb->data;
	printk("count = %d\ntype = %d\ndata = %s\n", test_data->count, test_data->type, test_data->data);
	/* process received data, put it in queue or revert with echo_reply */
	process_data(skb);
	return 0;
}

long test_dev_ioctl(struct file *filep, unsigned int cmd, unsigned long arg) {
	struct sk_buff *skb = NULL;
	str_test *temp_test_data;
	int len = sizeof(str_test);

	switch(cmd) {
		case READ_IOCTL:
			skb = skb_dequeue(&skb_q);
			if(skb) {
				temp_test_data = (str_test*)skb->data;
				if(copy_to_user((char *)arg, temp_test_data, sizeof(str_test)))
				{
					printk("copy_to_user failed \n");
					len = -1;
				}
				kfree_skb(skb);
			}
		case WRITE_IOCTL:
			temp_test_data = kmalloc(sizeof(str_test), GFP_ATOMIC);
			if(copy_from_user(temp_test_data, (char *)arg, sizeof(str_test)))
			{
				printk("copy_from_user \n");
				kfree(temp_test_data);
				return -1;
			}
			send_test(temp_test_data);
			kfree(temp_test_data);
	}
	return len;
}

static struct packet_type test_packet_type __read_mostly = {
	.type = cpu_to_be16(ETH_P_TEST),
	.func = test_rcv,
};

static struct file_operations fops = {
	.read = NULL,
	.write = NULL,
	.unlocked_ioctl = test_dev_ioctl,
};

static int __init test_module_init(void)
{
	major = register_chrdev(0, "test_dev", &fops);
	if(major < 0) {
		printk("registering the character device failed with = %d\n", major);
	}
	dev_add_pack(&test_packet_type);
	skb_queue_head_init(&skb_q);
	printk("module test inserted\n");
	return 0;
}

static void __exit test_module_exit(void)
{
	dev_remove_pack(&test_packet_type);
	unregister_chrdev(major, "test_dev");
	printk("module test removed\n");
}

module_init(test_module_init);
module_exit(test_module_exit);
MODULE_LICENSE("GPL");
