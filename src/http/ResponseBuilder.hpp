/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResponseBuilder.hpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fmoulin <fmoulin@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/28 12:26:27 by fmoulin           #+#    #+#             */
/*   Updated: 2026/10/07 15:45:33 by fmoulin          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RESPONSEBUILDER_HPP
#define RESPONSEBUILDER_HPP

# include "HttpRequest.hpp"
# include "../config/Config.hpp"
# include <vector>
# include <map>
# include <sys/stat.h>
# include <cerrno>
# include <fstream>
# include <sstream>
# include <dirent.h>
# include <cctype>

class ResponseBuilder
{
	public:
		ResponseBuilder(const Config &config);

		// TEMPORAIRE : tests
		void	debugRouting(const HttpRequest &request, const Endpoint &endpoint) const;
		
	private:
		enum ResourceType
		{
			RESOURCE_NOT_FOUND,
			RESOURCE_FILE,
			RESOURCE_DIRECTORY,
			RESOURCE_OTHER,
			RESOURCE_ERROR
		};

		struct	MultipartFile
		{
			std::string	filename;
			std::string	contentType;
			std::string	data;
		};
		
		const Config	&_config;
		
		std::string		hostWithoutPort(const std::string &host) const;
		std::string		getEffectiveRoot(const ServerBlock &server, const LocationBlock *location) const;
		std::string		joinPaths(const std::string &root, const std::string &suffix) const;
		std::string		buildFilePath(const std::string &normalizedPath, const ServerBlock &server, const LocationBlock *location) const;

		bool			normalizePath(const std::string &path, std::string &normalized) const;

		ResourceType	getResourceType(const std::string &path) const;
		
		bool			readFile(const std::string &path, std::string &content) const;
		std::string		getMimeType(const std::string &path) const;
		std::string		findIndexFile(const std::string &directoryPath, const LocationBlock *location) const;
		bool			buildAutoIndexBody(const std::string &directoryPath, const std::string &uriPath, std::string &body) const;
		std::string		getReasonPhrase(int statusCode) const;
		std::string		buildDefaultErrorBody(int statusCode) const;
		std::string		getCustomErrorPagePath(const ServerBlock &server, int statusCode) const;
		bool			isSupportedMethod(const std::string &method) const;
		bool			isMethodAllowed(const std::string &method, const LocationBlock *location) const;
		bool			deleteFile(const std::string &path) const;
		bool			writeFile(const std::string &path, const std::string &body) const;
		bool			isMultipartRequest(const HttpRequest &request) const;
		bool			getMultipartBoundary(const HttpRequest &request, std::string &boundary) const;
		bool			parseMultipartFile(const std::string &body, const std::string &boundary, MultipartFile &file) const;
		bool			isSafeUploadFilename(const std::string &filename) const;
		
		std::map<std::string, std::string>	buildCgiEnvironment(const HttpRequest &request, const ServerBlock &server, const Endpoint &endpoint, const std::string &scriptPath) const;
		std::string							headerToCgiName(const std::string &header) const;
		
		std::string		buildResponse(int statusCode, const std::string &reason, const std::string &contentType, const std::string &body, bool keepAlive, const std::string &extraHeaders = "") const;
		std::string		buildStaticFileResponse(const std::string &path, const HttpRequest &request) const;
		std::string		buildAutoindexResponse(const std::string &directoryPath, const std::string &uriPath, const HttpRequest &request) const;
		std::string		buildErrorResponse(int statusCode, const ServerBlock &server, const HttpRequest &request, const std::string &extraHeader = "") const;
		std::string		buildRedirectResponse(const LocationBlock &location, const HttpRequest &request) const;
		std::string		buildAllowHeader(const LocationBlock *location) const;
		std::string		buildNoContentResponse(const HttpRequest &request) const;
		std::string		buildDeleteResponse(const std::string &path, const ServerBlock &server, const HttpRequest &request) const;
		std::string		buildUploadPath(const std::string &normalizedPath, const LocationBlock &location) const;
		std::string		buildUploadResponse(const std::string &normalizedPath, const LocationBlock &location, const ServerBlock &server, const HttpRequest &request) const;
		std::string		buildMultipartUploadResponse(const LocationBlock &location, const ServerBlock &server, const HttpRequest &request) const;
		
		static std::string	toLower(const std::string &str);
	};

#endif