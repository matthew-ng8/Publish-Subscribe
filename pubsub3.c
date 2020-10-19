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

struct node {//struct to be inserted into linked list??
     int pid ;//this is the PID
     struct list_head mylist ;
} ;

struct list_head head_pub, head_sub;//global head, initialized in __init hello_init

static void hello_nl_recv_msg(struct sk_buff *skb)
{

    struct nlmsghdr *nlh;
	struct sk_buff *skb_out;
	int msg_size;
	char *msg = "Hello from kernel";
	char* pub_msg;
	int res;

	struct list_head *ptr;//ptr for iterations
	struct node *newNode, *node_entry;
	int unique;

	printk(KERN_INFO "Entering: %s\n", __FUNCTION__);

    	msg_size = strlen(msg);

    	nlh = (struct nlmsghdr *)skb->data;
    	printk(KERN_INFO "Netlink received msg payload:%s\n", (char *)nlmsg_data(nlh));
		pub_msg = (char *)nlmsg_data(nlh);
		msg_size = strlen(pub_msg);
	//msg =nlmsg_data(nlh);

	pid = nlh->nlmsg_pid; /*pid of publishing process */
	printk(KERN_INFO "Process PID = :%d\n", pid);//line i added
	
	unique = 1;
	if(pub_msg[0] == '0'){//its a publisher!
		pidpub = pid;
		printk(KERN_INFO "pidpub = :%d\n", pidpub);
		list_for_each(ptr,&head_pub){//iterate through list of publishers
			node_entry=list_entry(ptr,struct node, mylist);
			//printk(KERN_INFO "\n Hello %d  \n ", entry->temp);
			if(node_entry->pid == pidpub){
				unique = 0;		
			}

		}
		if(unique){//if pid not in the list, add it
			newNode = kmalloc(sizeof(struct node*), GFP_KERNEL);
			newNode->pid = pidpub;
			list_add(&newNode->mylist, &head_pub);
		}
	}
	unique = 1;
	if(pub_msg[0] == '1'){//its a subscriber!
		pidsub1 = pid;
		printk(KERN_INFO "pidsub1 = :%d\n", pidsub1);
		list_for_each(ptr,&head_sub){//iterate through list of subscribers
			node_entry=list_entry(ptr,struct node, mylist);
			//printk(KERN_INFO "\n Hello %d  \n ", entry->temp);
			if(node_entry->pid == pidsub1){
				unique = 0;		
			}

		}
		if(unique){//if pid not in the list, add it
			newNode = kmalloc(sizeof(struct node*), GFP_KERNEL);
			newNode->pid = pidsub1;
			list_add(&newNode->mylist, &head_sub);
		}
		
	}
//--------------------PRINT TO TERMINALS!!!------------------------------------------------------------
	if(pub_msg[0] != '1' && pub_msg[0] != '0'){//if its not registering, we can broadcast to everyone
		list_for_each(ptr,&head_sub){//iterate through list of publishers
			node_entry =list_entry(ptr,struct node, mylist);
			pidsub1 = node_entry-> pid;
		
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
		/*	
		list_for_each(ptr,&head_pub){//iterate through list of publishers
			node_entry = (list_entry(ptr,struct node, mylist));
			pidpub = node_entry-> pid;
		
			skb_out = nlmsg_new(msg_size, 0);
			if (!skb_out) {
				printk(KERN_ERR "Failed to allocate new skb\n");
				  	return;
			}
			nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size, 0);
			NETLINK_CB(skb_out).dst_group = 0; //not in mcast group
			strncpy(nlmsg_data(nlh), pub_msg, msg_size);//copying the message over to the header
			res = nlmsg_unicast(nl_sk, skb_out, pidpub);//this outputs the message from what I read. 
			if (res < 0)
				printk(KERN_INFO "Error while sending bak to user\n");
		}
		*/
	}	
	

	/*
	skb_out = nlmsg_new(msg_size, 0);
	if (!skb_out) {
		printk(KERN_ERR "Failed to allocate new skb\n");
	      	return;
	}

	nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size, 0);
	NETLINK_CB(skb_out).dst_group = 0; // not in mcast group
	strncpy(nlmsg_data(nlh), msg, msg_size);//copying the message over to the header

	res = nlmsg_unicast(nl_sk, skb_out, pid);//this outputs the message from what I read. 
	if (res < 0)
		printk(KERN_INFO "Error while sending bak to user\n");
	*/
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
	INIT_LIST_HEAD(&head_sub);//initiliazing the head for pub list and sub list
	INIT_LIST_HEAD(&head_pub);

    return 0;
}

static void __exit hello_exit(void)
{
	struct node* node_entry; 
	struct list_head *ptr, *temp;
    printk(KERN_INFO "exiting hello module belgh\n");
    netlink_kernel_release(nl_sk);
	//need to free everything used here
	list_for_each(ptr,&head_pub){//iterate through list of publishers
		node_entry =list_entry(ptr,struct node, mylist);
		temp = ptr;
		kfree(node_entry);
		__list_del_entry(temp);
	}
	list_for_each(ptr,&head_sub){//iterate through list of subscribers
		node_entry =list_entry(ptr,struct node, mylist);
		temp = ptr;
		kfree(node_entry);
		__list_del_entry(temp);
	}

}

module_init(hello_init); module_exit(hello_exit);

MODULE_LICENSE("GPL");