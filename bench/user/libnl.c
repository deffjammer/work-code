/*
 *  Copyright (c) 2007 - 2010 Silicon Graphics, Inc.
 *  All rights reserved.
 *
 *  Derek L. Fults <dfults@sgi.com
 */

#include <sys/types.h>
#include <unistd.h>
#include <asm/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>


/**
 * libnetlink_create_socket - Create a netlink socket
 * @id: identificator of the new netlink socket
 *
 * Up to 32 netlink sockets could be register, this parameter is set in
 * compilation time and it could be increased. Some id's are already in
 * use, see netlink.h in kernel sources.
 */
int
libnetlink_create_socket(int id)
{
	int fd;

	fd = socket(PF_NETLINK, SOCK_RAW, id);
	if (fd < 0) {
		perror("libnetlink_create_socket:");
		return(-1);
	}

	return(fd);
}

/**
 * libnetlink_bind - Bind a descriptor to their sockets structures
 * @fd: descriptor assoaciated to the socket
 * @local: structure which contains the information of the local peer
 * @peer: structure which contains the information of the remote peer
 *
 * FIXME: since we are working with broadcast netlink sockets, group
 * should be passed as parameter!!
 */

int
libnetlink_bind(int fd, struct sockaddr_nl *local, struct sockaddr_nl *peer)
{
	int status;
	
	memset(local, 0, sizeof(struct sockaddr_nl));
	local->nl_family = AF_NETLINK;
	local->nl_groups = 0;
	local->nl_pid = getpid();

	memset(peer, 0, sizeof(struct sockaddr_nl));
	peer->nl_family = AF_NETLINK;

	peer->nl_groups = 0;
	peer->nl_pid = 0;
	
	status = bind(fd, (struct sockaddr *) local, sizeof(*local));
	if (status == -1) {
		perror("bind:");
		close(fd);
		return(-1);
	}

	return(1);
}
	
/**
 * libnetlink_send_netlink_message - send a netlink message to kernel
 * @fd: descriptor associated to the socket
 * @data: payload of the message
 * @size: size of the payload
 */
int
libnetlink_send_netlink_message(int fd, void *data, int size)
{
	struct nlmsghdr nlh;
	struct msghdr msg;
	struct iovec iov[2];
        size_t tlen;
	int ret;

        memset(&nlh, 0, sizeof(nlh));
        nlh.nlmsg_flags = NLM_F_REQUEST;
        nlh.nlmsg_type = 0x20;
        nlh.nlmsg_pid = getpid();

        iov[0].iov_base = &nlh;
        iov[0].iov_len = sizeof(nlh);
        iov[1].iov_base = data;
        iov[1].iov_len = size;
	
        tlen = sizeof(nlh) + size;

        //msg.msg_name = (void *) &peer;
        //msg.msg_namelen = sizeof(peer);
	msg.msg_name = NULL;
	msg.msg_namelen = 0;
        msg.msg_iov = iov;
        msg.msg_iovlen = 2;
        msg.msg_control = NULL;
        msg.msg_controllen = 0;
        msg.msg_flags = 0;
        nlh.nlmsg_len = tlen;

	if ((ret = sendmsg(fd, &msg, 0)) < 0)
		perror("sendmsg:");
		
        return(ret);
}
