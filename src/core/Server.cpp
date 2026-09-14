/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: meelma <meelma@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/14 14:18:37 by meelma            #+#    #+#             */
/*   Updated: 2026/09/14 14:18:39 by meelma           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include "ListeningSocket.hpp"

#include <sys/socket.h>   // accept, recv
#include <sys/types.h>    // ssize_t
#include <unistd.h>       // close
#include <fcntl.h>        // fcntl, F_SETFL, O_NONBLOCK
#include <csignal>        // signal, SIGPIPE, SIG_IGN
#include <sstream>        // ostringstream
#include <stdexcept>      // runtime_error
#include <iostream>
#include <set>
#include <utility>        // pair, make_pair

Server::Server(const std::string& configPath)
    : _config(configPath)      // Config parses the file here; throws on bad config
{
    // A client or CGI child that vanishes mid-write would raise SIGPIPE, whose
    // default action KILLS the process. Ignore it and handle the failed write
    // via send()'s return value instead. One line, saves the whole server.
    signal(SIGPIPE, SIG_IGN);

    setupListeners();
}

Server::~Server() {
    // Close every fd still open (listeners + any live clients). Connection has
    // no destructor that closes fds, so this is the single owner of fd lifetime.
    for (size_t i = 0; i < _pfds.size(); ++i)
        close(_pfds[i].fd);
}

// ---- startup ---------------------------------------------------------------

void Server::setupListeners() {
    std::vector<Endpoint> endpoints = _config.getEndpoints();
    std::set<std::pair<std::string, int> > bound;   // note the space: > >, not >>

    for (size_t i = 0; i < endpoints.size(); ++i) {
        std::pair<std::string, int> key(endpoints[i].host, endpoints[i].port);
        if (bound.count(key))                       // dedup: two server blocks may
            continue;                               //   share a host:port -> one socket

        int fd = makeListeningSocket(endpoints[i].host, endpoints[i].port);
        if (fd < 0) {
            std::ostringstream oss;
            oss << "failed to bind " << endpoints[i].host << ":" << endpoints[i].port;
            throw std::runtime_error(oss.str());
        }

        addToPoll(fd, POLLIN, LISTENING);
        bound.insert(key);
        std::cout << "listening on " << endpoints[i].host
                  << ":" << endpoints[i].port << std::endl;
    }

    if (_pfds.empty())
        throw std::runtime_error("no listening sockets created");
}

// ---- the event loop --------------------------------------------------------

void Server::run() {
    while (true) {
        int ready = poll(&_pfds[0], _pfds.size(), -1);
        if (ready <= 0)
            continue;   // interrupted or nothing ready; robust handling comes later

        // Snapshot the ready fds BEFORE dispatching. accept/close mutate _pfds
        // (adding/removing entries), so iterating _pfds directly while mutating
        // it would skip or double-process. Copy (fd, revents), then act.
        std::vector<std::pair<int, short> > readyFds;
        for (size_t i = 0; i < _pfds.size(); ++i)
            if (_pfds[i].revents != 0)
                readyFds.push_back(std::make_pair(_pfds[i].fd, _pfds[i].revents));

        for (size_t i = 0; i < readyFds.size(); ++i)
            dispatch(readyFds[i].first, readyFds[i].second);
    }
}

void Server::dispatch(int fd, short revents) {
    std::map<int, FdRole>::iterator it = _roles.find(fd);
    if (it == _roles.end())
        return;   // this fd was already closed earlier in the same round

    if (it->second == LISTENING) {
        if (revents & POLLIN)
            acceptClient(fd);
        return;
    }

    // CLIENT fd
    if (revents & (POLLERR | POLLHUP | POLLNVAL)) {   // socket error / hang-up
        closeConnection(fd);
        return;
    }
    if (revents & POLLIN) {
        std::map<int, Connection>::iterator cit = _conns.find(fd);
        if (cit != _conns.end())
            onReadable(cit->second);
    }
    // POLLOUT handling arrives with the write path (next milestone)
}

void Server::acceptClient(int listenFd) {
    // One accept per POLLIN: with level-triggered poll we'll simply be told
    // again if more clients are queued. Avoids needing to read errno.
    int clientFd = accept(listenFd, NULL, NULL);
    if (clientFd < 0)
        return;

    // A freshly accepted socket is NOT non-blocking by default -- make it so,
    // or a stalled client could block the one thread.
    if (fcntl(clientFd, F_SETFL, O_NONBLOCK) < 0) {
        close(clientFd);
        return;
    }

    _conns.insert(std::make_pair(clientFd, Connection(clientFd)));
    addToPoll(clientFd, POLLIN, CLIENT);
    std::cout << "[+] client connected (fd " << clientFd << ")" << std::endl;
}

void Server::onReadable(Connection& conn) {
    char buf[4096];
    ssize_t n = recv(conn.fd, buf, sizeof(buf), 0);   // ONE recv per readiness

    if (n <= 0) {                 // 0 = client closed; <0 = gone (no errno check)
        closeConnection(conn.fd);
        return;
    }

    conn.inbuf.append(buf, static_cast<size_t>(n));   // connection owns the tape

    // --- next milestone plugs in here: feed the parser, and on a COMPLETE
    //     request build a response, queueResponse(), watchFor(fd, POLLOUT). ---
    std::cout << "[fd " << conn.fd << "] received " << n
              << " bytes (total buffered: " << conn.inbuf.size() << ")" << std::endl;
}

void Server::closeConnection(int fd) {
    close(fd);              // 1. close the socket
    removeFromPoll(fd);     // 2. drop it from the poll set
    _roles.erase(fd);       // 3. forget its role
    _conns.erase(fd);       // 4. destroy its Connection (fd already closed)
    std::cout << "[-] closed fd " << fd << std::endl;
}

// ---- poll-set bookkeeping --------------------------------------------------

void Server::addToPoll(int fd, short events, FdRole role) {
    struct pollfd pfd;
    pfd.fd      = fd;
    pfd.events  = events;
    pfd.revents = 0;
    _pfds.push_back(pfd);
    _roles[fd] = role;
}

void Server::removeFromPoll(int fd) {
    for (size_t i = 0; i < _pfds.size(); ++i) {
        if (_pfds[i].fd == fd) {
            _pfds.erase(_pfds.begin() + i);
            return;
        }
    }
}

void Server::watchFor(int fd, short events) {
    for (size_t i = 0; i < _pfds.size(); ++i) {
        if (_pfds[i].fd == fd) {
            _pfds[i].events = events;   // overwrite the mask (POLLIN <-> POLLOUT)
            return;
        }
    }
}

