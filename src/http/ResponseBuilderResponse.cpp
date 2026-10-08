/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResponseBuilderResponse.cpp                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fmoulin <fmoulin@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/28 12:33:26 by fmoulin           #+#    #+#             */
/*   Updated: 2026/10/07 15:48:40 by fmoulin          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ResponseBuilder.hpp"
#include <iostream>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

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
	
	return (buildResponse(statusCode, getReasonPhrase(statusCode), contentType, body, request.keepAlive, extraHeader));
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
