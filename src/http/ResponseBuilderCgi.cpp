/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResponseBuilderCgi.cpp                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: meelma <meelma@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/28 12:33:26 by fmoulin           #+#    #+#             */
/*   Updated: 2026/10/11 14:17:02 by meelma           ###   ########.fr       */
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

std::vector<std::string>	ResponseBuilder::cgiEnvToVector(const std::map<std::string, std::string> &env) const
{
	std::vector<std::string>	result;
	
	for (std::map<std::string, std::string>::const_iterator it = env.begin(); it != env.end(); ++it)
	{
		result.push_back(it->first + "=" + it->second);
	}
	
	return (result);
}

bool	ResponseBuilder::findCgiInterpreter(const LocationBlock *location, const std::string &path, std::string &interpreter) const
{
	if (location == NULL)
		return (false);

	std::string::size_type dot = path.rfind('.');

	if (dot == std::string::npos)
		return (false);

	std::string extension = path.substr(dot);
	
	std::map<std::string, std::string>::const_iterator it = location->cgiHandlers.find(extension);
	
	if (it == location->cgiHandlers.end())
		return (false);

	interpreter = it->second;

	return (true);
}

std::string ResponseBuilder::finalizeCgi(const std::string &cgiOut, bool keepAlive) const
{
    // 1. Find the blank line that separates CGI headers from the body.
    //    Accept both \r\n\r\n and \n\n (scripts are inconsistent).
    std::string headerBlock;
    std::string body;
    std::string::size_type sep = cgiOut.find("\r\n\r\n");
    std::string::size_type sepLen = 4;
    if (sep == std::string::npos) {
        sep = cgiOut.find("\n\n");
        sepLen = 2;
    }
    if (sep == std::string::npos) {
        // No header/body separator at all -> treat the whole thing as body,
        // with a default Content-Type.
        headerBlock = "";
        body = cgiOut;
    } else {
        headerBlock = cgiOut.substr(0, sep);
        body = cgiOut.substr(sep + sepLen);
    }
 
    // 2. Walk the CGI header lines. Pull out "Status:" (sets HTTP status),
    //    pass everything else through unchanged.
    int         statusCode   = 200;
    std::string statusReason = "OK";
    std::string passedHeaders;          // the CGI headers we forward as-is
 
    std::string::size_type pos = 0;
    while (pos < headerBlock.size()) {
        std::string::size_type eol = headerBlock.find('\n', pos);
        std::string line = (eol == std::string::npos)
                           ? headerBlock.substr(pos)
                           : headerBlock.substr(pos, eol - pos);
        // strip a trailing '\r'
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
 
        if (!line.empty()) {
            // case-insensitive check for "status:"
            std::string lower = line;
            for (std::string::size_type i = 0; i < lower.size(); ++i)
                lower[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(lower[i])));
 
            if (lower.compare(0, 7, "status:") == 0) {
                // e.g. "Status: 404 Not Found"
                std::string value = line.substr(7);
                // trim leading spaces
                std::string::size_type s = value.find_first_not_of(" \t");
                if (s != std::string::npos)
                    value = value.substr(s);
                std::istringstream iss(value);
                int code;
                if (iss >> code) {
                    statusCode = code;
                    std::string rest;
                    std::getline(iss, rest);
                    std::string::size_type r = rest.find_first_not_of(" \t");
                    statusReason = (r != std::string::npos) ? rest.substr(r) : "";
                }
            } else {
                // forward this CGI header into the HTTP response
                passedHeaders += line;
                passedHeaders += "\r\n";
            }
        }
 
        if (eol == std::string::npos)
            break;
        pos = eol + 1;
    }
 
    // 3. Build the HTTP response.
    std::ostringstream oss;
    oss << "HTTP/1.1 " << statusCode << " " << statusReason << "\r\n";
    oss << passedHeaders;                       // the CGI headers (Content-Type, etc.)
    oss << "Content-Length: " << body.size() << "\r\n";
    oss << "Connection: " << (keepAlive ? "keep-alive" : "close") << "\r\n";
    oss << "\r\n";
    oss << body;
    return oss.str();
}

