/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fmoulin <fmoulin@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/08 16:23:07 by fmoulin           #+#    #+#             */
/*   Updated: 2026/09/22 17:33:48 by fmoulin          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <string>
#include <map>

struct HttpRequest
{
	std::string	method;
	std::string	path;
	std::string	query;
	std::string	version;
	std::map<std::string, std::string>	headers;
	std::string	body;
	bool		keepAlive;
	
	HttpRequest()
		: 	method(""),
			path(""),
			query(""),
			version(""),
			body(""),
			keepAlive(false)
	{
	}
};

#endif
