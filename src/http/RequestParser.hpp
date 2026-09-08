/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestParser.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fmoulin <fmoulin@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/08 17:24:15 by fmoulin           #+#    #+#             */
/*   Updated: 2026/09/08 17:51:17 by fmoulin          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUESTPARSER_HPP
#define REQUESTPARSER_HPP

#include <string>
#include <cstddef>
#include "HttpRequest.hpp"

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
		State		_state;
		HttpRequest	_request;
		std::string	_buffer;
		size_t		_contentLength;
	public:
		RequestParser();
	
		void				feed(const char* data, size_t size);
		
		State				getState() const;
		bool				isComplete() const;
		const HttpRequest&	getRequest() const;
		
};

#endif