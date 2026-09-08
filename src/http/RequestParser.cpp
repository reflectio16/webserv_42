/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestParser.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fmoulin <fmoulin@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/08 17:36:27 by fmoulin           #+#    #+#             */
/*   Updated: 2026/09/08 17:58:58 by fmoulin          ###   ########.fr       */
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