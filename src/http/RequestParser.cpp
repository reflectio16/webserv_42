/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestParser.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fmoulin <fmoulin@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/08 17:36:27 by fmoulin           #+#    #+#             */
/*   Updated: 2026/09/09 16:44:03 by fmoulin          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "RequestParser.hpp"
#include <sstream>
#include <stdexcept>

RequestParser::RequestParser()
	:	_state(REQUEST_LINE),
		_request(),
		_buffer(""),
		_contentLength(0)
{
	
}

void	RequestParser::feed(const char* data, size_t size)
{
	_buffer.append(data, size);

	if (_state == REQUEST_LINE)
	{
		std::string::size_type	pos = _buffer.find("\r\n");

		if (pos == std::string::npos)
			return ;
			
		std::string line = _buffer.substr(0, pos);
		
		_buffer.erase(0, pos + 2);
		
		parseRequestLine(line);
		
		_state = HEADERS;
	}
}

void	RequestParser::parseRequestLine(const std::string &line)
{
	std::istringstream	stream(line);
	
	std::string	method;
	std::string	target;
	std::string	version;
	std::string	extra;

	if (!(stream >> method >> target >> version))
		throw std::runtime_error("HTTP request incomplete");

	if (stream >> extra)
		throw std::runtime_error("HTTP request incomplete");

	_request.method = method;
	_request.target = target;
	_request.version = version;
}

RequestParser::State	RequestParser::getState() const
{
	return (_state);
}

bool	RequestParser::isComplete() const
{
	return (_state == COMPLETE);
}

const HttpRequest&	RequestParser::getRequest() const
{
	return (_request);
}