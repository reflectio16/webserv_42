/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestParser.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fmoulin <fmoulin@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/08 17:24:15 by fmoulin           #+#    #+#             */
/*   Updated: 2026/09/09 18:26:53 by fmoulin          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUESTPARSER_HPP
#define REQUESTPARSER_HPP

#include "HttpRequest.hpp"
#include <string>
#include <cstddef>
#include <sstream>
#include <stdexcept>
#include <cctype>

class RequestParser
{
	public:
		enum State
		{
			REQUEST_LINE,
			HEADERS,
			BODY,
			COMPLETE
		};
		
	private:
		State				_state;
		HttpRequest			_request;
		std::string			_buffer;
		size_t				_contentLength;
		
		void				parseRequestLine(const std::string &line);
		void				parseHeaderLine(const std::string &line);

		static std::string	toLower(const std::string &str);
		static std::string	trim(const std::string &str);
		
	public:
		RequestParser();
	
		void				feed(const char* data, size_t size);
		
		State				getState() const;
		bool				isComplete() const;
		const HttpRequest&	getRequest() const;
};

#endif