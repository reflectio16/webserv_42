/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestParser.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fmoulin <fmoulin@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/08 17:36:27 by fmoulin           #+#    #+#             */
/*   Updated: 2026/09/10 16:10:53 by fmoulin          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "RequestParser.hpp"

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

	while (true)
	{
		if (_state == REQUEST_LINE)
		{
			std::string::size_type	pos = _buffer.find("\r\n");
	
			if (pos == std::string::npos)
				return ;
				
			std::string line = _buffer.substr(0, pos);
			
			_buffer.erase(0, pos + 2);
			
			parseRequestLine(line);
			
			_state = HEADERS;
			
			continue ;
		}
		
		if (_state == HEADERS)
		{
			std::string::size_type	pos = _buffer.find("\r\n");

			if (pos == std::string::npos)
				return ;

			if (pos == 0)
				return ;

			std::string	line = _buffer.substr(0, pos);
			
			_buffer.erase(0, pos + 2);
			
			parseHeaderLine(line);

			continue ;
		}
		
		return ;
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
		throw std::runtime_error("Invalid HTTP request line");

	if (stream >> extra)
		throw std::runtime_error("Invalid HTTP request line");

	_request.method = method;
	_request.target = target;
	_request.version = version;
}

void	RequestParser::parseHeaderLine(const std::string &line)
{
	std::string::size_type	colon = line.find(':');
	
	if (colon == std::string::npos || colon == 0)
		throw std::runtime_error("Invalid HTTP header");
		
	std::string	name = line.substr(0, colon);
	std::string	value = line.substr(colon + 1);
		
	for (std::string::size_type i = 0; i != name.size(); ++i)
	{
		if (name[i] == ' ' || name[i] == '\t')
			throw std::runtime_error("Invalid HTTP header name");
	}

	name = toLower(name);
	value = trim(value);
	
	_request.headers[name] = value;
}

std::string	RequestParser::toLower(const std::string &str)
{
	std::string result = str;

	for (std::string::size_type i = 0; i != result.size(); ++i)
	{
		result[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(result[i])));
	}
	return (result);
}

std::string	RequestParser::trim(const std::string &str)
{
	std::string::size_type start = str.find_first_not_of("\t");

	if (start == std::string::npos)
		return ("");

	std::string::size_type	end = str.find_last_not_of("\t");
	
	return (str.substr(start, end - start + 1));
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