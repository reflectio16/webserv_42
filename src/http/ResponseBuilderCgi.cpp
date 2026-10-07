/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResponseBuilderCgi.cpp                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fmoulin <fmoulin@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/28 12:33:26 by fmoulin           #+#    #+#             */
/*   Updated: 2026/10/07 15:55:07 by fmoulin          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ResponseBuilder.hpp"
#include <iostream>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

std::map<std::string, std::string>	ResponseBuilder::buildCgiEnvironment(const HttpRequest &request, const ServerBlock &server, const Endpoint &endpoint, const std::string &scriptPath) const
{
	std::map<std::string, std::string> env;
	
	env["GATEWAY_INTERFACE"] = "CGI/1.1";
	env["REQUEST_METHOD"] = request.method;
	env["QUERY_STRING"] = request.query;
	env["SERVER_PROTOCOL"] = request.version;
	env["SERVER_NAME"] = server.serverName;
	env["SCRIPT_NAME"] = request.path;
	env["SCRIPT_FILENAME"] = scriptPath;

	std::ostringstream port;
	port << endpoint.port;
	env["SERVER_PORT"] = port.str();

	std::ostringstream length;
	length << request.body.size();
	env["CONTENT_LENGTH"] = length.str();

	std::map<std::string, std::string>::const_iterator	contentType = request.headers.find("content-type");

	if (contentType != request.headers.end())
		env["CONTENT_TYPE"] = contentType->second;

	for (std::map<std::string, std::string>::const_iterator it = request.headers.begin(); it != request.headers.end(); ++it)
	{
		if (it->first == "content-type" || it->first == "content-length")
			continue;
		
		env[headerToCgiName(it->first)] = it->second;
	}
		
	return (env);
}

std::string	ResponseBuilder::headerToCgiName(const std::string &header) const
{
	std::string	result = "HTTP_";
	
	for (std::string::size_type i = 0; i < header.size(); ++i)
	{
		if (header[i] == '-')
			result += '_';
		else
		{
			result += static_cast<char>(std::toupper(static_cast<unsigned char>(header[i])));
		}
	}
	
	return (result);
}
