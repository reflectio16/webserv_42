/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResponseBuilder.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fmoulin <fmoulin@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/28 12:33:26 by fmoulin           #+#    #+#             */
/*   Updated: 2026/10/06 17:31:12 by fmoulin          ###   ########.fr       */
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

std::string	ResponseBuilder::getReasonPhrase(int statusCode) const
{
	switch(statusCode)
	{
		case 201:
			return ("Created");
		case 204:
			return ("No Content");
			
		case 301:
			return ("Moved Permanently");
		case 302:
			return ("Found");
		case 303:
			return ("See Other");
		case 307:
			return ("Temporary Redirect");
		case 308:
			return ("Permanent Redirect");
			
		case 400:
			return ("Bad Request");
		case 403:
			return ("Forbidden");
		case 404:
			return ("Not Found");
		case 405:
			return ("Method Not Allowed");
		case 413:
			return ("Payload Too Large");
			
		case 500:
			return ("Internal Server Error");
		case 501:
			return ("Not Implemented");
		default:
			return ("Error");
	}
}

std::string	ResponseBuilder::buildDefaultErrorBody(int statusCode) const
{
	std::ostringstream body;

	std::string	reason = getReasonPhrase(statusCode);

	body << "<html>\n";
	body << "<head><title>"
		 << statusCode
		 << " "
		 << reason
		 << "</title></head>\n";

	body << "<body>\n";
	body << "<h1>"
		 << statusCode
		 << " "
		 << reason
		 << "</h1>\n";

	body << "</body>\n";
	body << "</html>\n";

	return (body.str());
}

std::string	ResponseBuilder::getCustomErrorPagePath(const ServerBlock &server, int statusCode) const
{
	std::map<int, std::string>::const_iterator it = server.errorPages.find(statusCode);
	
	if (it == server.errorPages.end())
		return ("");

	return (joinPaths(server.root, it->second));
}

bool	ResponseBuilder::isSupportedMethod(const std::string &method) const
{
	return (method == "GET" || method == "POST" || method == "DELETE");
}

bool	ResponseBuilder::isMethodAllowed(const std::string &method, const LocationBlock *location) const
{
	if (location == NULL || location->methods.empty())
		return (true);
		
	std::vector<std::string>::const_iterator	it;
	
	for (it = location->methods.begin(); it < location->methods.end(); ++it)
	{
		if (*it == method)
			return (true);
	}
	
	return (false);
}

bool	ResponseBuilder::deleteFile(const std::string &path) const
{
	return (std::remove(path.c_str()) == 0);
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

bool	ResponseBuilder::writeFile(const std::string &path, const std::string &body) const
{
	int	fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);

	if (fd < 0)
		return (false);

	std::size_t	totalWritten = 0;
	
	while (totalWritten < body.size())
	{
		ssize_t	written = write(fd, body.data() + totalWritten, body.size() - totalWritten);

		if (written <= 0)
		{
			close(fd);
			return (false);
		}
		
		totalWritten += static_cast<std::size_t>(written);
	}
	
	if (close(fd) != 0)
		return (false);
		
	return (true);
}

bool	ResponseBuilder::isMultipartRequest(const HttpRequest &request) const
{
	std::map<std::string, std::string>::const_iterator	it = request.headers.find("content-type");
	
	if (it == request.headers.end())
		return (false);

	std::string	value = toLower(it->second);
	
	return (value.find("multipart/form-data") == 0);
}

bool	ResponseBuilder::getMultipartBoundary(const HttpRequest &request, std::string &boundary) const
{
	std::map<std::string, std::string>::const_iterator it = request.headers.find("content-type");
	
	if (it == request.headers.end())
		return (false);

	const std::string	&value = it->second;
	std::string	lower = toLower(value);
	
	std::string::size_type	pos = lower.find("boundary=");
	
	if (pos == std::string::npos)
		return (false);

	pos += 9;
	
	if (pos >= value.size())
		return (false);

	if (value[pos] == '"')
	{
		std::string::size_type	end = value.find('"', pos + 1);
		
		if (end == std::string::npos)
			return (false);

		boundary = value.substr(pos + 1, end - pos - 1);
	}
	else
	{
		std::string::size_type	end = value.find(";");

		if (end == std::string::npos)
			boundary = value.substr(pos);
		else
			boundary = value.substr(pos, end - pos);
	}

	if (boundary.empty())
		return (false);
		
	if (boundary.find_first_of("\r\n") != std::string::npos)
		return (false);

	return (true);
}

bool	ResponseBuilder::parseMultipartFile(const std::string &body, const std::string &boundary, MultipartFile &file) const
{
	const std::string	delimiter = "--" + boundary;
	
	std::string::size_type	pos = 0;

	while(pos < body.size())
	{
		if (body.compare(pos, delimiter.size(), delimiter) != 0)
			return (false);

		pos += delimiter.size();
		
		if (body.compare(pos, 2, "--") == 0)
			return (false);

		if (body.compare(pos, 2, "\r\n") != 0)
			return (false);

		pos += 2;
		
		std::string::size_type	headersEnd = body.find("\r\n\r\n", pos);
		
		if (headersEnd == std::string::npos)
			return (false);

		std::string headers = body.substr(pos, headersEnd - pos);
		
		std::string::size_type	dataStart = headersEnd + 4;
		
		std::string::size_type	nextBoundary = body.find("\r\n" + delimiter, dataStart);

		if (nextBoundary == std::string::npos)
			return (false);
		
		std::string	filename;
		std::string contentType;

		std::string::size_type lineStart = 0;
		
		while (lineStart < headers.size())
		{
			std::string::size_type lineEnd = headers.find("\r\n", lineStart);
			
			if (lineEnd == std::string::npos)
				lineEnd = headers.size();
			
			std::string line = headers.substr(lineStart, lineEnd - lineStart);
			
			std::string lowerLine = toLower(line);
			
			if (lowerLine.find("content-disposition:") == 0)
			{
				std::string::size_type filenamePos = lowerLine.find("filename=");
				
				if (filenamePos != std::string::npos)
				{
					filenamePos += 9;
					
					if (filenamePos < line.size() && line[filenamePos] == '"')
					{
						std::string::size_type filenameEnd = line.find('"', filenamePos + 1);
						
						if (filenameEnd == std::string::npos)
							return (false);

						filename = line.substr(filenamePos + 1, filenameEnd - filenamePos - 1);
					}
					else
					{
						std::string::size_type filenameEnd = line.find(';', filenamePos);

						if (filenameEnd == std::string::npos)
							return (false);
							
						filename = line.substr(filenamePos, filenameEnd - filenamePos);
					}
				}
			}
			else if (lowerLine.find("content-type:") == 0)
			{
				std::string::size_type colon = line.find(':');
				
				if (colon != std::string::npos)
					contentType = line.substr(colon + 1);
			}
			
			lineStart = lineEnd + 2;
		}
		
		if (!filename.empty())
		{
			file.filename = filename;
			file.contentType = contentType;
			file.data = body.substr(dataStart, nextBoundary - dataStart);
			
			return (true);
		}
		
		pos = nextBoundary + 2;
	}
	
	return (false);
}

bool	ResponseBuilder::isSafeUploadFilename(const std::string &filename) const
{
	if (filename.empty())
		return (false);

	if (filename == "." || filename == "..")
		return (false);

	if (filename.find("/") != std::string::npos)
		return (false);

	if (filename.find("\\") != std::string::npos)
		return (false);

	if (filename.find_first_of("\r\n") != std::string::npos)
		return (false);

	return (true);
}


std::string		ResponseBuilder::buildResponse(int statusCode, const std::string &reason, const std::string &contentType, const std::string &body, bool keepAlive, const std::string &extraHeaders) const
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

	response << extraHeaders;
	
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

std::string	ResponseBuilder::buildErrorResponse(int statusCode, const ServerBlock &server, const HttpRequest &request, const std::string &extraHeader) const
{	
	std::string body = buildDefaultErrorBody(statusCode);
	std::string	contentType = "text/html";
	
	std::string errorPath = getCustomErrorPagePath(server, statusCode);
	
	if (!errorPath.empty() && getResourceType(errorPath) == RESOURCE_FILE)
	{
		if (!readFile(errorPath, body))
			body = buildDefaultErrorBody(statusCode);
		else
			contentType = getMimeType(errorPath);
	}
	else
	{
		body = buildDefaultErrorBody(statusCode);
	}
	
	return (buildResponse(statusCode, getReasonPhrase(statusCode), "text/html", body, request.keepAlive, extraHeader));
}

std::string	ResponseBuilder::buildRedirectResponse(const LocationBlock &location, const HttpRequest &request) const
{
	int	code = location.redirectCode;
	const std::string &target = location.redirectTarget;

	if (code != 301 && code != 302 && code != 303 && code != 307 && code != 308)
		return ("");

	if (target.empty() || target.find_first_of("\r\n") != std::string::npos)
		return ("");

	std::string extraHeaders;

	extraHeaders = "Location: " + target + "\r\n";

	return (buildResponse(code, getReasonPhrase(code), "text/html", "", request.keepAlive, extraHeaders));
}

std::string	ResponseBuilder::buildAllowHeader(const LocationBlock *location) const
{
	if (location == NULL || location->methods.empty())
		return ("");

	std::ostringstream	header;
	
	header << "Allow: ";

	for (std::vector<std::string>::size_type i = 0; i < location->methods.size(); ++i)
	{
		if (i != 0)
			header << ", ";
		
		header << location->methods[i];
	}
	
	header << "\r\n";
	
	return (header.str());
}

std::string	ResponseBuilder::buildNoContentResponse(const HttpRequest &request) const
{
	std::ostringstream response;

	response << "HTTP/1.1 204 No Content\r\n";

	response << "Connection: "
			 << (request.keepAlive ? "keep-alive" : "close")
			 << "\r\n";

	response << "\r\n";

	return (response.str());
}

std::string	ResponseBuilder::buildDeleteResponse(const std::string &path, const ServerBlock &server, const HttpRequest &request) const
{
	ResourceType	type = getResourceType(path);

	if (type == RESOURCE_NOT_FOUND)
		return (buildErrorResponse(404, server, request));
		
	if (type == RESOURCE_DIRECTORY)
		return (buildErrorResponse(403, server, request));

	if (type == RESOURCE_OTHER)
		return (buildErrorResponse(403, server, request));

	if (type == RESOURCE_ERROR)
		return (buildErrorResponse(500, server, request));

	if (type != RESOURCE_FILE)
		return (buildErrorResponse(500, server, request));

	if (!deleteFile(path))
		return (buildErrorResponse(500, server, request));
		
	return (buildNoContentResponse(request));
}

std::string	ResponseBuilder::buildUploadPath(const std::string &normalizedPath, const LocationBlock &location) const
{
	if (location.uploadDir.empty())
		return ("");

	if (normalizedPath.size() < location.path.size())
		return ("");

	std::string suffix = normalizedPath.substr(location.path.size());

	if (suffix.empty() || suffix == "/")
		return ("");

	if (suffix[suffix.size() - 1] == '/')
		return ("");

	return (joinPaths(location.uploadDir, suffix));
}

std::string	ResponseBuilder::buildUploadResponse(const std::string &normalizedPath, const LocationBlock &location, const ServerBlock &server, const HttpRequest &request) const
{
	if (location.uploadDir.empty())
		return (buildErrorResponse(403, server, request));
	
	if (getResourceType(location.uploadDir) != RESOURCE_DIRECTORY)
	{
		return (buildErrorResponse(500, server, request));
	}

	std::string uploadPath = buildUploadPath(normalizedPath, location);
	
	if (uploadPath.empty())
		return (buildErrorResponse(400, server, request));
		
	ResourceType	targetType = getResourceType(uploadPath);
	
	if (targetType == RESOURCE_DIRECTORY || targetType == RESOURCE_OTHER)
		return (buildErrorResponse(403, server, request));
		
	if (targetType == RESOURCE_ERROR)
		return (buildErrorResponse(500, server, request));
	
	bool alreadyExist = (targetType == RESOURCE_FILE);
	
	if (!writeFile(uploadPath, request.body))
		return (buildErrorResponse(500, server, request));

	if (alreadyExist)
		return(buildNoContentResponse(request));
		
	return (buildResponse(201, getReasonPhrase(201), "text/plain", "", request.keepAlive));
}

std::string	ResponseBuilder::buildMultipartUploadResponse(const LocationBlock &location, const ServerBlock &server, const HttpRequest &request) const
{
	if (location.uploadDir.empty())
		return (buildErrorResponse(403, server, request));

	if (getResourceType(location.uploadDir) != RESOURCE_DIRECTORY)
		return (buildErrorResponse(500, server, request));

	std::string	boundary;

	if (!getMultipartBoundary(request, boundary))
		return (buildErrorResponse(400, server, request));
		
	MultipartFile	file;
	
	if (!parseMultipartFile(request.body, boundary, file))
		return (buildErrorResponse(400, server, request));

	if (!isSafeUploadFilename(file.filename))
		return (buildErrorResponse(400, server, request));
		
	std::string	uploadPath = joinPaths(location.uploadDir, file.filename);
	
	ResourceType targetType = getResourceType(uploadPath);

	if (targetType == RESOURCE_DIRECTORY || targetType == RESOURCE_OTHER)
		return (buildErrorResponse(403, server, request));

	if (targetType == RESOURCE_ERROR)
		return (buildErrorResponse(500, server, request));

	bool alreadyExists = (targetType == RESOURCE_FILE);

	if (!writeFile(uploadPath, file.data))
		return (buildErrorResponse(500, server, request));

	if (alreadyExists)
		return (buildNoContentResponse(request));
		
	return (buildResponse(201, getReasonPhrase(201), "text/plain", "", request.keepAlive));
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

	if (!isSupportedMethod(request.method))
	{
		std::string response =
			buildErrorResponse(501, *server, request);

		std::cout << "\n--- HTTP RESPONSE ---\n";
		std::cout << response;
		std::cout << "\n--- END RESPONSE ---\n";

		return ;
	}

	if (!isMethodAllowed(request.method, location))
	{
		std::string response =
			buildErrorResponse(
				405,
				*server,
				request,
				buildAllowHeader(location));

		std::cout << "\n--- HTTP RESPONSE ---\n";
		std::cout << response;
		std::cout << "\n--- END RESPONSE ---\n";

		return ;
	}
		
	if (location != NULL && location->redirectCode != 0)
	{
		std::string	response = buildRedirectResponse(*location, request);

		if (response.empty())
			response = buildErrorResponse(500, *server, request);

		std::cout << "\n--- HTTP RESPONSE ---\n" << std::endl;
		std::cout << response;
		std::cout << "\n--- END RESPONSE ---\n" << std::endl;

		return ;
	}

	std::string root =
		getEffectiveRoot(*server, location);

	std::string path =
		buildFilePath(normalizedPath, *server, location);

	if (path.empty())
	{
		std::cout << buildErrorResponse(
			500, *server, request);
		return ;
	}

	if (request.method == "DELETE")
	{
		std::string response =
			buildDeleteResponse(
				path,
				*server,
				request);

		std::cout << "\n--- HTTP RESPONSE ---\n";
		std::cout << response;
		std::cout << "\n--- END RESPONSE ---\n";

		return ;
	}

	if (request.method == "POST")
	{
		if (location == NULL)
		{
			std::cout << buildErrorResponse(
				404, *server, request);

			return ;
		}

		if (server->clientMaxBodySize != 0
			&& request.body.size() > server->clientMaxBodySize)
		{
			std::string	response = buildErrorResponse(
				413, *server, request);

			
			std::cout << "\n--- HTTP RESPONSE ---\n";
			std::cout << response;
			std::cout << "\n--- END RESPONSE ---\n";
			
			return ;
		}

		std::string response ;
		
		if (isMultipartRequest(request))
		{
			response = buildMultipartUploadResponse(*location, *server, request);
		}
		else
		{
			response = buildUploadResponse(normalizedPath, *location, *server, request);
		}

		std::cout << "\n--- HTTP RESPONSE ---\n";
		std::cout << response;
		std::cout << "\n--- END RESPONSE ---\n";

		return ;
	}
	
	if (request.method != "GET")
	{
		std::cout << "Method allowed but not implemented yet: "
				<< request.method
				<< std::endl;
		return ;
	}

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
		
		std::string	response = buildErrorResponse(403, *server, request);
		
		std::cout << response << std::endl;
		return ;
	}
	else if (type == RESOURCE_NOT_FOUND)
	{
		std::string	response = buildErrorResponse(404, *server, request);
		
		std::cout << response << std::endl;
		return ;
	}
	else if (type == RESOURCE_OTHER)
		std::cout << "resource : OTHER" << std::endl;
	else if (type == RESOURCE_ERROR)
	{
		std::string	response = buildErrorResponse(500, *server, request);
		
		std::cout << response << std::endl;
		return ;
	}
}