/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Config.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fmoulin <fmoulin@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/24 16:59:16 by fmoulin           #+#    #+#             */
/*   Updated: 2026/08/26 15:56:15 by fmoulin          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Config.hpp"
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

void	Config::parseListen(ServerBlock &server, const std::vector<std::string> &tokens, size_t &i)
{
	if (i + 2 >= tokens.size())
		throw std::runtime_error("Incomplete listen directive");

	const std::string &value = tokens[i + 1];

	std::string::size_type colonPos = value.find(':');

	if (colonPos == std::string::npos)
		throw std::runtime_error("Invalid listen directive");

	std::string host = value.substr(0, colonPos);
	std::string portString = value.substr(colonPos + 1);
	
	if (host.empty())
		throw std::runtime_error("Listen host cannot be empty");

	if (portString.empty())
		throw std::runtime_error("Listen port cannot be empty");
	
	if (tokens[i + 2] != ";")
		throw std::runtime_error("Expected ';' after listen directive");
		
	std::istringstream	stream(portString); 
	int					port;
	char				extra; //(extra is usefull in the case there would be something after the port. for example 8080banana. In this case with stream >> extra, banana would go directly in extra)

	if (!(stream >> port) || (stream >> extra))
		throw std::runtime_error("Invalid port");
	
	if (port < 1 || port > 65635)
		throw std::runtime_error("Invalid port: out of range");
	
	server.listenAddr.host = host;
	server.listenAddr.port = port;

	i+=3;
}

void	Config::parseServerName(ServerBlock &server, const std::vector<std::string> &tokens, size_t &i)
{
	if (i + 2 >= tokens.size())
		throw std::runtime_error("Incomplete server name directive");
	
	if (tokens[i + 1] == ";")
		throw std::runtime_error("Server name directive cannot be empty");
		
	if (tokens[i + 2] != ";")
		throw std::runtime_error("Expected ';' after server name directive");
	
	server.serverName = tokens[i + 1];

	i += 3;
}

void	Config::parseRoot(ServerBlock &server, const std::vector<std::string> &tokens, size_t &i)
{
	if (i + 2 >= tokens.size())
		throw std::runtime_error("Incomplete root directive");

	if (tokens[i + 1] == ";")
		throw std::runtime_error("Root directive cannot be empty");

	if (tokens[i + 1][0] != '/')
		throw std::runtime_error("Root must be an absolute path");

	if (tokens[i + 2] != ";")
		throw std::runtime_error("Expected ';' after root directive");
	
	server.root = tokens[i + 1];

	i += 3;
}


void	Config::parse(const std::vector<std::string> &tokens)
{
	size_t i = 0;
	
	while (i < tokens.size())
	{
		if (tokens[i] != "server")
			throw std::runtime_error("Expected: 'server'");
		++i;
		
		if (i >= tokens.size() || tokens[i] != "{")
			throw std::runtime_error("Expected: '{'");
		++i;
		
		ServerBlock	server;
		bool		hasListen = false;
		bool		hasServerName = false;
		bool		hasRoot = false;
		
		while (i < tokens.size() && tokens[i] != "}")
		{			
			if (tokens[i] == "listen")
			{
				if (hasListen)
					throw std::runtime_error("Duplicate listen directive");
				parseListen(server, tokens, i);
				hasListen = true;
			}
			else if (tokens[i] == "server_name")
			{
				if (hasServerName)
					throw std::runtime_error("Duplicate server name directive");
				parseServerName(server, tokens, i);
				hasServerName = true;
			}
			else if (tokens[i] == "root")
			{
				if (hasRoot)
					throw std::runtime_error("Duplicate root directive");
				parseRoot(server, tokens, i);
				hasRoot = true;
			}
			else
				throw std::runtime_error("Unknown directive: " + tokens[i]);
		}
		
		if (i >= tokens.size())
			throw std::runtime_error("Unclosed server block");

		if (!hasListen)
			throw std::runtime_error("Missing listen directive");
		
		if (!hasRoot)
			throw std::runtime_error("Missing root directive");
		
		++i;
		
		_servers.push_back(server);
	}
}

std::vector<Endpoint>	Config::getEndpoints() const
{
	std::vector<Endpoint>						result;
	std::vector<ServerBlock>::const_iterator	it;
	
	for (it = _servers.begin(); it != _servers.end(); ++it)
		result.push_back(it->listenAddr);

	return (result);
}
