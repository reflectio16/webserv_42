/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResponseBuilder.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fmoulin <fmoulin@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/28 12:33:26 by fmoulin           #+#    #+#             */
/*   Updated: 2026/09/30 17:04:29 by fmoulin          ###   ########.fr       */
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

bool	ResponseBuilder::readFile(const std::string &path, std::string &content) const
{
	std::ifstream file(path.c_str(), std::ios::in | std::ios::binary);

	if (!file.is_open())
		return (false);

	std::ostringstream stream;

	stream << file.rdbuf();

	if (file.bad())
		return (false);

	content = stream.str();

	return (true);
}

std::string	ResponseBuilder::getMimeType(
	const std::string &path) const
{
	std::string::size_type dot = path.rfind('.');

	if (dot == std::string::npos)
		return ("application/octet-stream");

	std::string extension = toLower(path.substr(dot + 1));

	if (extension == "html" || extension == "htm")
		return ("text/html");

	if (extension == "css")
		return ("text/css");

	if (extension == "js")
		return ("application/javascript");

	if (extension == "txt")
		return ("text/plain");

	if (extension == "jpg" || extension == "jpeg")
		return ("image/jpeg");

	if (extension == "png")
		return ("image/png");

	if (extension == "gif")
		return ("image/gif");

	if (extension == "svg")
		return ("image/svg+xml");

	if (extension == "ico")
		return ("image/x-icon");

	if (extension == "json")
		return ("application/json");

	if (extension == "pdf")
		return ("application/pdf");

	return ("application/octet-stream");
}

std::string	ResponseBuilder::sizeToString(std::size_t value) const
{
	std::ostringstream	stream;

	stream << value;

	return (stream.str());
}

std::string		ResponseBuilder::buildResponse(int statusCode, const std::string &reason, const std::string &contentType, const std::string &body, bool keepAlive) const
{
	std::ostringstream	response;

	response << "HTTP/1.1 "
			 << statusCode
			 << " "
			 << reason
			 << "\r\n";

	response << "Content-Type: "
			 << contentType
			 << "\r\n";

	response << "Content-Length: "
			 << body.size()
			 << "\r\n";
			 
	response << "Connection: "
			 << (keepAlive ? "keep-alive" : "close")
			 << "\r\n";
	
	response << "\r\n";
	
	response << body;

	return (response.str());
}

std::string	ResponseBuilder::buildStaticFileResponse(const std::string &path, const HttpRequest &request) const
{
	std::string	body;

	if (!readFile(path, body))
		return ("");
		
	return (buildResponse(200, "OK", getMimeType(path), body, request.keepAlive));
}

std::string	ResponseBuilder::buildAutoindexResponse(const std::string &directoryPath, const std::string &uriPath, const HttpRequest &request) const
{
	std::string	body;

	if (!buildAutoIndexBody(directoryPath, uriPath, body))
		return ("");

	return (buildResponse(200, "OK", "text/html", body, request.keepAlive));
}	

std::string	ResponseBuilder::findIndexFile(const std::string &directoryPath, const LocationBlock *location) const
{
	if (location == NULL || location->index.empty())
		return ("");

	std::string	indexPath = joinPaths(directoryPath, location->index);

	if (getResourceType(indexPath) != RESOURCE_FILE)
		return ("");

	return (indexPath);
}

bool	ResponseBuilder::buildAutoIndexBody(const std::string &directoryPath, const std::string &uriPath, std::string &body) const
{
	DIR	*dir = opendir(directoryPath.c_str());
	
	if (dir == NULL)
		return (false);

	std::ostringstream	html;
	
	html << "<html>\n";
	html << "<head><title>Index of "
		 << uriPath
		 << "</title></head>\n";
		 
	html << "<body>\n";
	html << "<h1>Index of "
		 << uriPath
		 << "</h1>\n";

	html << "<ul>\n";

	struct dirent *entry;

	while ((entry = readdir(dir)) != NULL)
	{
		std::string	name = entry->d_name;

		if (name == "." || name == "..")
			continue;

		html << "<li><a href=\"";

		if (uriPath.empty() || uriPath[uriPath.size() - 1] != '/')
			html << uriPath << "/";
		else
			html << uriPath;

		html << name
			 << "\">"
			 << name
			 << "</a></li>\n";
	}
	
	html << "</ul>\n";
	html << "</body>\n";
	html << "</html>\n";

	closedir(dir);

	body = html.str();

	return (true);
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

	if (type == RESOURCE_FILE && request.method == "GET")
	{
		std::string	response = buildStaticFileResponse(path, request);

		std::cout << "\n--- HTTP RESPONSE ---\n" << std::endl;
		std::cout << response;
		std::cout << "\n--- END RESPONSE ---\n" << std::endl;
	}
	else if (type == RESOURCE_DIRECTORY)
	{
		std::string	indexPath = findIndexFile(path, location);

		if (!indexPath.empty())
		{
			std::cout	<< "index    : "
						<< indexPath
						<<std::endl;
						
			std::string	response = buildStaticFileResponse(indexPath, request);

			std::cout << "\n--- HTTP RESPONSE ---\n" << std::endl;
			std::cout << response;
			std::cout << "\n--- END RESPONSE ---\n" << std::endl;
			return ;
		}
		if (location != NULL && location->autoindex)
		{
			std::string response = buildAutoindexResponse(path, normalizedPath, request);

			std::cout << "\n--- HTTP RESPONSE ---\n" << std::endl;
			std::cout << response;
			std::cout << "\n--- END RESPONSE ---\n" << std::endl;
			return ;
		}
		
		std::cout << "DIRECTORY FORBIDDEN" << std::endl;
		return ;
	}
	else if (type == RESOURCE_NOT_FOUND)
		std::cout << "resource : NOT FOUND" << std::endl;
	else if (type == RESOURCE_OTHER)
		std::cout << "resource : OTHER" << std::endl;
	else
		std::cout << "resource : ERROR" << std::endl;
}