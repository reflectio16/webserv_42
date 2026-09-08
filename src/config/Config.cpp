/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Config.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fmoulin <fmoulin@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/24 16:59:16 by fmoulin           #+#    #+#             */
/*   Updated: 2026/09/08 15:31:49 by fmoulin          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Config.hpp"
#include "ServerBlock.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cctype>
#include <iostream>

Config::Config()
{
	
}

Config::Config(const std::string &filename)
{
	std::string	content = readFile(filename);
	std::vector<std::string> tokens = tokenize(content);

	parse(tokens);
	validate();
}

std::string	Config::readFile(const std::string &filename) const
{
	std::ifstream	file(filename.c_str());
	
	if (!file.is_open())
		throw std::runtime_error("Could not open configuration file");

	std::stringstream	buffer;
	buffer << file.rdbuf();
	
	return (buffer.str());
}

std::vector<std::string>	Config::tokenize(const std::string &content) const
{
	std::vector<std::string>	tokens;
	std::string					current;

	for (size_t i = 0; i < content.size(); ++i)
	{
		char c = content[i];
		
		if (std::isspace(static_cast<unsigned char>(c)))
		{
			if (!current.empty())
			{
				tokens.push_back(current);
				current.clear();
			}
		}
		else if (c == '{' || c == '}' || c == ';')
		{
			if (!current.empty())
			{
				tokens.push_back(current);
				current.clear();
			}
			tokens.push_back(std::string(1, c));
		}
		else
		{
			current.push_back(c);
		}
	}
	
	if (!current.empty())
	{
		tokens.push_back(current);
		current.clear();
	}
	
	return (tokens);
}

std::vector<Endpoint>	Config::getEndpoints() const
{
	std::vector<Endpoint>	endpoints;
	
	for (std::vector<ServerBlock>::const_iterator it = _servers.begin(); it != _servers.end(); ++it)
	{
		bool	alreadyExists = false;
		
		for (std::vector<Endpoint>::const_iterator it2 = endpoints.begin(); it2 != endpoints.end(); ++it2)
		{
			if (it2->host == it->listenAddr.host && it2->port == it->listenAddr.port)
			{
				alreadyExists = true;
				break;
			}
		}
		
		if (!alreadyExists)
			endpoints.push_back(it->listenAddr);
	}

	return (endpoints);
}

const std::vector<ServerBlock>	Config::getServers() const
{
	return (_servers);
}

const ServerBlock*	Config::findServer(const Endpoint &endpoint, const std::string &hostHeader) const
{
	const ServerBlock*	defaultServer = NULL;
	
	std::string			normalizedHost = hostHeader;
	std::string::size_type	colon = normalizedHost.find(':');

	if (colon != std::string::npos)
		normalizedHost = normalizedHost.substr(0, colon);
	
	for (std::vector<ServerBlock>::const_iterator it = _servers.begin(); it != _servers.end(); ++it)
	{
		if (endpoint.host == it->listenAddr.host
			&& endpoint.port == it->listenAddr.port)
		{
			if (defaultServer == NULL)
				defaultServer = &(*it);
			
			if (!normalizedHost.empty() && normalizedHost == it->serverName)
				return (&(*it));
		}
	}
	return (defaultServer);
}

const LocationBlock*	Config::findLocation(const ServerBlock &server, const std::string &uriPath) const
{
	const	LocationBlock* 	bestMatch = NULL;
	std::string::size_type	bestLength = 0;
	
	for (std::vector<LocationBlock>::const_iterator it = server.locations.begin(); it != server.locations.end(); ++it)
	{
		if (uriPath.compare(0, it->path.size(), it->path) == 0 && it->path.size() > bestLength)
		{
			bestMatch = &(*it);
			bestLength = it->path.size();
		}
	}
	
	return (bestMatch);
}
		