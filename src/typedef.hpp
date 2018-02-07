#ifndef LIBMIMIIO_TYPEDEF_HPP__
#define LIBMIMIIO_TYPEDEF_HPP__

#include <stddef.h>
#include <memory>

/**
 * @file typedef.hpp
 * @brief libmimiio type definition.
 *
 * Callback function types are defined here and MUST match to mimi.h definition.
 *
 * @author Copyright (c) 2014-2015 Fairy Devices Inc. http://www.fairydevices.jp/
 * @author Masato Fujino
 */

/**
 * @brief Namespace for libmimiio.
 *
 * All of libmimiio is defined within the libmimiio namespace.
 */
namespace mimiio
{
	class mimiioController;
}

/**
 * @brief just encapsulation of mimimioMultiThread class
 */
struct mimi_io_s
{
	std::unique_ptr<mimiio::mimiioController> mt_;
};

/**
 * @brief On tx callback type definition
 *
 * @param [in] char* audio buffer for sending
 * @param [in] size_t* buffer length
 * @param [in] bool* recog_break flag defined in mimi(R) WebSocket API
 * @param [in] int* internal error code, set negative value when any error happens in user defined callback function
 * @param [in,out] void* user data
 */
typedef void (*ON_TX_CALLBACK_T)(char*, size_t*, bool*, int*,  void*);

/**
 * @brief On rx callback type definition
 *
 * @param [out] const char* response string buffer
 * @param [out] size_t length of the buffer
 * @param [in] int* internal error code, set negative value when any error happens in user defined callback function
 * @param [in,out] void* user data
 */
typedef void (*ON_RX_CALLBACK_T)(const char*, size_t, int*, void*);

#endif
