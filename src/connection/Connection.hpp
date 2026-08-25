/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: meelma <meelma@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/25 17:13:30 by meelma            #+#    #+#             */
/*   Updated: 2026/08/25 17:26:10 by meelma           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <string>
#include <cstddef>
#include <sys/types.h>          // pid_t
//#include "RequestParser.hpp"    // parser is a by-value member -> needs full type

// Per-client state object, owned by the I/O layer. Created on accept(), lives
// until the socket closes. Holds everything that must survive between poll
// wakeups -- this struct IS the connection's saved place on the heap.

enum ConnState {
    READING_REQUEST,   // parked on POLLIN
    WRITING_RESPONSE,  // parked on POLLOUT
    CGI_RUNNING,       // parked on the CGI pipe fds
    CLOSING            // transient teardown
};

struct Connection {
    // ---- identity ----
    int           fd;
    ConnState     state;

    // ---- read side: append-only tape + cursor ----
    std::string   inbuf;          // recv() appends here; never erased mid-request
    std::size_t   parsePos;       // how far the parser has consumed inbuf
    //RequestParser parser;         // reads inbuf from parsePos; never mutates it

    // ---- write side ----
    std::string   outbuf;         // full response queued to send
    std::size_t   writeOffset;    // how much of outbuf already sent

    // ---- control ----
    bool          keepAlive;      // reuse after this response, or close?
    long          lastActivityMs; // for the idle-timeout sweep

    // ---- CGI (valid only while state == CGI_RUNNING) ----
    pid_t         cgiPid;
    int           cgiStdinFd;
    int           cgiStdoutFd;
    std::string   cgiStdin;
    std::size_t   cgiStdinOffset;
    std::string   cgiBuf;
    long          cgiStartMs;

    explicit Connection(int clientFd);

    // ---- read side ----
    std::size_t unconsumedBytes() const;    // inbuf bytes not yet parsed
    bool        hasBufferedRequest() const; // a pipelined request already waiting?
    void        compact();                  // drop consumed prefix, rebase cursor

    // ---- write side ----
    void        queueResponse(const std::string& bytes); // arm the write phase
    bool        responseFullySent() const;

    // ---- keep-alive transition (caller must also flip poll mask to POLLIN) ----
    void        resetForNextRequest();
};

#endif // CONNECTION_HPP
