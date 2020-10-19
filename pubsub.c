#include <linux/module.h>
#include <net/sock.h> 
#include <linux/netlink.h>
#include <linux/skbuff.h> 

//#include <linux/types.h>
//#include <linux/signal.h>

#include<linux/list.h>//allows me to create a list?
#define NETLINK_USER 31

struct sock *nl_sk = NULL;
int pid, pidpub, pidsub1, pidsub2;// i added pidpub etc

struct node {
     int data ;
     struct list_head mylist ;
} ;

static void hello_nl_recv_msg(struct sk_buff *skb)
{

    struct nlmsghdr *nlh;
	struct sk_buff *skb_out;
	int msg_size;
	char *msg = "Hello from kernel";
	char* pub_msg;
	int res;

	printk(KERN_INFO "Entering: %s\n", __FUNCTION__);

    	msg_size = strlen(msg);

    	nlh = (struct nlmsghdr *)skb->data;
    	printk(KERN_INFO "Netlink received msg payload:%s\n", (char *)nlmsg_data(nlh));
		pub_msg = (char *)nlmsg_data(nlh);
		msg_size = strlen(pub_msg);
	//msg =nlmsg_data(nlh);

	pid = nlh->nlmsg_pid; /*pid of publishing process */
	printk(KERN_INFO "Process PID = :%d\n", pid);//line i added

	//check if processes still exist to avoid overlapping?
	/*
	if (0 != kill(pidsub1, 0))
	{
    	pidsub1 = -1;
	}
	if (0 != kill(pidsub2, 0))
	{
    	pidsub2 = -1;
	}
	if (0 != kill(pidpub, 0))
	{
    	pidpub = -1;
	}
*/
	if(pidpub == -1 &&  pub_msg[0] == '0'){
		pidpub = pid;
		printk(KERN_INFO "pidpub = :%d\n", pidpub);
	}
	if(pub_msg[0] == '1'){
		if(pidsub1 == -1){
			pidsub1 = pid;
			printk(KERN_INFO "sub1 pid = :%d\n", pidsub1);		
		}
		else if(pidsub2 == -1){
			pidsub2 = pid;
			printk(KERN_INFO "sub2 pid = :%d\n", pidsub2);		
		}

	}
	
	if(pid == pidpub && pub_msg[0] != '0'){//if you are pub and not doing a meet and greet

		if(pidsub1 != -1){//if there's a sub we can send to
			skb_out = nlmsg_new(msg_size, 0);
			if (!skb_out) {
				printk(KERN_ERR "Failed to allocate new skb\n");
				  	return;
			}

			nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size, 0);
			NETLINK_CB(skb_out).dst_group = 0; /* not in mcast group */
			strncpy(nlmsg_data(nlh), pub_msg, msg_size);//copying the message over to the header

			res = nlmsg_unicast(nl_sk, skb_out, pidsub1);//this outputs the message from what I read. 
			if (res < 0)
				printk(KERN_INFO "Error while sending bak to user\n");

		}
		if(pidsub2 != -1){//if there's a second one we can write to
			skb_out = nlmsg_new(msg_size, 0);
			if (!skb_out) {
				printk(KERN_ERR "Failed to allocate new skb\n");
				  	return;
			}

			nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size, 0);
			NETLINK_CB(skb_out).dst_group = 0; /* not in mcast group */
			strncpy(nlmsg_data(nlh), pub_msg, msg_size);//copying the message over to the header

			res = nlmsg_unicast(nl_sk, skb_out, pidsub2);//this outputs the message from what I read. 
			if (res < 0)
				printk(KERN_INFO "Error while sending bak to user\n");


		}

	}


	skb_out = nlmsg_new(msg_size, 0);
	if (!skb_out) {
		printk(KERN_ERR "Failed to allocate new skb\n");
	      	return;
	}

	nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size, 0);
	NETLINK_CB(skb_out).dst_group = 0; /* not in mcast group */
	strncpy(nlmsg_data(nlh), msg, msg_size);//copying the message over to the header

	res = nlmsg_unicast(nl_sk, skb_out, pid);//this outputs the message from what I read. 
	if (res < 0)
		printk(KERN_INFO "Error while sending bak to user\n");
}

static int __init hello_init(void)
{

    printk("Entering: %s\n", __FUNCTION__);
    struct netlink_kernel_cfg cfg = {
        .input = hello_nl_recv_msg,
    };

    nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);
    if (!nl_sk) {
        printk(KERN_ALERT "Error creating socket.\n");
        return -10;
    }
	pidpub = pidsub1 = pidsub2 = -1;//i think i can make it so taht the pids are -1 the process doesn't exist

    return 0;
}

static void __exit hello_exit(void)
{

    printk(KERN_INFO "exiting hello module belgh\n");
    netlink_kernel_release(nl_sk);
	//need to free everything used here
}

module_init(hello_init); module_exit(hello_exit);

MODULE_LICENSE("GPL");