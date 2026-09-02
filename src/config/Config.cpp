/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Config.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fmoulin <fmoulin@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/24 16:59:16 by fmoulin           #+#    #+#             */
/*   Updated: 2026/09/02 15:10:13 by fmoulin          ###   ########.fr       */
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
	
	if (port < 1 || port > 65535)
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

void	Config::parseClientMaxBodySize(ServerBlock &server, const std::vector<std::string> &tokens, size_t &i)
{
	if (i + 2 >= tokens.size())
		throw std::runtime_error("Incomplete client_max_body_size directive");

	if (tokens[i + 1] == ";")
		throw std::runtime_error("client_max_body_size cannot be empty");
	
	if (tokens[i + 2] != ";")
		throw std::runtime_error("Expected ';' after client max body size directive");

	const std::string &value = tokens[i + 1];

	for (std::string::size_type j = 0; j < value.size(); ++j)
	{
		if (!isdigit(static_cast<unsigned char>(value[j])))
			throw std::runtime_error("Invalid client_max_body_size");
	}

	std::istringstream	stream(tokens[i + 1]);
	size_t				bodySize;

	if (!(stream >> bodySize))
		throw std::runtime_error("Invalid client_max_body_size");
		
	server.clientMaxBodySize = bodySize;

	i += 3;
}

void	Config::parseErrorPage(ServerBlock &server, const std::vector<std::string> &tokens, size_t &i)
{
	if (i + 3 >= tokens.size())
		throw std::runtime_error("Incomplete Error Page directive");
	
	if (tokens[i + 1] == ";" || tokens[i + 2] == ";")
		throw std::runtime_error("Error page directive cannot be empty");
	
	if (tokens[i + 3] != ";")
		throw std::runtime_error("Expected ';' after error page directive");
	
	const std::string &value = tokens[i + 1];
		
	for (std::string::size_type j = 0; j < value.size(); ++j)
	{
		if (!isdigit(static_cast<unsigned char>(value[j])))
			throw std::runtime_error("Invalid error page status code");
	}
		
	std::istringstream	stream(value);
	int					code;

	if (!(stream >> code))
		throw std::runtime_error("Invalid error page status code");
		
	if (code < 400 || code > 599)
		throw std::runtime_error("Error page status code must be between 400 and 599");

	if (tokens[i + 2][0] != '/')
		throw std::runtime_error("Error page path must start with '/'");

	if (server.errorPages.find(code) != server.errorPages.end())
		throw std::runtime_error("Duplicate error_page status code");
		
	server.errorPages[code] = tokens[i + 2];

	i += 4;
}

void	Config::parseMethod(LocationBlock &location, const std::vector<std::string> &tokens, size_t &i)
{
	++i;
	
	if (i >= tokens.size() || tokens[i] == ";")
		throw std::runtime_error("Method directive cannot be empty");

	while (i < tokens.size() && tokens[i] != ";")
	{
		if (tokens[i] != "GET" && tokens[i] != "POST" && tokens[i] != "DELETE")
			throw std::runtime_error("Invalid HTTP method: " + tokens[i]);
		location.method.push_back(tokens[i]);
		++i;
	}
	
	if (i > tokens.size())
		throw std::runtime_error("Expected ';' after method directive");

	++i;
}

void	Config::parseLocationRoot(LocationBlock &location, const std::vector<std::string> &tokens, size_t &i)
{
	if (i + 2 >= tokens.size())
		throw std::runtime_error("Incomplete root directive");

	if (tokens[i + 1] == ";")
		throw std::runtime_error("Root directive cannot be empty");

	if (tokens[i + 1][0] != '/')
		throw std::runtime_error("Root must be an absolute path");

	if (tokens[i + 2] != ";")
		throw std::runtime_error("Expected ';' after root directive");
	
	location.root = tokens[i + 1];

	i += 3;
}

void	Config::parseIndex(LocationBlock &location, const std::vector<std::string> &tokens, size_t &i)
{
	if (i + 2 >= tokens.size())
		throw std::runtime_error("Incomplete index directive");

	if (tokens[i + 1] == ";")
		throw std::runtime_error("Index directive cannot be empty");
		
	if (tokens[i + 2] != ";")
		throw std::runtime_error("Expected ';' after index directive");

	location.index = tokens[i + 1];

	i += 3;
}

void	Config::parseAutoIndex(LocationBlock &location, const std::vector<std::string> &tokens, size_t &i)
{
	if (i + 2 >= tokens.size())
		throw std::runtime_error("Incomplete autoindex directive");

	if (tokens[i + 1] == ";")
		throw std::runtime_error("Autoindex directive cannot be empty");
	
	if (tokens[i + 2] != ";")
		throw std::runtime_error("Expected ';' after autoindex directive");
	
	if (tokens[i + 1] == "on")
		location.autoindex = true;
	else if (tokens[i + 1] == "off")
		location.autoindex = false;
	else
		throw std::runtime_error("Autoindex must be 'on' or 'off'");

	i += 3;
}

void	Config::parseUploadDir(LocationBlock &location, const std::vector<std::string> &tokens, size_t &i)
{
	if (i + 2 >= tokens.size())
		throw std::runtime_error("Incomplete upload_dir directive");
		
	if (tokens[i + 1] == ";")
		throw std::runtime_error("upload_dir directive cannot be empty");

	if (tokens[i + 1][0] != '/')
		throw std::runtime_error("upload_dir directive path must start with '/'");

	if (tokens[i + 2] != ";")
		throw std::runtime_error("Expected ';' after upload_dir directive");

	location.uploadDir = tokens[i + 1];

	i += 3;
}

void	Config::parseRedirect(LocationBlock &location, const std::vector<std::string> &tokens, size_t &i)
{
	if (i + 3 >= tokens.size())
		throw std::runtime_error("Incomplete redirect directive");
	
	if (tokens[i + 1] == ";" || tokens[i + 2] == ";")
		throw std::runtime_error("redirect directive cannot be empty");
	
	if (tokens[i + 3] != ";")
		throw std::runtime_error("Expected ';' after redirect directive");

	for (std::string::size_type j = 0; j < tokens[i + 1].size(); ++j)
	{
		if (!isdigit(static_cast<unsigned char>(tokens[i + 1][j])))
			throw std::runtime_error("Invalid redirect status code");
	}
	
	std::istringstream	stream(tokens[i + 1]);
	int					code;
	
	if (!(stream >> code))
		throw std::runtime_error("Invalid redirect status code");

	if (code != 301 
		&& code != 302
		&& code != 303
		&& code != 307
		&& code != 308)
	{
		throw std::runtime_error("Invalid redirect status code");
	}
	
	location.redirectCode = code;
	location.redirectTarget = tokens[i + 2];

	i += 4;
}

void	Config::parseCgiHandler(LocationBlock &location, const std::vector<std::string> &tokens, size_t &i)
{
	if (i + 3 >= tokens.size())
		throw std::runtime_error("Incomplete cgi_handler directive");

	if (tokens[i + 1] == ";" || tokens[i + 2] == ";")
		throw std::runtime_error("cgi_handler directive cannot be empty");
		
	if (tokens[i + 3] != ";")
		throw std::runtime_error("Expected ';' after cgi_handler directive");

	const std::string &extension = tokens[i + 1];
	const std::string &interpreter = tokens[i + 2];

	if (extension.size() < 2 || extension[0] != '.')
		throw std::runtime_error("Invalid CGI extension");

	if (interpreter.empty() || interpreter[0] != '/')
		throw std::runtime_error("CGI interpreter must be an absolute path");
	
	if (location.cgiHandlers.find(extension) != location.cgiHandlers.end())
		throw std::runtime_error("Duplicate CGI extension");
		
	location.cgiHandlers[extension] = interpreter;

	i += 4;
}

void	Config::parseLocation(ServerBlock &server, const std::vector<std::string> &tokens, size_t &i)
{
	if (i + 2 >= tokens.size())
		throw std::runtime_error("Incomplete location path");

	if (tokens[i + 1].empty() || tokens[i + 1][0] != '/')
		throw std::runtime_error("Invalid location path");

	if (tokens[i + 2] != "{")
		throw std::runtime_error("Expected '{' after location path");
	
		
	LocationBlock	location;
	location.path = tokens[i + 1];
	
	i += 3;

	bool	hasUploadDir = false;
	
	while (i < tokens.size() && tokens[i] != "}")
	{
		if (tokens[i] == "methods")
			parseMethod(location, tokens, i);
		else if (tokens[i] == "root")
			parseLocationRoot(location, tokens, i);
		else if (tokens[i] == "index")
			parseIndex(location, tokens, i);
		else if (tokens[i] == "autoindex")
			parseAutoIndex(location, tokens, i);
		else if (tokens[i] == "upload_dir")
		{
			if (hasUploadDir)
				throw std::runtime_error("Duplicate upload_dir directive");
			parseUploadDir(location, tokens, i);
			hasUploadDir = true;
		}
		else if (tokens[i] == "redirect")
		{
			if (location.redirectCode != 0)
				throw std::runtime_error("Duplicate redirect directive");
			parseRedirect(location, tokens, i);
		}
		else if (tokens[i] == "cgi_handler")
			parseCgiHandler(location, tokens, i);
		else
			++i;
	}
	
	if (i >= tokens.size())
		throw std::runtime_error("Unclosed location block");
	
	++i;
	
	server.locations.push_back(location);

	for (std::map<std::string, std::string>::const_iterator it =
         location.cgiHandlers.begin();
     it != location.cgiHandlers.end();
     ++it)
	{
		std::cout << it->first
				<< " -> "
				<< it->second
				<< std::endl;
	}
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
		bool		hasClientMaxBodySize = false;
		
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
			else if (tokens[i] == "client_max_body_size")
			{
				if (hasClientMaxBodySize)
					throw std::runtime_error("Duplicate client_max_body_size directive");
				parseClientMaxBodySize(server, tokens, i);
				hasClientMaxBodySize = true;
			}
			else if (tokens[i] == "error_page")
			{
				parseErrorPage(server, tokens, i);
			}
			else if (tokens[i] == "location")
			{
				parseLocation(server, tokens, i);
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
