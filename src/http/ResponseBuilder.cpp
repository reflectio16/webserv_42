/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResponseBuilder.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fmoulin <fmoulin@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/28 12:33:26 by fmoulin           #+#    #+#             */
/*   Updated: 2026/10/08 16:32:23 by fmoulin          ###   ########.fr       */
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

Outcome	ResponseBuilder::makeResponseOutcome(const std::string &response, bool keepAlive) const
{
	Outcome	outcome;
	
	outcome.kind = Outcome::RESPONSE;
	outcome.keepAlive = keepAlive;
	outcome.responseBytes = response;

	return (outcome);
}

Outcome	ResponseBuilder::makeCgiOutcome(const HttpRequest &request, const ServerBlock &server, const Endpoint &endpoint, const std::string &scriptPath, const std::string &interpreter) const
{
	Outcome	outcome;

	outcome.kind = Outcome::CGI;
	outcome.keepAlive = request.keepAlive;
	outcome.cgiInterpreter = interpreter;
	outcome.cgiScriptPath = scriptPath;
	outcome.cgiBody = request.body;
	
	std::map<std::string, std::string> env = buildCgiEnvironment(request, server, endpoint, scriptPath);
	outcome.cgiEnv = cgiEnvToVector(env);

	return (outcome);
}

Outcome	ResponseBuilder::build(const HttpRequest &request, Endpoint &endpoint) const
{
	std::string	host;

	std::map<std::string, std::string>::const_iterator hostIt = request.headers.find("host");
	
	if (hostIt != request.headers.end())
		host = hostWithoutPort(hostIt->second);

	const ServerBlock *server = _config.findServer(endpoint, host);
	
	if (server == NULL)
	{
		std::string body = buildDefaultErrorBody(500);

		std::string	response = buildResponse(500, getReasonPhrase(500), "text/html", body, false);
		
		return (makeResponseOutcome(response, false));
	}

	std::string	normalizedPath;

	if (!normalizePath(request.path, normalizedPath))
		return (makeResponseOutcome(buildErrorResponse(400, *server, request), request.keepAlive));

	const LocationBlock *location = _config.findLocation(*server, normalizedPath);
	
	// 1. Méthode HTTP non implémentée par Webserv
	if (!isSupportedMethod(request.method))
	{
		return (makeResponseOutcome(
			buildErrorResponse(
				501,
				*server,
				request),
			request.keepAlive));
	}

	// 2. Méthode connue, mais interdite sur cette location
	if (!isMethodAllowed(request.method, location))
	{
		return (makeResponseOutcome(
			buildErrorResponse(
				405,
				*server,
				request,
				buildAllowHeader(location)),
			request.keepAlive));
	}

	// 3. Body trop gros
	if (server->clientMaxBodySize != 0
		&& request.body.size() > server->clientMaxBodySize)
	{
		return (makeResponseOutcome(
			buildErrorResponse(
				413,
				*server,
				request),
			request.keepAlive));
	}

	// 4. Redirection configurée
	if (location != NULL && location->redirectCode != 0)
	{
		std::string response =
			buildRedirectResponse(*location, request);

		if (response.empty())
		{
			response =
				buildErrorResponse(
					500,
					*server,
					request);
		}

		return (makeResponseOutcome(
			response,
			request.keepAlive));
	}

	std::string path =
		buildFilePath(
			normalizedPath,
			*server,
			location);

	if (path.empty())
	{
		return (makeResponseOutcome(
			buildErrorResponse(
				500,
				*server,
				request),
			request.keepAlive));
	}

	std::string interpreter;

	if (findCgiInterpreter(
			location,
			normalizedPath,
			interpreter))
	{
		ResourceType type =
			getResourceType(path);

		if (type == RESOURCE_NOT_FOUND)
		{
			return (makeResponseOutcome(
				buildErrorResponse(
					404,
					*server,
					request),
				request.keepAlive));
		}

		if (type == RESOURCE_DIRECTORY
			|| type == RESOURCE_OTHER)
		{
			return (makeResponseOutcome(
				buildErrorResponse(
					403,
					*server,
					request),
				request.keepAlive));
		}

		if (type == RESOURCE_ERROR)
		{
			return (makeResponseOutcome(
				buildErrorResponse(
					500,
					*server,
					request),
				request.keepAlive));
		}

		return (makeCgiOutcome(
			request,
			*server,
			endpoint,
			path,
			interpreter));
	}

	if (request.method == "DELETE")
	{
		return (makeResponseOutcome(
			buildDeleteResponse(
				path,
				*server,
				request),
			request.keepAlive));
	}

	if (request.method == "POST")
	{
		if (location == NULL)
		{
			return (makeResponseOutcome(
				buildErrorResponse(
					404,
					*server,
					request),
				request.keepAlive));
		}

		std::string response;

		if (isMultipartRequest(request))
		{
			response =
				buildMultipartUploadResponse(
					*location,
					*server,
					request);
		}
		else
		{
			response =
				buildUploadResponse(
					normalizedPath,
					*location,
					*server,
					request);
		}

		return (makeResponseOutcome(
			response,
			request.keepAlive));
	}

	ResourceType type =
		getResourceType(path);

	if (type == RESOURCE_FILE)
	{
		std::string response =
			buildStaticFileResponse(
				path,
				request);

		if (response.empty())
		{
			response =
				buildErrorResponse(
					500,
					*server,
					request);
		}

		return (makeResponseOutcome(
			response,
			request.keepAlive));
	}

	if (type == RESOURCE_DIRECTORY)
	{
		std::string indexPath =
			findIndexFile(
				path,
				location);

		if (!indexPath.empty())
		{
			std::string response =
				buildStaticFileResponse(
					indexPath,
					request);

			if (response.empty())
			{
				response =
					buildErrorResponse(
						500,
						*server,
						request);
			}

			return (makeResponseOutcome(
				response,
				request.keepAlive));
		}

		if (location != NULL
			&& location->autoindex)
		{
			std::string response =
				buildAutoindexResponse(
					path,
					normalizedPath,
					request);

			if (response.empty())
			{
				response =
					buildErrorResponse(
						500,
						*server,
						request);
			}

			return (makeResponseOutcome(
				response,
				request.keepAlive));
		}

		return (makeResponseOutcome(
			buildErrorResponse(
				403,
				*server,
				request),
			request.keepAlive));
	}

	if (type == RESOURCE_NOT_FOUND)
	{
		return (makeResponseOutcome(
			buildErrorResponse(
				404,
				*server,
				request),
			request.keepAlive));
	}

	if (type == RESOURCE_OTHER)
	{
		return (makeResponseOutcome(
			buildErrorResponse(
				403,
				*server,
				request),
			request.keepAlive));
	}

	return (makeResponseOutcome(
		buildErrorResponse(
			500,
			*server,
			request),
		request.keepAlive));
}
