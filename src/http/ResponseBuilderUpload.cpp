/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResponseBuilderUpload.cpp                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fmoulin <fmoulin@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/28 12:33:26 by fmoulin           #+#    #+#             */
/*   Updated: 2026/10/07 15:53:51 by fmoulin          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ResponseBuilder.hpp"
#include <iostream>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

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
