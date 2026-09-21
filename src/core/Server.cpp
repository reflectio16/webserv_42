/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: meelma <meelma@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/14 14:18:37 by meelma            #+#    #+#             */
/*   Updated: 2026/09/21 15:56:21 by meelma           ###   ########.fr       */
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


// Guard so a client whose headers never end can't grow inbuf without bound.
static const std::size_t MAX_REQUEST_BYTES = 64 * 1024;

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
    std::vector<Endpoint> endpoints = _config.getEndpoints(); //Ask the config for the list of host:port pairs to open
    std::set<std::pair<std::string, int> > bound;   // note the space: > >, not >>
                                                    // bound is a set that remembers 
                                                    // which host:port pairs you've already opened, 
                                                    // so we can skip duplicates.

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
    if (revents & POLLOUT) {
        // re-find: onReadable above may have closed this fd
        std::map<int, Connection>::iterator cit = _conns.find(fd);
        if (cit != _conns.end())
            onWritable(cit->second);
    }
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
    processInput(conn);
    std::cout << "[fd " << conn.fd << "] received " << n
              << " bytes (total buffered: " << conn.inbuf.size() << ")" << std::endl;
}

// Feed the parser and act on its verdict. Split out from onReadable so the
// keep-alive path can re-run it on already-buffered (pipelined) bytes with
// no new recv.
    void Server::processInput(Connection& conn) {
        if (conn.unconsumedBytes() > MAX_REQUEST_BYTES) {
            conn.keepAlive = false;
            conn.queueResponse(buildError(431, "Request Header Fields Too Large"));
            watchFor(conn.fd, POLLOUT);
            return;
    }
 
    RequestParser::Status st = conn.parser.parse(conn.inbuf, conn.parsePos);
 
    if (st == RequestParser::INCOMPLETE)
        return;                                   // need more bytes; stay on POLLIN
 
    if (st == RequestParser::PARSE_ERROR) {
        conn.keepAlive = false;
        conn.queueResponse(buildError(400, "Bad Request"));
        watchFor(conn.fd, POLLOUT);
        return;
    }
 
    // COMPLETE
    conn.parsePos = conn.parser.bytesConsumed();  // advance past this request
    conn.queueResponse(buildResponse());          // TODO: ResponseBuilder::build(req, cfg)
    watchFor(conn.fd, POLLOUT);                   // arm the write phase
}

// ---- write, then keep-alive or close ---------------------------------------
 
void Server::onWritable(Connection& conn) {
    size_t remaining = conn.outbuf.size() - conn.writeOffset;
    ssize_t n = send(conn.fd, conn.outbuf.data() + conn.writeOffset, remaining, 0);
    if (n < 0) {                                  // gone; no errno check
        closeConnection(conn.fd);
        return;
    }
    conn.writeOffset += static_cast<size_t>(n);
 
    if (!conn.responseFullySent())
        return;                                   // partial send; wait next POLLOUT
 
    if (conn.keepAlive) {
        conn.resetForNextRequest();               // struct scrubs its own leftovers
        watchFor(conn.fd, POLLIN);                // THE FLIP: writing -> reading
        if (conn.hasBufferedRequest())            // a pipelined request already here?
            processInput(conn);                   // parse it now, no waiting
    } else {
        closeConnection(conn.fd);
    }
}

// ---- response building (temporary; becomes the HTTP ResponseBuilder) -------
 
std::string Server::buildResponse() {
    std::string body = "Hello from webserv\n";
    std::ostringstream oss;
    oss << "HTTP/1.1 200 OK\r\n"
        << "Content-Type: text/plain\r\n"
        << "Content-Length: " << body.size() << "\r\n"
        << "\r\n"
        << body;
    return oss.str();
}
 
std::string Server::buildError(int code, const std::string& reason) {
    std::ostringstream body;
    body << "<html><body><h1>" << code << " " << reason << "</h1></body></html>\n";
    std::string b = body.str();
 
    std::ostringstream oss;
    oss << "HTTP/1.1 " << code << " " << reason << "\r\n"
        << "Content-Type: text/html\r\n"
        << "Content-Length: " << b.size() << "\r\n"
        << "Connection: close\r\n"           // errors close the connection
        << "\r\n"
        << b;
    return oss.str();
}

// ---- teardown + poll-set bookkeeping ---------------------------------------

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

