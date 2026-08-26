/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fmoulin <fmoulin@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/25 17:06:44 by meelma            #+#    #+#             */
/*   Updated: 2026/08/26 15:59:08 by fmoulin          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/*int main(int argc, char** argv) {
    (void)argc; (void)argv;
    std::cout << "webserv: skeleton compiles\n";
    return 0;
}*/

// #include "Connection.hpp"
#include "./src/config/Config.hpp"
#include <cassert>
#include <iostream>

int main(int argc, char **argv) {
    // Connection c(-1);                                   // fake fd, no socket
    // c.inbuf    = "GET / HTTP/1.1\r\n\r\nGET /x HTTP/1.1\r\n\r\n";
    // c.parsePos = 18;                                    // pretend req #1 consumed
    // assert(c.hasBufferedRequest());                     // req #2 is buffered
    // assert(c.unconsumedBytes() == c.inbuf.size() - 18);
    // c.compact();                                        // drop consumed prefix
    // assert(c.parsePos == 0);
    // assert(c.inbuf.size() == c.unconsumedBytes());      // nothing lost
    // std::cout << "Connection OK\n";

	if (argc != 2)
	{
		std::cerr << "see usage" << std::endl;
		return (1);
	}
	try
	{
		Config config(argv[1]);
		
		std::vector<Endpoint>					endpoints = config.getEndpoints();
		std::vector<Endpoint>::const_iterator	it;

		for (it = endpoints.begin(); it != endpoints.end(); ++it)
		{
			std::cout 	<< "host: "
						<< it->host
						<< std::endl
						<< "port: "
						<< it->port
						<< std::endl;
		}
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
	}
	
    return 0;
}
