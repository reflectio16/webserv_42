/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test_routing.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fmoulin <fmoulin@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/08 15:59:32 by fmoulin           #+#    #+#             */
/*   Updated: 2026/10/08 16:50:20 by fmoulin          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>

#include "config/Config.hpp"
#include "http/ResponseBuilder.hpp"
#include "http/HttpRequest.hpp"

static HttpRequest	makeRequest(const std::string &path)
{
	HttpRequest	request;

	request.method = "POST";
	request.path = path;
	request.query = "";
	request.version = "HTTP/1.1";
	request.body = "hello=world";
	request.headers["content-type"] = "application/x-www-form-urlencoded";
	request.keepAlive = true;

	return (request);
}

static void	printOutcome(const Outcome &outcome)
{
	if (outcome.kind == Outcome::RESPONSE)
	{
		std::cout << "===== RESPONSE =====" << std::endl;
		std::cout << outcome.responseBytes << std::endl;
		return ;
	}

	std::cout << "===== CGI =====" << std::endl;

	std::cout
		<< "Interpreter : "
		<< outcome.cgiInterpreter
		<< std::endl;

	std::cout
		<< "Script      : "
		<< outcome.cgiScriptPath
		<< std::endl;

	std::cout
		<< "Body        : "
		<< outcome.cgiBody
		<< std::endl;

	std::cout
		<< "KeepAlive   : "
		<< outcome.keepAlive
		<< std::endl;

	std::cout << "Environment :" << std::endl;

	for (std::vector<std::string>::const_iterator it =
			outcome.cgiEnv.begin();
		it != outcome.cgiEnv.end();
		++it)
	{
		std::cout
			<< "  "
			<< *it
			<< std::endl;
	}
}

int	main(int argc, char **argv)
{
	if (argc != 2)
	{
		std::cerr
			<< "usage: "
			<< argv[0]
			<< " <config file>"
			<< std::endl;

		return (1);
	}

	try
	{
		Config	config(argv[1]);

		ResponseBuilder	builder(config);

		Endpoint	endpoint;

		endpoint.host = "0.0.0.0";
		endpoint.port = 8080;

		std::cout
			<< "\n===== TEST 1: normal path =====\n"
			<< std::endl;

		HttpRequest request1 =
			makeRequest("/cgi/inexistant.py");

		Outcome outcome = builder.build(request1, endpoint);

		printOutcome(outcome);

		std::cout
			<< "\n===== TEST 2: normalized path =====\n"
			<< std::endl;

		HttpRequest request2 =
			makeRequest("/images/../index.html");

		outcome = builder.build(request2, endpoint);

		printOutcome(outcome);
	}
	catch (const std::exception &e)
	{
		std::cerr
			<< "error: "
			<< e.what()
			<< std::endl;

		return (1);
	}

	return (0);
}
