/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: meelma <meelma@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/14 14:18:37 by meelma            #+#    #+#             */
/*   Updated: 2026/09/29 15:56:39 by meelma           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include "ListeningSocket.hpp"
#include "CgiProcess.hpp"
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
    if (it->second == CGI_STDIN) {
        if (revents & POLLOUT) {
            std::map<int, int>::iterator o = _cgiOwner.find(fd);
            if (o != _cgiOwner.end()) {
                std::map<int, Connection>::iterator c = _conns.find(o->second);
                if (c != _conns.end()) {
                    CgiProcess::onStdinWritable(c->second);
                    // flag check: body fully sent (or broke) -> close+unregister stdin
                    if (c->second.cgiDoneWritingStdin && c->second.cgiStdinFd != -1) {
                        removeCgiPipe(c->second.cgiStdinFd);   // close + unpoll + _cgiOwner.erase
                        c->second.cgiStdinFd = -1;
                    }
                }
            }
        }
        return;
    }
    if (it->second == CGI_STDOUT) {
        if (revents & (POLLIN | POLLHUP)) {
            std::map<int, int>::iterator o = _cgiOwner.find(fd);
            if (o != _cgiOwner.end()) {
                std::map<int, Connection>::iterator c = _conns.find(o->second);
                if (c != _conns.end()) {
                    CgiProcess::onStdoutReadable(c->second);
                    if (c->second.cgiOutputComplete)
                        finishCgi(c->second);   // reap, finalize, arm write path
                }
            }
        }
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

    Connection conn(clientFd);
    std::map<int, Endpoint>::iterator ep = _listenEndpoints.find(listenFd);
    if (ep != _listenEndpoints.end()) {
        conn.listenHost = ep->second.host;
        conn.listenPort = ep->second.port;
    }
    _conns.insert(std::make_pair(clientFd, conn));
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
 
   // 3. COMPLETE -- now we have a request; decide what to do with it
   
    const HttpRequest& req = conn.parser.request();   // read his parsed request
    
    std::cerr << "[COMPLETE] method=" << req.method
              << " path=[" << req.path << "]" << std::endl;   // TEMP

              
    conn.keepAlive = req.keepAlive;                    // honor Connection: close for real
    conn.parsePos  = conn.parser.bytesConsumed();

    // ===== TEMPORARY CGI TEST STUB -- delete when build() lands =====

    std::cerr << "[check] comparing path to /hello.py" << std::endl; // TEMP    
    if (req.path == "/hello.py") {

        std::cerr << "[CGI] entering CGI branch" << std::endl;        // TEMP
        
        Outcome o;
        o.kind           = Outcome::CGI;
        o.keepAlive      = req.keepAlive;
        o.cgiInterpreter = "/usr/bin/python3";
        o.cgiScriptPath  = "www/cgi-bin/hello.py";
        o.cgiEnv.push_back("REQUEST_METHOD=" + req.method);
        o.cgiEnv.push_back("QUERY_STRING=" + req.query);
        o.cgiBody        = req.body;

        if (!CgiProcess::start(conn, o)) {
            conn.keepAlive = false;
            conn.queueResponse(buildError(500, "Internal Server Error"));
            watchFor(conn.fd, POLLOUT);
            return;
        }
        addCgiPipe(conn.cgiStdoutFd, conn.fd, POLLIN, CGI_STDOUT);
        if (conn.cgiStdinFd != -1)
            addCgiPipe(conn.cgiStdinFd, conn.fd, POLLOUT, CGI_STDIN);
        return;
    }
    // ===== END TEMPORARY STUB =====

    // 4. normal (non-CGI) path
    
    conn.queueResponse(buildResponse(req));            // pass the request in
    watchFor(conn.fd, POLLOUT);

   

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
 
std::string Server::buildResponse(const HttpRequest& req) {
    std::ostringstream body;
    body << "You requested: " << req.method << " " << req.path << "\n";
    std::string b = body.str();

    std::ostringstream oss;
    oss << "HTTP/1.1 200 OK\r\n"
        << "Content-Type: text/plain\r\n"
        << "Content-Length: " << b.size() << "\r\n";
    if (!req.keepAlive)
        oss << "Connection: close\r\n";
    oss << "\r\n" << b;
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
    std::map<int, Connection>::iterator c = _conns.find(fd);
    if (c != _conns.end() && c->second.state == CGI_RUNNING)
        CgiProcess::cleanup(c->second);   // kill/reap child, close+unregister its pipes

    close(fd);
    removeFromPoll(fd);
    _roles.erase(fd);
    _conns.erase(fd);
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

void Server::addCgiPipe(int pipeFd, int ownerClientFd, short events, FdRole role) {
    addToPoll(pipeFd, events, role);   // reuse existing: pushes pollfd + sets _roles
    _cgiOwner[pipeFd] = ownerClientFd;
}

void Server::removeCgiPipe(int pipeFd) {
    close(pipeFd);
    removeFromPoll(pipeFd);   // reuse existing: erases from _pfds + _roles
    _cgiOwner.erase(pipeFd);
}

// TEMP -- until François's ResponseBuilder::finalizeCgi lands.
// CGI output is "headers\r\n\r\nbody"; a real finalize parses the CGI headers.
// This crude version just wraps whatever the script printed as the body.
static std::string tempFinalizeCgi(const std::string& cgiOut, bool keepAlive) {
    std::ostringstream oss;
    oss << "HTTP/1.1 200 OK\r\n"
        << "Content-Length: " << cgiOut.size() << "\r\n";
    if (!keepAlive) oss << "Connection: close\r\n";
    oss << "\r\n" << cgiOut;
    return oss.str();
}

void Server::finishCgi(Connection& conn) {
    // stdout pipe is done -> stop watching it
    if (conn.cgiStdoutFd != -1) {
        removeCgiPipe(conn.cgiStdoutFd);
        conn.cgiStdoutFd = -1;
    }
    // if stdin pipe is somehow still open (e.g. child died early), drop it too
    if (conn.cgiStdinFd != -1) {
        removeCgiPipe(conn.cgiStdinFd);
        conn.cgiStdinFd = -1;
    }
    CgiProcess::cleanup(conn);   // reap the child (waitpid) -- kills the zombie

    // CGI stdout -> real HTTP response.  (Until François's finalizeCgi exists,
    // use a temporary wrapper -- see note below.)

    std::string resp = tempFinalizeCgi(conn.cgiBuf, conn.keepAlive); // will remove!!
    //std::string resp = ResponseBuilder::finalizeCgi(conn.cgiBuf, conn.keepAlive);
    conn.queueResponse(resp);
    conn.state = WRITING_RESPONSE;
    watchFor(conn.fd, POLLOUT);
}



