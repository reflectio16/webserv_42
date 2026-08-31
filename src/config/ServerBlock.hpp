/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerBlock.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fmoulin <fmoulin@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/27 14:50:26 by fmoulin           #+#    #+#             */
/*   Updated: 2026/08/31 17:15:21 by fmoulin          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVERBLOCK_HPP
#define SERVERBLOCK_HPP

#include "LocationBlock.hpp"
#include <vector>

struct Endpoint
{
	std::string	host;
	int			port;
	
	Endpoint() : host(""), port(0) {}
};

struct ServerBlock
{
	Endpoint					listenAddr;
	std::string					serverName;
	std::string					root;
	size_t						client_max_body_size;
	std::vector<LocationBlock>	locations;

	ServerBlock()
		:	serverName(""),
			root(""),
			client_max_body_size(0)
	{	
	}
};

#endif