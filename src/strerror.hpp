#ifndef MIMIIOSTRERROR_HPP__
#define MIMIIOSTRERROR_HPP__

#include <string>

namespace mimiio{

/**
 * @file strerror.hpp
 * @brief libmimiio error code to string map, other codes may be remote host's (pass through)
 * @author Copyright (c) 2014-2015 Fairy Devices Inc. http://www.fairydevices.jp/
 * @author Masato Fujino
 */

/**
 * @brief libmimiio error code to error string mapping function
 *
 * @param [in] errorno error number
 * @return error string
 */
inline std::string strerror(int errorno)
{
	if(errorno < 0){
		return "user defined error.";
	}

	//error in libmimiio or remote host
	switch (errorno){
	  case 0:
		  return "no error.";
	  case 101: // 100s' are misc error except for other explicit errors.
		  return "unknown error.";
	  case 501: // 500s' are audio encoder error
		  return "encoder initialization error.";
	  case 502:
		  return "encoder processing error.";
	  case 601: // 600s' are SSL error
		  return "SSL client context error.";
	  case 602:
		  return "SSL invalid certificate error.";
	  case 603:
		  return "SSL certificate validation error.";
	  case 604:
		  return "SSL unexpectedly connection closed.";
	  case 605:
		  return "SSL error, server certificate validation error.";
	  case 701: // 700s' are network errors, including server processing limit exceeding.
		  return "host not found.";
	  case 703:
		  return "timed out for establishing connection.";
	  case 704:
		  return "connection refused by remote host.";
	  case 705:
		  return "connection reset by peer, which means exceeded simultaneous processing limit.";
	  case 790:
		  return "network error.";
	  case 791:
		  return "unexpected network disconnection.";
	  case 799:
		  return "undefined network error.";
	  case 801: // 800s' are WebSocket opening handshake errors.
		  return "WebSocket connection error no handshake, no connection.";
	  case 802:
		  return "WebSocket connection error no version, no sec-websocket-version header in handshake request.";
	  case 803:
		  return "WebSocket connection error unsupported version.";
	  case 804:
		  return "WebSocket connection error no key, no sec-websocket-key header in handshake request.";
	  case 805:
		  return "WebSocket connection error handshake accept, no sec-websocket-accept header or invalid.";
	  case 806:
		  return "WebSocket connection error unauthorized.";
	  case 810:
		  return "WebSocket connection error payload is too big.";
	  case 811:
		  return "WebSocket connection error incomplete frame received.";
	  case 830:
		  return "WebSocket receive frame timeout";
	  case 890:
		  return "WebSocket unknown flag received.";
	  case 901: // 900s' are errors about user defined callback functions and about WebSocket communication.
		  return "tx_func is not set. Must mimi_txfunc() before start().";
	  case 902:
		  return "rx_func is not set. Must mimi_rxfunc() before start().";
	  case 903:
		  return "audio buffer is over maximum payload size 65536. ";
	  case 904:
		  return "close frame received from remote host normally, but close status is not set.";
	  case 905:
		  return "could not start API.";
	  case 906:
		  return "received zero length text frame.";
	  case 907:
		  return "received zero length binary frame.";
	  case 1000: // 1000s' are errors defined in RFC 6455
		  return "WebSocket connection closed by host, no error, normal close.";
	  case 1001:
		  return "WebSocket connection closed by host, end-point going away.";
	  case 1002:
		  return "WebSOcket connection closed by host, protocol error.";
	  case 1003:
		  return "WebSocket connection closed by host, payload not acceptable.";
	  case 1004:
		  return "WebSocket connection closed by host, reserved.";
	  case 1005:
		  return "WebSocket connection closed by host, reserved no status code.";
	  case 1006:
		  return "WebSocket connection closed by host, reserved abnormal close.";
	  case 1007:
		  return "WebSocket connection closed by host, malformed payload.";
	  case 1008:
		  return "WebSocket connection closed by host, policy violation.";
	  case 1009:
		  return "WebSocket connection closed by host, payload too big.";
	  case 1010:
		  return "WebSocket connection closed by host, extension required.";
	  case 1011:
		  return "WebSocket connection closed by host, unexpected condition.";
	  case 1015:
		  return "WebSocket connection closed by host, reserved TLS failure.";
	  default: // Over 1000s' are errors from remote host
		  return "Remote host could not process your request normally, remote host error code is shown.";
	  }
}

}

#endif
