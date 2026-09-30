/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResponseBuilder.hpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fmoulin <fmoulin@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/28 12:26:27 by fmoulin           #+#    #+#             */
/*   Updated: 2026/09/30 16:52:18 by fmoulin          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RESPONSEBUILDER_HPP
#define RESPONSEBUILDER_HPP

# include "HttpRequest.hpp"
# include "../config/Config.hpp"
# include <vector>
# include <sys/stat.h>
# include <cerrno>
# include <fstream>
# include <sstream>
# include <dirent.h>

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
		
		const Config	&_config;
		
		std::string		hostWithoutPort(const std::string &host) const;
		std::string		getEffectiveRoot(const ServerBlock &server, const LocationBlock *location) const;
		std::string		joinPaths(const std::string &root, const std::string &suffix) const;
		std::string		buildFilePath(const std::string &normalizedPath, const ServerBlock &server, const LocationBlock *location) const;

		bool			normalizePath(const std::string &path, std::string &normalized) const;

		ResourceType	getResourceType(const std::string &path) const;
		
		bool			readFile(const std::string &path, std::string &content) const;
		std::string		getMimeType(const std::string &path) const;
		std::string		sizeToString(std::size_t value) const;
		std::string		findIndexFile(const std::string &directoryPath, const LocationBlock *location) const;
		bool			buildAutoIndexBody(const std::string &directoryPath, const std::string &uriPath, std::string &body) const;
		
		std::string		buildResponse(int statusCode, const std::string &reason, const std::string &contentType, const std::string &body, bool keepAlive) const;
		std::string		buildStaticFileResponse(const std::string &path, const HttpRequest &request) const;
		std::string		buildAutoindexResponse(const std::string &directoryPath, const std::string &uriPath, const HttpRequest &request) const;
		
		static std::string	toLower(const std::string &str);
	};

#endif