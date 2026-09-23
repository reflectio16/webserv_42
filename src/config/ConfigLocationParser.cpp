/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigLocationParser.cpp                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fmoulin <fmoulin@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/08 15:27:15 by fmoulin           #+#    #+#             */
/*   Updated: 2026/09/08 15:31:06 by fmoulin          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Config.hpp"
#include "ServerBlock.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cctype>
#include <iostream>

void	Config::parseMethods(LocationBlock &location, const std::vector<std::string> &tokens, size_t &i)
{
	++i;
	
	if (i >= tokens.size() || tokens[i] == ";")
		throw std::runtime_error("Method directive cannot be empty");

	while (i < tokens.size() && tokens[i] != ";")
	{
		if (tokens[i] != "GET" && tokens[i] != "POST" && tokens[i] != "DELETE")
			throw std::runtime_error("Invalid HTTP method: " + tokens[i]);
			
		for (std::vector<std::string>::const_iterator it = location.methods.begin(); it != location.methods.end(); ++it)
		{
			if (*it == tokens[i])
				throw std::runtime_error("Duplicate HTTP method: " + tokens[i]);
		}
		
		location.methods.push_back(tokens[i]);
		
		++i;
	}
	
	if (i >= tokens.size())
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
	
	for (std::vector<LocationBlock>::const_iterator it = server.locations.begin(); it != server.locations.end(); ++it)
	{
		if (it->path == location.path)
			throw std::runtime_error("Duplicate location path: " + location.path);
	}
	
	i += 3;

	bool	hasMethods = false;
	bool	hasLocationRoot = false;
	bool	hasIndex = false;
	bool	hasAutoIndex = false;
	bool	hasUploadDir = false;
	bool	hasRedirect = false;
	
	while (i < tokens.size() && tokens[i] != "}")
	{
		if (tokens[i] == "methods")
		{
			if (hasMethods)
				throw std::runtime_error("Duplicate method directive");
			
			parseMethods(location, tokens, i);
			hasMethods = true;
		}
		else if (tokens[i] == "root")
		{
			if (hasLocationRoot)
				throw std::runtime_error("Duplicate root directive in location");
				
			parseLocationRoot(location, tokens, i);
			hasLocationRoot = true;
		}
		else if (tokens[i] == "index")
		{
			if (hasIndex)
				throw std::runtime_error("Duplicate index directive");
				
			parseIndex(location, tokens, i);
			hasIndex = true;
		}
		else if (tokens[i] == "autoindex")
		{
			if (hasAutoIndex)
				throw std::runtime_error("Duplicate autoindex directive");
				
			parseAutoIndex(location, tokens, i);
			hasAutoIndex = true;
		}
		else if (tokens[i] == "upload_dir")
		{
			if (hasUploadDir)
				throw std::runtime_error("Duplicate upload_dir directive");
			
			parseUploadDir(location, tokens, i);
			hasUploadDir = true;
		}
		else if (tokens[i] == "redirect")
		{
			if (hasRedirect)
				throw std::runtime_error("Duplicate redirect directive");
			
			parseRedirect(location, tokens, i);
			hasRedirect = true;
		}
		else if (tokens[i] == "cgi_handler")
			parseCgiHandler(location, tokens, i);
		else
			throw std::runtime_error("Unknown location directive: " + tokens[i]);
	}
	
	if (i >= tokens.size())
		throw std::runtime_error("Unclosed location block");
	
	if (!hasMethods)
		throw std::runtime_error("Location block requires a methods directive");
		
	++i;
	
	server.locations.push_back(location);
}