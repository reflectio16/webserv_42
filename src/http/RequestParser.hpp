/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestParser.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: meelma <meelma@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/21 15:57:56 by meelma            #+#    #+#             */
/*   Updated: 2026/09/21 15:58:13 by meelma           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUESTPARSER_HPP
#define REQUESTPARSER_HPP
#include <string>
#include <cstddef>

// TEMPORARY STUB. Same class name + interface as the real one, so it swaps in.
class RequestParser {
    
public:
    enum Status { INCOMPLETE, COMPLETE, PARSE_ERROR };
    RequestParser() : _end(0) {}

    Status parse(const std::string& inbuf, std::size_t fromPos) {
        std::size_t hdrEnd = inbuf.find("\r\n\r\n", fromPos);
        if (hdrEnd == std::string::npos) return INCOMPLETE;
        _end = hdrEnd + 4;
        return COMPLETE;   // stub: "complete" = headers ended. Ignores body/Content-Length.
    }
    std::size_t bytesConsumed() const { return _end; }
    void        reset()               { _end = 0; }
    
private:
    std::size_t _end;
};

#endif