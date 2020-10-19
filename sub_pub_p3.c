#include <linux/netlink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#define NETLINK_USER 31

#define MAX_PAYLOAD 1024 /* maximum payload size*/
struct sockaddr_nl src_addr, dest_addr;
struct nlmsghdr *nlh = NULL;
struct iovec iov;
int sock_fd;
struct msghdr msg;

pthread_t tid[2];//slapping this up here b/c maybe needs to be global?
//idk if there will be an issue with the other ones.....in terms of netlink

void *publish(void *vargp){
	strcpy(NLMSG_DATA(nlh), "0");//say hey i a publisher
	printf("Sending message to kernel\n");
    sendmsg(sock_fd, &msg, 0);
	printf("Message sent, enter words to broadcast\n");
	
	//start of stuff i wrote
	char input[MAX_PAYLOAD];
	while(1){
		fgets(input, MAX_PAYLOAD, stdin);
		strcpy(NLMSG_DATA(nlh), input);
		sendmsg(sock_fd, &msg, 0);
	}
}

void *subscribe(void *vargp){
	strcpy(NLMSG_DATA(nlh), "1");//say hey i am a subscriber
	printf("Sending message to kernel\n");
    sendmsg(sock_fd, &msg, 0);//establish contact with the kernel

    printf("Waiting for message from kernel\n");
	while(1){
    	recvmsg(sock_fd, &msg, 0);
    	printf("Received message payload: %s\n", NLMSG_DATA(nlh));
	}


}

int main()
{
    sock_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_USER);
    if (sock_fd < 0)
        return -1;

    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid(); /* self pid */

    bind(sock_fd, (struct sockaddr *)&src_addr, sizeof(src_addr));

    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = 0; /* For Linux Kernel */
    dest_addr.nl_groups = 0; /* unicast */

    nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
    memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
    nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
    nlh->nlmsg_pid = getpid();
    nlh->nlmsg_flags = 0;

    //strcpy(NLMSG_DATA(nlh), "0");//say hey i a publisher

    iov.iov_base = (void *)nlh;
    iov.iov_len = nlh->nlmsg_len;
    msg.msg_name = (void *)&dest_addr;
    msg.msg_namelen = sizeof(dest_addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

	//make the two threads
	//printf("HELLO????");
 	pthread_create(&tid[0], NULL, &subscribe, NULL);
	pthread_create(&tid[1], NULL, &publish, NULL);
	pthread_join(tid[1], NULL);//waitinf for it to finsih
	pthread_join(tid[0], NULL);
	//printf("BACK FROM THREADS???");

    
	
    close(sock_fd);

}