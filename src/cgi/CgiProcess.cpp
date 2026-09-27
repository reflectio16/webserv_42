/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiProcess.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: meelma <meelma@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/27 21:56:03 by meelma            #+#    #+#             */
/*   Updated: 2026/09/27 21:56:14 by meelma           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CgiProcess.hpp"

// ============================================================================
// LAYER 2 fills these in. For now they are empty stubs so the loop-side
// bookkeeping (layer 3) compiles and links. None of these do anything yet.
// ============================================================================

namespace CgiProcess {

bool start(Connection& conn, const Outcome& recipe) {
    (void)conn;
    (void)recipe;
    return false;   // "spawn failed" until start() is implemented
}

void onStdinWritable(Connection& conn) {
    (void)conn;
}

void onStdoutReadable(Connection& conn) {
    (void)conn;
}

void cleanup(Connection& conn) {
    (void)conn;
}

}