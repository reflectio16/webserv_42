/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigValidation.cpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fmoulin <fmoulin@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/08 15:27:37 by fmoulin           #+#    #+#             */
/*   Updated: 2026/09/08 15:31:55 by fmoulin          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Config.hpp"
#include "ServerBlock.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cctype>
#include <iostream>

void	Config::validateLocation(const LocationBlock &location) const
{
	if (location.path.empty() || location.path[0] != '/')
		throw std::runtime_error("Invalide location path");
	
	if (location.methods.empty())
		throw std::runtime_error("Location must define ate least one HTTP method");

	if (location.redirectCode != 0 && location.redirectTarget.empty())
		throw std::runtime_error("Redirect requires a target");
}

void	Config::validateServer(const ServerBlock &server) const
{
	if (server.listenAddr.port <= 0 || server.listenAddr.port > 65535)
		throw std::runtime_error("Invalide server port");
	
	if (server.listenAddr.host.empty())
		throw std::runtime_error("Server host cannot be empty");

	if (server.root.empty())
		throw std::runtime_error("Server root cannot be empty");

	for (std::vector<LocationBlock>::const_iterator it = server.locations.begin(); it != server.locations.end(); ++it)
	{
		validateLocation(*it);
	}
}

void	Config::validate() const
{
	if (_servers.empty())
		throw std::runtime_error("Configuration must contains at least one server");

	for (std::vector<ServerBlock>::const_iterator it = _servers.begin(); it != _servers.end(); ++it)
	{
		validateServer(*it);
	}
}
