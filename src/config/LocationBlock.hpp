/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   LocationBlock.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fmoulin <fmoulin@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/27 15:00:27 by fmoulin           #+#    #+#             */
/*   Updated: 2026/08/27 15:19:35 by fmoulin          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef LOCATIONBLOCK_HPP
#define LOCATIONBLOCK_HPP

#include <string>
#include <vector>
#include <map>

struct LocationBlock
{
	std::string							path;
	std::vector<std::string>			method;
	
	std::string							root;
	std::string 						index;
	bool								autoindex;
	
	std::string							uploadDir;
	
	int									redirectCode;
	std::string							redirectTarget;

	std::map<std::string, std::string>	cgiHandlers;

    LocationBlock()
        : path(""),
          root(""),
          index(""),
          autoindex(false),
          uploadDir(""),
          redirectCode(0),
          redirectTarget("")
    {
	}	
};

#endif