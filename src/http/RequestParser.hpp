/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestParser.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fmoulin <fmoulin@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/08 17:24:15 by fmoulin           #+#    #+#             */
/*   Updated: 2026/09/25 12:45:38 by fmoulin          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUESTPARSER_HPP
#define REQUESTPARSER_HPP

#include "HttpRequest.hpp"
#include <string>
#include <cstddef>

class RequestParser
{
	public:
		enum Status
		{
			INCOMPLETE,
			COMPLETE,
			PARSE_ERROR	
		};

		RequestParser();
	
		Status				parse(const std::string& inbuf, std::size_t fromPos);
		
		std::size_t			bytesConsumed() const;
		void				reset();
		const HttpRequest&	request() const;
		
		
	private:
		enum ParseState
		{
			REQUEST_LINE,
			HEADERS,
			BODY,
			CHUNK_SIZE,
			CHUNK_DATA,
			CHUNK_DATA_CRLF,
			CHUNK_TRAILERS
		};
		
		ParseState			_state;
		HttpRequest			_request;
		
		std::size_t			_contentLength;
		std::size_t			_pos;
		std::size_t			_bytesConsumed;

		bool				_started;

		std::size_t			_chunkSize;
		bool				_chunked;
		
		bool				parseRequestLine(const std::string &line);
		bool				parseHeaderLine(const std::string &line);
		bool				finishHeaders();
		
		bool				parseChunkSize(const std::string& line, std::size_t& size);

		bool				validateVersion(const std::string& version) const;

		static std::string	toLower(const std::string &str);
		static std::string	trim(const std::string &str);

		void				updateKeepAlive();
};

#endif