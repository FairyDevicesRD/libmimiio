/**
 * @file mimiio.h
 * @brief libmimiio public external API definition.
 * @ingroup public_header
 *
 * See README.md how to use libmimiio in detail.
 *
 * @author Copyright (c) 2014-2015 Fairy Devices Inc. http://www.fairydevices.jp/
 * @author Masato Fujino
 */

#ifndef LIBMIMIIO_MIMIIO_H__
#define LIBMIMIIO_MIMIIO_H__

#include <stdlib.h>   // size_t
#include <stdbool.h>  // bool
#ifdef __cplusplus
extern "C"{
#endif

  /**
   * @brief mimi connection handler
   */
  typedef struct mimi_io_s MIMI_IO;

  /**
   * @brief HTTP request header
   */
  typedef struct{
	  char key[1024];
	  char value[1024];
  } MIMIIO_HTTP_REQUEST_HEADER;

  /**
   * @brief Stream Status
   */
  typedef enum{
	  MIMIIO_STREAM_WAIT,   //!< Stream is not started.
	  MIMIIO_STREAM_CLOSED, //!< Stream is closed.
	  MIMIIO_STREAM_BOTH,   //!< Stream is fully-duplexed (both sending and receiving streams are active)
	  MIMIIO_STREAM_SEND,   //!< Only sending stream is active.
	  MIMIIO_STREAM_RECV,   //!< Only receiving stream is active.
  } MIMIIO_STREAM_STATE;

  /**
   * @brief audio format
   */
  typedef enum{
	  MIMIIO_RAW_PCM, //!< Raw PCM format
	  MIMIIO_FLAC_0,  //!< Flac compression level is 0 (fastest, least compression)
	  MIMIIO_FLAC_1,  //!< Flac compression level is 1
	  MIMIIO_FLAC_2,  //!< Flac compression level is 2
	  MIMIIO_FLAC_3,  //!< Flac compression level is 3
	  MIMIIO_FLAC_4,  //!< Flac compression level is 4
	  MIMIIO_FLAC_5,  //!< Flac compression level is 5 (default, preferred)
	  MIMIIO_FLAC_6,  //!< Flac compression level is 6
	  MIMIIO_FLAC_7,  //!< Flac compression level is 7
	  MIMIIO_FLAC_8,  //!< Flac compression level is 8 (slowest, most compression)
	  MIMIIO_FLAC_PASS_THROUGH //!< Input is externally encoded in flac. libmimiio do nothing with input.
  } MIMIIO_AUDIO_FORMAT;

  /**
   * @brief log level enumeration
   */
  enum{
	  MIMIIO_LOG_ERROR   = 3, //!< error level, libmimiio can not continue processing any more.
	  MIMIIO_LOG_WARNING = 4, //!< warning level, libmimiio can continue processing but require special care.
	  MIMIIO_LOG_INFO    = 6, //!< info level, normal information.
	  MIMIIO_LOG_DEBUG   = 7,  //!< debug information.
	  MIMIIO_LOG_TRACE   = 9  //!< debug information.
  };

  /**
   * @brief Initialize and open mimi(R) connection
   *
   * If \e access_token is NULL, libmimiio try opening connection without authentication.
   *
   * Both \e on_tx_func and \e on_rx_func can be set NULL for libmimiio blocking API. One can not use blocking API and callback API simultaneously.
   *
   * @param [in] mimi_host mimi(R) remote hostname
   * @param [in] mimi_port mimi(R) remote host port
   * @param [in] on_tx_callback user defined callback function for sending audio, which is called periodically by libmimiio. NULL can be set for blocking API.
   * @param [in] on_rx_callback user defined callback function for receiving results from remote host, which is called periodically by libmimiio. NULL can be set for blocking API.
   * @param [in] userdata_for_tx user defined data for on_tx_callback
   * @param [in] userdata_for_rx user defined data for on_rx_callback
   * @param [in] format Audio format defined in enum ::MIMIIO_AUDIO_FORMAT
   * @param [in] samplingrate Audio samplingrate
   * @param [in] channels Audio channels
   * @param [in] extra_request_headers user defined request headers which is send with WebSocket upgrade request.
   * @param [in] extra_request_headess_len The number of request_headers.
   * @param [in] access_token if NULL is set, try opening connection without authentication.
   * @param [in] loglevel log level
   * @param [out] errorno errorno is set when something goes wrong and return NULL, otherwise 0 returns.
   * @return mimi connection handler, or return NULL if something is wrong with opening new connection.
   */
  MIMI_IO* mimi_open(
		  const char* mimi_host,
		  int mimi_port,
		  void (*on_tx_callback)(char* buffer, size_t* len, bool* recog_break, int* txfunc_error, void* userdata_for_tx),
		  void (*on_rx_callback)(const char* result, size_t len, int* rxfunc_error, void* userdata_for_rx),
		  void* userdata_for_tx,
		  void* userdata_for_rx,
		  MIMIIO_AUDIO_FORMAT format,
		  int samplingrate,
		  int channels,
		  const MIMIIO_HTTP_REQUEST_HEADER* extra_request_headers,
		  int custom_request_headers_len,
		  const char* access_token,
		  int loglevel,
		  int* errorno);

  /**
   * @brief Start loop of sending sound and receiving result.
   *
   * @param [in] mio mimi connection handler
   * @return error code
   */
  int mimi_start(MIMI_IO* mio);

  /**
   * @brief Determine whether the mimi connection is active.
   *
   * A mimi connection is active after a successful call to mimi_start(), until it becomes inactive either
   * the remote host close connection as a result of receiving <em> break command </em>.
   *
   * @param [in] mio mimi connection handler
   * @return Returns true when the connection is active (ie remote host can accept audio)
   */
  bool mimi_is_active(MIMI_IO* mio);

  /**
   * @brief Get state of the internal stream
   *
   * A mimiio connection has 5 state, which is called stream state. See MIMIIO_STREAM_STATE enumeration.
   *
   * @param [in] mio mimi connection handler
   * @return Returns current internal stream state
   * @see MIMIIO_STREAM_STATE
   */
  MIMIIO_STREAM_STATE mimi_stream_state(MIMI_IO* mio);

  /**
   * @brief Close mimi(R) connection. Release all resources related to libmimiio.
   *
   * Close and release a mimi connection regardless whether the connection is active or not.
   *
   * @param [in] mio mimi connection handler
   */
  void mimi_close(MIMI_IO* mio);

  /**
   * \brief Get error code in libmimiio
   *
   * @param [in] mimi connection handler.
   * @return 0 if no error, otherwise return non-zero number .
   */
  int mimi_error(MIMI_IO *mio);

  /**
   * @brief Get error string corresponding to errorno.
   *
   * @param [in] errorno error number
   * @return error string
   */
  const char* mimi_strerror(int errorno);

  /**
   * @brief Get libmimiio version
   *
   * @return libmimiio version
   */
  const char* mimi_version();

  /**
   * @brief Send audio data to mimi(R) remote host.
   * @note Hidden API without support.
   * @attention Synchronous API. Asynchronous callback API should be used for maximum performance and stability.
   *
   * @param [in] mimi connection handler.
   * @param [in] buffer audio data
   * @param [in] len length of \e buffer
   * @return size of sent data
   */
  //int mimi_send(MIMI_IO* mio, char* buffer, size_t len);

  /**
   * @brief Send break command to mimi(R) remote host.
   * @note Hidden API without support.
   * @attention Synchronous API. Asynchronous callback API should be used for maximum performance and stability.
   *
   * One MUST send break command immediately after the audio stream is considerably paused otherwise the recognition result would be not fixed. See <em> mimi(R) WebSocket API service </em> in detail about break command.
   *
   * @param [in] mimi connection handler.
   */
  //void mimi_break(MIMI_IO* mio);

  /**
   * @brief Receive result from mimi(R) remote host.
   * @note Hidden API without support
   * @attention Synchronous API.  Asynchronous callback API should be used for maximum performance and stability.
   *
   * Note that one should check error state with using mimi_error() after each call of this function.
   *
   * @param [in] mimi connection handler.
   * @param [in] result the buffer for received result
   * @param [in] len length of \e result
   * @param [in] blocking Set the mimi_recv() in blocking mode if \e blocking is true, disable blocking mode if \e blocking is false.
   * @return actual size of received data.
   */
  //int mimi_recv(MIMI_IO* mio, char* result, size_t len, bool blocking);

#ifdef __cplusplus
}
#endif

#endif
