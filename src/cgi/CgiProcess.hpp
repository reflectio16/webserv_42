/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiProcess.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: meelma <meelma@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/27 21:55:24 by meelma            #+#    #+#             */
/*   Updated: 2026/09/27 21:59:33 by meelma           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGIPROCESS_HPP
#define CGIPROCESS_HPP

#include "Connection.hpp"
//#include "ResponseBuilder.hpp"   // for Outcome (the CGI recipe)
#include "Outcome.hpp"

// Stateless helpers that operate on a Connection's cgi* fields. All CGI state
// lives on the Connection (its lifetime is a subset of the connection's), so
// these functions take a Connection& and act on it -- the same shape as the
// loop's onReadable / onWritable.
namespace CgiProcess {

    // fork/exec the script from the recipe, wire its pipes onto
    // conn.cgiStdinFd / conn.cgiStdoutFd, set conn.cgiPid + conn.cgiStartMs,
    // and move conn.state to CGI_RUNNING. Returns false if the spawn failed
    // (the caller then sends a 500).
    bool start(Connection& conn, const Outcome& recipe);

    // pipe-fd events, dispatched from the loop like onReadable/onWritable:
    void onStdinWritable(Connection& conn);   // feed conn.cgiStdin to the child
    void onStdoutReadable(Connection& conn);  // drain child stdout into conn.cgiBuf

    // kill (if still alive), reap the child, close+forget its pipes. Called on
    // normal completion, on timeout, and from closeConnection if a CGI was live.
    void cleanup(Connection& conn);
}

#endif