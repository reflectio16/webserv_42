/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerBlock.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fmoulin <fmoulin@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/27 14:50:26 by fmoulin           #+#    #+#             */
/*   Updated: 2026/08/31 18:06:02 by fmoulin          ###   ########.fr       */
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
	size_t						clientMaxBodySize;
	std::map<int, std::string>	errorPage;
	std::vector<LocationBlock>	locations;

	ServerBlock()
		:	serverName(""),
			root(""),
			clientMaxBodySize(0)
	{	
	}
};

#endif