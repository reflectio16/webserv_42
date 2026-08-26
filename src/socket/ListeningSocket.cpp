/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ListeningSocket.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: meelma <meelma@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/26 17:29:41 by meelma            #+#    #+#             */
/*   Updated: 2026/08/26 17:29:44 by meelma           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ListeningSocket.hpp"

#include <sys/socket.h>   // socket, setsockopt, bind, listen, SOMAXCONN
#include <netdb.h>        // getaddrinfo, freeaddrinfo, gai_strerror, addrinfo
#include <unistd.h>       // close
#include <fcntl.h>        // fcntl, F_SETFL, O_NONBLOCK
#include <cstring>        // memset
#include <sstream>        // ostringstream (int -> string, C++98-friendly)
#include <iostream>       // cerr

int makeListeningSocket(const std::string& host, int port)
{
    // getaddrinfo wants the port as a C string.
    std::ostringstream portStream;
    portStream << port;
    std::string portStr = portStream.str();

    // Describe the kind of socket we want: IPv4, TCP, intended for bind().
    struct addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = AI_PASSIVE;

    struct addrinfo* res = NULL;
    int gai = getaddrinfo(host.c_str(), portStr.c_str(), &hints, &res);
    if (gai != 0)
    {
        std::cerr << "getaddrinfo(" << host << ":" << port << "): "
                  << gai_strerror(gai) << std::endl;
        return -1;
    }

    int fd  = -1;
    int yes = 1;

    // getaddrinfo returns a linked list of candidate addresses. Walk it and
    // keep the first one we can successfully create AND bind a socket on.
    for (struct addrinfo* p = res; p != NULL; p = p->ai_next)
    {
        fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd < 0)
            continue;

        // SO_REUSEADDR, set BEFORE bind: without it, restarting the server
        // fails with "Address already in use" while the old port lingers in
        // TIME_WAIT. This is the single most important line here.
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
        {
            close(fd);
            fd = -1;
            continue;
        }

        if (bind(fd, p->ai_addr, p->ai_addrlen) == 0)
            break;              // bound — done

        close(fd);              // this candidate failed; try the next
        fd = -1;
    }

    freeaddrinfo(res);          // always release the list

    if (fd < 0)
    {
        std::cerr << "bind failed on " << host << ":" << port << std::endl;
        return -1;
    }

    // Passive mode: the kernel now completes handshakes and queues clients.
    if (listen(fd, SOMAXCONN) < 0)
    {
        std::cerr << "listen failed on " << host << ":" << port << std::endl;
        close(fd);
        return -1;
    }

    // Non-blocking, so accept() in the event loop never stalls the one thread.
    // 42 rule: for sockets, fcntl may be used ONLY in exactly this form.
    if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
    {
        std::cerr << "fcntl failed on " << host << ":" << port << std::endl;
        close(fd);
        return -1;
    }

    return fd;
}
