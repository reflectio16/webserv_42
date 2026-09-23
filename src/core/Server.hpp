/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: meelma <meelma@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/14 14:18:56 by meelma            #+#    #+#             */
/*   Updated: 2026/09/23 14:12:21 by meelma           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <map>
#include <poll.h>            // struct pollfd, poll, POLLIN, ...
#include "Config.hpp"
#include "Connection.hpp"

// The engine. Owns the config, the listening sockets, the live connections,
// and the single poll() loop that drives everything. One instance = one server.
class Server {
public:
    explicit Server(const std::string& configPath);  // may throw on bad config/bind
    ~Server();

    void run();   // the event loop; runs until the process is killed

private:
    // What kind of fd is this? (client vs a listening socket.) CGI roles later.
    enum FdRole { LISTENING, CLIENT };

    Config                      _config;   // kept for later (routing)
    std::vector<struct pollfd>  _pfds;     // the poll set -- each .events IS the mask
    std::map<int, FdRole>       _roles;    // fd -> role
    std::map<int, Connection>   _conns;    // client fd -> its Connection (by value:
                                           //   map nodes are stable, no manual delete)

    // ---- startup ----
    void setupListeners();

    // ---- loop steps ----
    void dispatch(int fd, short revents);
    void acceptClient(int listenFd);
    void onReadable(Connection& conn);      //recv, then hand to processInput
    void processInput(Connection& conn);   // feed the parser, act on the verdict
    void onWritable(Connection& conn);     // drain outbuf, then keep-alive or close
    void closeConnection(int fd);


    // response building -- TEMPORARY placeholders for the HTTP-side ResponseBuilder
    std::string buildResponse(const HttpRequest& req);
    std::string buildError(int code, const std::string& reason);


    // ---- poll-set bookkeeping (the mask lives here, not in Connection) ----
    void addToPoll(int fd, short events, FdRole role);
    void removeFromPoll(int fd);
    void watchFor(int fd, short events);   // used once the write path exists

    // non-copyable: server owns fds, must not be duplicated
    // we declare them private and never define them
    Server(const Server&);
    Server& operator=(const Server&);
};

#endif

