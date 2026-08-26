/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ListeningSocket.hpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: meelma <meelma@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/26 17:28:45 by meelma            #+#    #+#             */
/*   Updated: 2026/08/26 17:29:11 by meelma           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef LISTENINGSOCKET_HPP
#define LISTENINGSOCKET_HPP

#include <string>

// Create a non-blocking TCP listening socket bound to host:port.
//
// Returns the listening fd on success, or -1 on failure (a message naming
// the failed step is printed to stderr). The CALLER owns the returned fd:
// register it in the poll set with role LISTENING, and close() it on shutdown.
int makeListeningSocket(const std::string& host, int port);

#endif
