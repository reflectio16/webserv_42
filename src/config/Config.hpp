/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Config.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: meelma <meelma@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/24 16:59:21 by fmoulin           #+#    #+#             */
/*   Updated: 2026/08/26 19:26:52 by meelma           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <vector>

struct Endpoint
{
	std::string	host;
	int			port;
	
	Endpoint() : host(""), port(0) {}
};

struct ServerBlock
{
	Endpoint	listenAddr;
	std::string	serverName;
	std::string	root;
};

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
	public:
		Config();
		Config(const std::string &filename);

		std::vector<Endpoint>	getEndpoints() const;
};

#endif