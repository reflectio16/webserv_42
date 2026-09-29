/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResponseBuilder.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fmoulin <fmoulin@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/28 12:33:26 by fmoulin           #+#    #+#             */
/*   Updated: 2026/09/29 17:40:00 by fmoulin          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ResponseBuilder.hpp"
#include <iostream>

ResponseBuilder::ResponseBuilder(const Config &config) : _config(config)
{
	
}

std::string		ResponseBuilder::hostWithoutPort(const std::string &host) const
{
	std::string::size_type	colon = host.find(":");

	if (colon == std::string::npos)
		return (host);
	
	return (host.substr(0, colon));
}

std::string		ResponseBuilder::getEffectiveRoot(const ServerBlock &server, const LocationBlock *location) const
{
	if (location != NULL && !location->root.empty())
		return (location->root);
	
	return (server.root);
}

std::string	ResponseBuilder::joinPaths(const std::string &root, const std::string &suffix) const
{
	if (root.empty())
		return ("");

	if (suffix.empty())
		return (root);

	if (root[root.size() - 1] == '/' && suffix[0] == '/')
		return (root + suffix[1]);
	
	if (root[root.size() - 1] != '/' && suffix[0] != '/')
		return (root + "/" + suffix);

	return (root + suffix);
}

std::string		ResponseBuilder::buildFilePath(const std::string &normalizedPath, const ServerBlock &server, const LocationBlock *location) const
{
	if (location != NULL && !location->root.empty())
	{
		std::string suffix = normalizedPath.substr(location->path.size());
		
		return (joinPaths(location->root, suffix));
	}

	return (joinPaths(server.root, normalizedPath));
}

bool	ResponseBuilder::normalizePath(const std::string &path, std::string &normalized) const
{
	if (path.empty() || path[0] != '/')
		return (false);

	std::vector<std::string>	parts;
	std::string::size_type		start = 0;

	while (start < path.size())
	{
		while (start < path.size() && path[start] == '/')
			++start;
			
		if (start == path.size())
			break;

		std::string::size_type	end = path.find('/', start);
		
		if (end == std::string::npos)
			end = path.size();

		std::string	part = path.substr(start, end - start);

		if (part == "." || part.empty())
		{
			
		}
		else if (part == "..")
		{
			if (parts.empty())
				return (false);
			
			parts.pop_back();
		}
		else
		{
			parts.push_back(part);
		}
		
		start = end;	
	}
	
	normalized = "/";

	for (std::vector<std::string>::size_type i = 0; i < parts.size(); ++i)
	{
		if (i != 0)
			normalized += "/";
		
		normalized += parts[i];
	}
	
	return (true);
}

ResponseBuilder::ResourceType	ResponseBuilder::getResourceType(const std::string &path) const
{
	struct stat info;

	if (stat(path.c_str(), &info) != 0)
	{
		if (errno == ENOENT || errno == ENOTDIR)
			return (RESOURCE_NOT_FOUND);
			
		return (RESOURCE_ERROR);
	}
	
	if (S_ISREG(info.st_mode))
		return (RESOURCE_FILE);

	if (S_ISDIR(info.st_mode))
		return (RESOURCE_DIRECTORY);

	return (RESOURCE_OTHER);
}


// Pour tests //

void	ResponseBuilder::debugRouting(
	const HttpRequest &request,
	const Endpoint &endpoint) const
{
	std::map<std::string, std::string>::const_iterator hostIt;

	hostIt = request.headers.find("host");

	if (hostIt == request.headers.end())
	{
		std::cout << "No Host header" << std::endl;
		return ;
	}

	std::string host = hostWithoutPort(hostIt->second);

	const ServerBlock *server =
		_config.findServer(endpoint, host);

	if (server == NULL)
	{
		std::cout << "No server found" << std::endl;
		return ;
	}

	std::string	normalizedPath;

	if (!normalizePath(request.path, normalizedPath))
	{
		std::cout << "INVALID PATH" << std::endl;
		return ;
	}
	
	const LocationBlock *location =
		_config.findLocation(*server, normalizedPath);

	std::string root =
		getEffectiveRoot(*server, location);

	std::string path =
		buildFilePath(normalizedPath, *server, location);

	std::cout << "Host     : " << host << std::endl;
	std::cout << "URI      : " << request.path << std::endl;
	std::cout << "server   : " << server->serverName << std::endl;

	if (location != NULL)
		std::cout << "location : " << location->path << std::endl;
	else
		std::cout << "location : NONE" << std::endl;

	std::cout << "root     : " << root << std::endl;
	
	std::cout << "path     : " << path << std::endl;

	if (path.empty())
	{
		std::cout << "resource : INVALID PATH" << std::endl;
		return ;
	}

	ResourceType type = getResourceType(path);

	if (type == RESOURCE_FILE)
		std::cout << "resource : FILE" << std::endl;
	else if (type == RESOURCE_DIRECTORY)
		std::cout << "resource : DIRECTORY" << std::endl;
	else if (type == RESOURCE_NOT_FOUND)
		std::cout << "resource : NOT FOUND" << std::endl;
	else if (type == RESOURCE_OTHER)
		std::cout << "resource : OTHER" << std::endl;
	else
		std::cout << "resource : ERROR" << std::endl;
}