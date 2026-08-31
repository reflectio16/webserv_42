/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Config.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fmoulin <fmoulin@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/24 16:59:21 by fmoulin           #+#    #+#             */
/*   Updated: 2026/08/31 18:25:54 by fmoulin          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <vector>
#include "ServerBlock.hpp"

class Config
{
	private:
		std::vector<ServerBlock>	_servers;
		std::string					readFile(const std::string &filename) const;
		std::vector<std::string>	tokenize(const std::string &content) const;
		
		void						parse(const std::vector<std::string> &tokens);
		
		void						parseListen(ServerBlock &server, const std::vector<std::string> &tokens, size_t &i);
		void						parseServerName(ServerBlock &server, const std::vector<std::string> &tokens, size_t &i);
		void						parseRoot(ServerBlock &server, const std::vector<std::string> &tokens, size_t &i);
		void						parseClientMaxBodySize(ServerBlock &server, const std::vector<std::string> &tokens, size_t &i);
		void						parseErrorPage(ServerBlock &server, const std::vector<std::string> &tokens, size_t &i);
		
		void						parseLocation(ServerBlock &server, const std::vector<std::string> &tokens, size_t &i);
		
		void						parseMethod(LocationBlock &location, const std::vector<std::string> &tokens, size_t &i);
		void						parseLocationRoot(LocationBlock &location, const std::vector<std::string> &tokens, size_t &i);
		void						parseIndex(LocationBlock &location, const std::vector<std::string> &tokens, size_t &i);
		void						parseAutoIndex(LocationBlock &location, const std::vector<std::string> &tokens, size_t &i);
	public:
		Config();
		Config(const std::string &filename);

		std::vector<Endpoint>	getEndpoints() const;
};

#endif