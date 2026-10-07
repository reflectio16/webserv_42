/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResponseBuilder.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fmoulin <fmoulin@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/28 12:33:26 by fmoulin           #+#    #+#             */
/*   Updated: 2026/10/07 15:54:20 by fmoulin          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ResponseBuilder.hpp"
#include <iostream>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

ResponseBuilder::ResponseBuilder(const Config &config) : _config(config)
{
	
}

std::string	ResponseBuilder::toLower(const std::string &str)
{
	std::string result = str;

	for (std::string::size_type i = 0; i != result.size(); ++i)
	{
		result[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(result[i])));
	}
	return (result);
}

// Pour tests //

// void	ResponseBuilder::debugRouting(
// 	const HttpRequest &request,
// 	const Endpoint &endpoint) const
// {
// 	std::map<std::string, std::string>::const_iterator hostIt;

// 	hostIt = request.headers.find("host");

// 	if (hostIt == request.headers.end())
// 	{
// 		std::cout << "No Host header" << std::endl;
// 		return ;
// 	}

// 	std::string host = hostWithoutPort(hostIt->second);

// 	const ServerBlock *server =
// 		_config.findServer(endpoint, host);

// 	if (server == NULL)
// 	{
// 		std::cout << "No server found" << std::endl;
// 		return ;
// 	}

// 	std::string	normalizedPath;

// 	if (!normalizePath(request.path, normalizedPath))
// 	{
// 		std::cout << "INVALID PATH" << std::endl;
// 		return ;
// 	}
	
// 	const LocationBlock *location =
// 		_config.findLocation(*server, normalizedPath);

// 	if (request.path == "/cgi/hello.py")
// 	{
// 		std::string scriptPath =
// 			"/tmp/webserv/cgi/hello.py";

// 		std::map<std::string, std::string> env =
// 			buildCgiEnvironment(
// 				request,
// 				*server,
// 				endpoint,
// 				scriptPath);

// 		for (std::map<std::string, std::string>::const_iterator it =
// 				env.begin();
// 			it != env.end();
// 			++it)
// 		{
// 			std::cout << it->first
// 					<< "="
// 					<< it->second
// 					<< std::endl;
// 		}

// 		return ;
// 	}
		
// 	if (!isSupportedMethod(request.method))
// 	{
// 		std::string response =
// 			buildErrorResponse(501, *server, request);

// 		std::cout << "\n--- HTTP RESPONSE ---\n";
// 		std::cout << response;
// 		std::cout << "\n--- END RESPONSE ---\n";

// 		return ;
// 	}

// 	if (!isMethodAllowed(request.method, location))
// 	{
// 		std::string response =
// 			buildErrorResponse(
// 				405,
// 				*server,
// 				request,
// 				buildAllowHeader(location));

// 		std::cout << "\n--- HTTP RESPONSE ---\n";
// 		std::cout << response;
// 		std::cout << "\n--- END RESPONSE ---\n";

// 		return ;
// 	}
		
// 	if (location != NULL && location->redirectCode != 0)
// 	{
// 		std::string	response = buildRedirectResponse(*location, request);

// 		if (response.empty())
// 			response = buildErrorResponse(500, *server, request);

// 		std::cout << "\n--- HTTP RESPONSE ---\n" << std::endl;
// 		std::cout << response;
// 		std::cout << "\n--- END RESPONSE ---\n" << std::endl;

// 		return ;
// 	}

// 	std::string root =
// 		getEffectiveRoot(*server, location);

// 	std::string path =
// 		buildFilePath(normalizedPath, *server, location);

// 	if (path.empty())
// 	{
// 		std::cout << buildErrorResponse(
// 			500, *server, request);
// 		return ;
// 	}

// 	if (request.method == "DELETE")
// 	{
// 		std::string response =
// 			buildDeleteResponse(
// 				path,
// 				*server,
// 				request);

// 		std::cout << "\n--- HTTP RESPONSE ---\n";
// 		std::cout << response;
// 		std::cout << "\n--- END RESPONSE ---\n";

// 		return ;
// 	}

// 	if (request.method == "POST")
// 	{
// 		if (location == NULL)
// 		{
// 			std::cout << buildErrorResponse(
// 				404, *server, request);

// 			return ;
// 		}

// 		if (server->clientMaxBodySize != 0
// 			&& request.body.size() > server->clientMaxBodySize)
// 		{
// 			std::string	response = buildErrorResponse(
// 				413, *server, request);

			
// 			std::cout << "\n--- HTTP RESPONSE ---\n";
// 			std::cout << response;
// 			std::cout << "\n--- END RESPONSE ---\n";
			
// 			return ;
// 		}

// 		std::string response ;
		
// 		if (isMultipartRequest(request))
// 		{
// 			response = buildMultipartUploadResponse(*location, *server, request);
// 		}
// 		else
// 		{
// 			response = buildUploadResponse(normalizedPath, *location, *server, request);
// 		}

// 		std::cout << "\n--- HTTP RESPONSE ---\n";
// 		std::cout << response;
// 		std::cout << "\n--- END RESPONSE ---\n";

// 		return ;
// 	}
	
// 	if (request.method != "GET")
// 	{
// 		std::cout << "Method allowed but not implemented yet: "
// 				<< request.method
// 				<< std::endl;
// 		return ;
// 	}

// 	std::cout << "Host     : " << host << std::endl;
// 	std::cout << "URI      : " << request.path << std::endl;
// 	std::cout << "server   : " << server->serverName << std::endl;

// 	if (location != NULL)
// 		std::cout << "location : " << location->path << std::endl;
// 	else
// 		std::cout << "location : NONE" << std::endl;

// 	std::cout << "root     : " << root << std::endl;
	
// 	std::cout << "path     : " << path << std::endl;

// 	if (path.empty())
// 	{
// 		std::cout << "resource : INVALID PATH" << std::endl;
// 		return ;
// 	}

// 	ResourceType type = getResourceType(path);

// 	if (type == RESOURCE_FILE && request.method == "GET")
// 	{
// 		std::string	response = buildStaticFileResponse(path, request);

// 		std::cout << "\n--- HTTP RESPONSE ---\n" << std::endl;
// 		std::cout << response;
// 		std::cout << "\n--- END RESPONSE ---\n" << std::endl;
// 	}
// 	else if (type == RESOURCE_DIRECTORY)
// 	{
// 		std::string	indexPath = findIndexFile(path, location);

// 		if (!indexPath.empty())
// 		{
// 			std::cout	<< "index    : "
// 						<< indexPath
// 						<<std::endl;
						
// 			std::string	response = buildStaticFileResponse(indexPath, request);

// 			std::cout << "\n--- HTTP RESPONSE ---\n" << std::endl;
// 			std::cout << response;
// 			std::cout << "\n--- END RESPONSE ---\n" << std::endl;
// 			return ;
// 		}
// 		if (location != NULL && location->autoindex)
// 		{
// 			std::string response = buildAutoindexResponse(path, normalizedPath, request);

// 			std::cout << "\n--- HTTP RESPONSE ---\n" << std::endl;
// 			std::cout << response;
// 			std::cout << "\n--- END RESPONSE ---\n" << std::endl;
// 			return ;
// 		}
		
// 		std::string	response = buildErrorResponse(403, *server, request);
		
// 		std::cout << response << std::endl;
// 		return ;
// 	}
// 	else if (type == RESOURCE_NOT_FOUND)
// 	{
// 		std::string	response = buildErrorResponse(404, *server, request);
		
// 		std::cout << response << std::endl;
// 		return ;
// 	}
// 	else if (type == RESOURCE_OTHER)
// 		std::cout << "resource : OTHER" << std::endl;
// 	else if (type == RESOURCE_ERROR)
// 	{
// 		std::string	response = buildErrorResponse(500, *server, request);
		
// 		std::cout << response << std::endl;
// 		return ;
// 	}
// }