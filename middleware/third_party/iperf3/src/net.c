/*
 * iperf, Copyright (c) 2014, 2015, The Regents of the University of
 * California, through Lawrence Berkeley National Laboratory (subject
 * to receipt of any required approvals from the U.S. Dept. of
 * Energy).  All rights reserved.
 *
 * If you have questions about your rights to use or distribute this
 * software, please contact Berkeley Lab's Technology Transfer
 * Department at TTD@lbl.gov.
 *
 * NOTICE.  This software is owned by the U.S. Department of Energy.
 * As such, the U.S. Government has been granted for itself and others
 * acting on its behalf a paid-up, nonexclusive, irrevocable,
 * worldwide license in the Software to reproduce, prepare derivative
 * works, and perform publicly and display publicly.  Beginning five
 * (5) years after the date permission to assert copyright is obtained
 * from the U.S. Department of Energy, and subject to any subsequent
 * five (5) year renewals, the U.S. Government is granted for itself
 * and others acting on its behalf a paid-up, nonexclusive,
 * irrevocable, worldwide license in the Software to reproduce,
 * prepare derivative works, distribute copies to the public, perform
 * publicly and display publicly, and to permit others to do so.
 *
 * This code is distributed under a BSD style license, see the LICENSE
 * file for complete information.
 */

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/errno.h>
//#include <netinet/in.h>
//#include <netinet/tcp.h>
#include <assert.h>
#include <netdb.h>
#include <string.h>
#include <sys/fcntl.h>
#include <lwip/inet.h>   // <arpa/inet.h>

#ifdef linux
#include <sys/sendfile.h>
#else
#ifdef __FreeBSD__
#include <sys/uio.h>
#else
#if defined(__APPLE__) && defined(__MACH__)	/* OS X */
#include <AvailabilityMacros.h>
#if defined(MAC_OS_X_VERSION_10_6)
#include <sys/uio.h>
#endif
#endif
#endif
#endif

#include "iperf_util.h"
#include "net.h"
#include "timer.h"
#include "iperf.h"
#include "iperf_timer.h"

/* Possible values for `ai_flags' field in `addrinfo' structure.  */
#ifndef AI_PASSIVE
    # define AI_PASSIVE	0x0001	/* Socket address is intended for `bind'.  */
#endif

#ifndef AI_CANONNAME
    # define AI_CANONNAME	0x0002	/* Request for canonical name.  */
#endif

#ifndef AI_NUMERICHOST
# define AI_NUMERICHOST	0x0004	/* Don't use name resolution.  */
#endif

#ifndef AI_V4MAPPED
# define AI_V4MAPPED	0x0008	/* IPv4 mapped addresses are acceptable.  */
#endif

#ifndef AI_ALL
# define AI_ALL		0x0010	/* Return IPv4 mapped and IPv6 addresses.  */
#endif

#ifndef AI_ADDRCONFIG
# define AI_ADDRCONFIG	0x0020	/* Use configuration of this host to choose
				   returned address type..  */
#endif

/* netdial and netannouce code comes from libtask: http://swtch.com/libtask/
 * Copyright: http://swtch.com/libtask/COPYRIGHT
*/

/* make connection to server */
int
netdial(int domain, int proto, char *local, char *server, int port)
{
    struct addrinfo hints, *local_res, *server_res;
    int s, ret = -1;

#if IPERF3_DEBUG
    printf("[iperf_test][netdial], domain = %d, proto = %d, local = %s, server = %s, port = %d.\n", domain, proto, local, server, port);
#endif

    if (local) {
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = domain;
        hints.ai_socktype = proto;
        if (getaddrinfo(local, NULL, &hints, &local_res) != 0)
            return -1;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = domain;
    hints.ai_socktype = proto;
    if (getaddrinfo(server, NULL, &hints, &server_res) != 0)
        return -2;

    s = socket(server_res->ai_family, proto, 0);
    if (s < 0) {
	if (local)
	    freeaddrinfo(local_res);
	freeaddrinfo(server_res);
#if IPERF3_DEBUG
    printf("[iperf_test][netdial]fail to socket, ai_family = %d, proto, = %d.\n", (int)server_res->ai_family, (int)proto);
#endif
        return -3;
    }

    if (local) {
        if (bind(s, (struct sockaddr *) local_res->ai_addr, local_res->ai_addrlen) < 0) {
	    close(s);
	    freeaddrinfo(local_res);
	    freeaddrinfo(server_res);
            return -4;
	}
        freeaddrinfo(local_res);
    }

    ((struct sockaddr_in *) server_res->ai_addr)->sin_port = htons(port);
    ret = connect(s, (struct sockaddr *) server_res->ai_addr, server_res->ai_addrlen);

    if (ret < 0 && errno != EINPROGRESS) {
	close(s);
	freeaddrinfo(server_res);
        return -5;
    }

#if IPERF3_DEBUG
    printf("[iperf_debug]netdial - 2, ret = %d, errno = %d, EINPROGRESS = %d.\n", ret, errno, EINPROGRESS);
    printf("[iperf_debug]netdial - 3, ai_addrlen = %d, sa_len = %d, sa_family = %d, sa_data = %s.\n", (int)server_res->ai_addrlen, (int)((struct sockaddr *) server_res->ai_addr)->sa_len, (int)((struct sockaddr *) server_res->ai_addr)->sa_family, (char*)((struct sockaddr *) server_res->ai_addr)->sa_data);
#endif

    freeaddrinfo(server_res);
    return s;
}

/***************************************************************/

int
netannounce(int domain, int proto, char *local, int port)
{
    struct addrinfo hints, *res;
    char portstr[6];
    int s, opt = 1;
    int ret = 0;
    struct sockaddr_in addr;

#if defined(IPV6_V6ONLY) && !defined(__OpenBSD__)
    int opt;
#endif

#if IPERF3_DEBUG
    printf("[iperf_test][netannounce]begin, domain = %d, proto = %d, local = %s, port = %d.\n", port, domain, local, proto);
#endif

    snprintf(portstr, 6, "%d", port);
    memset(&hints, 0, sizeof(hints));
    /* 
     * If binding to the wildcard address with no explicit address
     * family specified, then force us to get an AF_INET6 socket.  On
     * CentOS 6 and MacOS, getaddrinfo(3) with AF_UNSPEC in ai_family,
     * and ai_flags containing AI_PASSIVE returns a result structure
     * with ai_family set to AF_INET, with the result that we create
     * and bind an IPv4 address wildcard address and by default, we
     * can't accept IPv6 connections.
     *
     * On FreeBSD, under the above circumstances, ai_family in the
     * result structure is set to AF_INET6.
     */
    if (domain == AF_UNSPEC && !local) {
	hints.ai_family = AF_INET6;
    }
    else {
	hints.ai_family = domain;
    }
    hints.ai_socktype = proto;
    hints.ai_flags = AI_PASSIVE;
    if (getaddrinfo(local, portstr, &hints, &res) != 0)
        return -1; 

    s = socket(res->ai_family, proto, 0);
    if (s < 0) {
	freeaddrinfo(res);
        return -2;
    }

#if defined(IPV6_V6ONLY) && !defined(__OpenBSD__)
    opt = 1;
#endif

    if ((ret = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, 
		   (char *) &opt, sizeof(opt))) < 0) {
#if IPERF3_DEBUG
    printf("[iperf_test][netannounce]ERROR: setsockopt SO_REUSEADDR, ret= %d, opt = %d.\n", (int)ret, (int)opt);
#endif
#if 0
	close(s);
	freeaddrinfo(res);
	return -3;
#endif
    }


    /*
     * If we got an IPv6 socket, figure out if it should accept IPv4
     * connections as well.  We do that if and only if no address
     * family was specified explicitly.  Note that we can only
     * do this if the IPV6_V6ONLY socket option is supported.  Also,
     * OpenBSD explicitly omits support for IPv4-mapped addresses,
     * even though it implements IPV6_V6ONLY.
     */
#if defined(IPV6_V6ONLY) && !defined(__OpenBSD__)
    if (res->ai_family == AF_INET6 && (domain == AF_UNSPEC || domain == AF_INET6)) {
	if (domain == AF_UNSPEC)
	    opt = 0;
	else
	    opt = 1;
	if (setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, 
		       (char *) &opt, sizeof(opt)) < 0) {
	    close(s);
	    freeaddrinfo(res);
	    return -4;
	}
    }
#endif /* IPV6_V6ONLY */

#if IPERF3_DEBUG
    {
    int i;
    printf("[iperf_test][netannounce]bind, s = %d, addrlen = %d, addr->sa_len = %d, addr->sa_family = %d, add->as_data = %s \n", s, (int)res->ai_addrlen, (int)res->ai_addr->sa_len, (int)res->ai_addr->sa_family, res->ai_addr->sa_data);
    printf("[iperf_test][netannounce]bind add->as_data: ");
    for (i = 0; i < res->ai_addr->sa_len; i++)
    {
        printf("%d, ", (int)res->ai_addr->sa_data[i]);
    }
    printf("\n");
    }
#endif

////////////////temp solution
//res->ai_addr->sa_data[2]= 192;
//res->ai_addr->sa_data[3]= 168;
//res->ai_addr->sa_data[4]= 31;
//res->ai_addr->sa_data[5]= 240;
/////////////////////////////
    memset(&addr, 0, sizeof(addr));
    addr.sin_len = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_port = PP_HTONS(port);
    addr.sin_addr.s_addr = IPADDR_ANY;

    //if (bind(s, (struct sockaddr *) res->ai_addr, res->ai_addrlen) < 0) {
    if (bind(s, (struct sockaddr *) &addr, res->ai_addrlen) < 0) {
        close(s);
	freeaddrinfo(res);
        return -5;
    }

    freeaddrinfo(res);
    
    if (proto == SOCK_STREAM) {
        if (listen(s, 5) < 0) {
	    close(s);
            return -6;
        }
    }
#if IPERF3_DEBUG
    printf("[iperf_test][netannounce]end, s = %d\n", s);
#endif
    return s;
}


/*******************************************************************/
/* reads 'count' bytes from a socket  */
/********************************************************************/

int
Nread(int fd, char *buf, size_t count, int prot)
{
    register ssize_t r;
    register size_t nleft = count;

#if IPERF3_DEBUG
    printf("[iperf_test][Nread], fd = %d, buf = %d, count = %d, prot = %d.\n", fd, (int)(*buf), (int)count, prot);
#endif

    while (nleft > 0) {
#if IPERF3_DEBUG_TEMP
       {
        int result = 0;
        fd_set read_set;
       
        struct timeval now;
        now.tv_sec = 50;
        now.tv_usec = 500000;
        FD_ZERO(&read_set);
        FD_SET(fd+1, &read_set);
        result = select(fd + 1, &read_set, NULL, NULL, &now);
        if(result < 0)
        {
            printf("nread select error :%d\r\n", result);
            break;
        }
        }
#endif
        r = read(fd, buf, nleft);

#if IPERF3_DEBUG
    printf("[iperf_test][Nread], fd = %d, buf = %s(%d), nleft = %d, r = %d, errno = %d.\n", fd, buf, (int)(*buf), (int)nleft, (int)r, (int)errno);
#endif

        if (r < 0) {
            if (errno == EINTR || errno == EAGAIN)
                break;
            else
                return NET_HARDERROR;
        } else if (r == 0)
            break;

        nleft -= r;
        buf += r;
    }
#if IPERF3_DEBUG
    printf("[iperf_test][Nread], count - nleft = %d.\n", (int)(count - nleft));
#endif
    return count - nleft;
}


/*
 *                      N W R I T E
 */

int
Nwrite(int fd, const char *buf, size_t count, int prot)
{
    register ssize_t r;
    register size_t nleft = count;

    while (nleft > 0) {
#if IPERF3_DEBUG_TEMP
       {
        int result = 0;
        struct timeval now;
        fd_set write_set;
        now.tv_sec = 50;
        now.tv_usec = 500000;
        FD_ZERO(&write_set);
        FD_SET(fd+1, &write_set);
        result = select(fd + 1, NULL, &write_set, NULL, &now);
        if(result < 0)
        {
            printf("nwrite select error :%d\r\n", result);
            break;
        }
        }
#endif
	r = write(fd, buf, nleft);
	if (r < 0) {
	    switch (errno) {
		case EINTR:
		case EAGAIN:
		return count - nleft;

		case ENOBUFS:
		return NET_SOFTERROR;

		default:
		return NET_HARDERROR;
	    }
	} else if (r == 0)
	    return NET_SOFTERROR;
	nleft -= r;
	buf += r;
    }
    return count;
}


int
has_sendfile(void)
{
#ifdef linux
    return 1;
#else
#ifdef __FreeBSD__
    return 1;
#else
#if defined(__APPLE__) && defined(__MACH__) && defined(MAC_OS_X_VERSION_10_6)	/* OS X */
    return 1;
#else
    return 0;
#endif
#endif
#endif
}


/*
 *                      N S E N D F I L E
 */

int
Nsendfile(int fromfd, int tofd, const char *buf, size_t count)
{
    off_t offset;
#if defined(__FreeBSD__) || (defined(__APPLE__) && defined(__MACH__) && defined(MAC_OS_X_VERSION_10_6))
    off_t sent;
#endif
    register size_t nleft;
    register ssize_t r;

    nleft = count;
    while (nleft > 0) {
	offset = count - nleft;
#ifdef linux
	r = sendfile(tofd, fromfd, &offset, nleft);
	if (r > 0)
	    nleft -= r;
#elif defined(__FreeBSD__)
	r = sendfile(fromfd, tofd, offset, nleft, NULL, &sent, 0);
	nleft -= sent;
#elif defined(__APPLE__) && defined(__MACH__) && defined(MAC_OS_X_VERSION_10_6)	/* OS X */
	sent = nleft;
	r = sendfile(fromfd, tofd, offset, &sent, NULL, 0);
	nleft -= sent;
#else
	/* Shouldn't happen. */
	r = -1;
	errno = ENOSYS;
#endif

    if (offset)
    {
        offset = 0;
    }
	if (r < 0) {
	    switch (errno) {
		case EINTR:
		case EAGAIN:
		if (count == nleft)
		    return NET_SOFTERROR;
		return count - nleft;

		case ENOBUFS:
		case ENOMEM:
		return NET_SOFTERROR;

		default:
		return NET_HARDERROR;
	    }
	}
#ifdef linux
	else if (r == 0)
	    return NET_SOFTERROR;
#endif
    }
    return count;
}

/*************************************************************************/

/**
 * getsock_tcp_mss - Returns the MSS size for TCP
 *
 */

int
getsock_tcp_mss(int inSock)
{
    int             mss = 0;

    int             rc = 0;
    //socklen_t       len;

    assert(inSock >= 0); /* print error and exit if this is not true */

    /* query for mss */
    //len = sizeof(mss);
    //rc = getsockopt(inSock, IPPROTO_TCP, TCP_MAXSEG, (char *)&mss, &len);
    if (rc == -1) {
	perror("getsockopt TCP_MAXSEG");
	return -1;
    }

    return mss;
}



/*************************************************************/

/* sets TCP_NODELAY and TCP_MAXSEG if requested */
// XXX: This function is not being used.

int
set_tcp_options(int sock, int no_delay, int mss)
{
    socklen_t len;
    int rc = 0;
    //int new_mss;

    if (no_delay == 1) {
        len = sizeof(no_delay);
        rc = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&no_delay, len);
        if (rc == -1) {
            perror("setsockopt TCP_NODELAY");
            return -1;
        }
    }
#ifdef TCP_MAXSEG
    if (mss > 0) {
        len = sizeof(new_mss);
        assert(sock != -1);

        /* set */
        new_mss = mss;
        len = sizeof(new_mss);
        //rc = setsockopt(sock, IPPROTO_TCP, TCP_MAXSEG, (char *)&new_mss, len);
        if (rc == -1) {
            perror("setsockopt TCP_MAXSEG");
            return -1;
        }
        /* verify results */
        //rc = getsockopt(sock, IPPROTO_TCP, TCP_MAXSEG, (char *)&new_mss, &len);
        if (rc == -1) {
            perror("getsockopt TCP_MAXSEG");
            return -1;
        }
        if (new_mss != mss) {
            perror("setsockopt value mismatch");
            return -1;
        }
    }
#endif
    return 0;
}

/****************************************************************************/

int
setnonblocking(int fd, int nonblocking)
{
#if IPERF3_DEBUG
    printf("[iperf_test][setnonblocking]nonblocking = %d\n", nonblocking);
#endif
#if(0)
    if (nonblocking)
    {
        return( fcntl( fd, F_SETFL, fcntl( fd, F_GETFL, 0 ) | O_NONBLOCK ) );
    }
    else
    {
        return( fcntl( fd, F_SETFL, fcntl( fd, F_GETFL, 0 ) & ~O_NONBLOCK ) );
    }
#endif


    int flags, newflags;

    flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        perror("fcntl(F_GETFL)");
        return -1;
    }
    if (nonblocking)
	newflags = flags | (int) O_NONBLOCK;
    else
	newflags = flags & ~((int) O_NONBLOCK);
    if (newflags != flags)
	if (fcntl(fd, F_SETFL, newflags) < 0) {
	    perror("fcntl(F_SETFL)");
	    return -1;
	}
    return 0;

    
}

/****************************************************************************/

int
getsockdomain(int sock)
{
    struct sockaddr sa;
    socklen_t len = sizeof(sa);

    if (getsockname(sock, &sa, &len) < 0)
	return -1;
    return sa.sa_family;
}
