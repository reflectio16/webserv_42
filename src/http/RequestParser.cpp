/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestParser.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fmoulin <fmoulin@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/08 17:36:27 by fmoulin           #+#    #+#             */
/*   Updated: 2026/09/25 12:55:01 by fmoulin          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "RequestParser.hpp"
#include <sstream>
#include <cctype>
#include <iomanip>
#include <iostream>

RequestParser::RequestParser()
	:	_state(REQUEST_LINE),
		_request(),
		_contentLength(0),
		_pos(0),
		_bytesConsumed(0),
		_started(false),
		_chunkSize(0),
		_chunked(false)
{
	
}

RequestParser::Status	RequestParser::parse(const std::string& inbuf, std::size_t fromPos)
{
	try
	{
		if (!_started)
		{
			if (fromPos > inbuf.size())
				return (PARSE_ERROR);
				
			_pos = fromPos;
			_started = true;
		}

		while (true)
		{
			std::cout	<< "STATE = " << _state
						<< " POS = " << _pos
						<< " SIZE = " << inbuf.size()
						<< std::endl;
		  
			if (_state == REQUEST_LINE)
			{
				std::string::size_type	end = inbuf.find("\r\n", _pos);

				if (end == std::string::npos)
					return (INCOMPLETE);
				
				std::string	line = inbuf.substr(_pos, end - _pos);

				if (!parseRequestLine(line))
					return (PARSE_ERROR);

				_pos = end + 2;
				_state = HEADERS;
				continue ;
			}
			
			if (_state == HEADERS)
			{
				std::string::size_type	end = inbuf.find("\r\n", _pos);

				if (end == std::string::npos)
					return (INCOMPLETE);
				
				if (end == _pos)
				{
					_pos += 2;

					if (!finishHeaders())
						return (PARSE_ERROR);
						
					updateKeepAlive();
					
					if (_state != BODY && _state != CHUNK_SIZE)
					{
						_bytesConsumed = _pos;
						return (COMPLETE);
					}
					
					continue ;
				}
				
				std::string line = inbuf.substr(_pos, end - _pos);
				
				if (!parseHeaderLine(line))
					return (PARSE_ERROR);

				_pos = end + 2;
				continue ;
			}

			if (_state == BODY)
			{
				if (_pos > inbuf.size())
					return (PARSE_ERROR);
					
				if (inbuf.size() - _pos < _contentLength)
					return (INCOMPLETE);
					
				_request.body.assign(inbuf, _pos, _contentLength);
				
				_pos += _contentLength;
				_bytesConsumed = _pos;

				return (COMPLETE);
			}

			if (_state == CHUNK_SIZE)
			{
				std::string::size_type	end = inbuf.find("\r\n", _pos);
				
				if (end == std::string::npos)
					return (INCOMPLETE);
				
				std::string	line = inbuf.substr(_pos, end - _pos);
					
				if (!parseChunkSize(line, _chunkSize))
					return (PARSE_ERROR);

				_pos = end + 2;
				
				if (_chunkSize == 0)
				{
					_state = CHUNK_TRAILERS;
					continue;
				}

				_state = CHUNK_DATA;
				continue;
			}

			if (_state == CHUNK_DATA)
			{
				if (_pos > inbuf.size())
					return (PARSE_ERROR);
				
				if (inbuf.size() - _pos < _chunkSize)
					return (INCOMPLETE);

				if (_chunkSize > std::string::npos - _request.body.size())
					return (PARSE_ERROR);
					
				_request.body.append(inbuf, _pos, _chunkSize);

				_pos += _chunkSize;
				
				_state = CHUNK_DATA_CRLF;
				continue;
			}

			if (_state == CHUNK_DATA_CRLF)
			{
				if (inbuf.size() - _pos < 2)
					return (INCOMPLETE);

				if (inbuf.compare(_pos, 2, "\r\n") != 0)
					return (PARSE_ERROR);
					
				_pos += 2;

				_state = CHUNK_SIZE;
				continue;
			}

			if (_state == CHUNK_TRAILERS)
			{
				std::string::size_type	end = inbuf.find("\r\n", _pos);

				if (end == std::string::npos)
					return (INCOMPLETE);
				
				if (end == _pos)
				{
					_pos += 2;
					_bytesConsumed = _pos;

					return (COMPLETE);
				}

				_pos = end + 2;
				continue;
			}
		}
	}
	catch(...)
	{
		return (PARSE_ERROR);
	}
}

bool	RequestParser::parseRequestLine(const std::string &line)
{
	std::istringstream	stream(line);
	
	std::string	method;
	std::string	target;
	std::string	version;
	std::string	extra;

	if (!(stream >> method >> target >> version))
		return (false);

	if (stream >> extra)
		return (false);

	_request.method = method;
	_request.version = version;

	if (!validateVersion(version))
		return (false);
		
	std::string::size_type	question = target.find('?');

	if (question == std::string::npos)
	{
		_request.path = target;
		_request.query = "";
	}
	else
	{
		_request.path = target.substr(0, question);
		_request.query = target.substr(question + 1);
	}

	return (true);
}

bool	RequestParser::parseHeaderLine(const std::string &line)
{
	std::string::size_type	colon = line.find(':');
	
	if (colon == std::string::npos || colon == 0)
		return (false);
		
	std::string	name = line.substr(0, colon);
	std::string	value = line.substr(colon + 1);
		
	for (std::string::size_type i = 0; i != name.size(); ++i)
	{
		if (name[i] == ' ' || name[i] == '\t')
			return (false);
	}

	name = toLower(name);
	value = trim(value);
	
	_request.headers[name] = value;

	return (true);
}

bool	RequestParser::finishHeaders()
{
	_contentLength = 0;
	_chunked = false;
	
	std::map<std::string, std::string>::const_iterator	contentLength;
	std::map<std::string, std::string>::const_iterator	transferEncoding;
	
	
	contentLength = _request.headers.find("content-length");
	transferEncoding = _request.headers.find("transfer-encoding");

	if (contentLength != _request.headers.end()
		&& transferEncoding != _request.headers.end())
		return (false);
		
	if (transferEncoding != _request.headers.end())
	{
		std::string	value = toLower(trim(transferEncoding->second));
		
		if (value != "chunked")
			return (false);
		
		_chunked = true;
		_state = CHUNK_SIZE;

		return (true);
	}
	
	if (contentLength == _request.headers.end())
		return (true);

	const std::string& value = contentLength->second;

	if (value.empty())
		return (false);
		
	for (std::string::size_type i = 0; i < value.size(); ++i)
	{
		if (!std::isdigit(static_cast<unsigned char>(value[i])))
			return (false);
	}

	std::istringstream	stream(value);

	if (!(stream >> _contentLength))
		return (false);
		
	if (_contentLength > 0)
		_state = BODY;
	
	return (true);
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
	std::string::size_type start = str.find_first_not_of(" \t");

	if (start == std::string::npos)
		return ("");

	std::string::size_type	end = str.find_last_not_of(" \t");
	
	return (str.substr(start, end - start + 1));
}

std::size_t			RequestParser::bytesConsumed() const
{
	return (_bytesConsumed);
}


void	RequestParser::reset()
{
	_state = REQUEST_LINE;
	_request = HttpRequest();
	_contentLength = 0;
	_pos = 0;
	_bytesConsumed = 0;
	_started = false;
	_chunkSize = 0;
	_chunked = false;
}

const HttpRequest&	RequestParser::request() const
{
	return (_request);
}

void	RequestParser::updateKeepAlive()
{
	_request.keepAlive = (_request.version == "HTTP/1.1");

	std::map<std::string, std::string>::const_iterator	it = _request.headers.find("connection");

	if (it == _request.headers.end())
		return ;

	std::string	value = toLower(trim(it->second));

	if (value == "close")
		_request.keepAlive = false;
	else if (value == "keep-alive")
		_request.keepAlive = true;
}

bool	RequestParser::parseChunkSize(const std::string& line, std::size_t& size)
{
	std::string	value = line;
	
	std::string::size_type	semicolon = value.find(';');

	if (semicolon != std::string::npos)
		value = value.substr(0, semicolon);

	value = trim(value);

	if (value.empty())
		return (false);

	for (std::string::size_type i = 0; i < value.size(); ++i)
	{
		if (!isxdigit(static_cast<unsigned char>(value[i])))
			return (false);
	}
	
	std::istringstream	stream(value);

	stream >> std::hex >> size;

	if (stream.fail())
		return (false);
	
	return (true);
}

bool	RequestParser::validateVersion(const std::string& version) const
{
		return (version == "HTTP/1.1" || version == "HTTP/1.0");
}
