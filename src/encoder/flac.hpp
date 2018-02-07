/**
 * @file flac.hpp
 * @brief Flac encoder definition
 * @author Copyright (C) 2015 Fairy Devices Inc. http://www.fairydevices.jp/
 * @author Masato Fujino
 */

#ifndef MIMIIO_FLAC_HPP_
#define MIMIIO_FLAC_HPP_

#include "encoder/encoder.hpp"
#include <FLAC++/metadata.h>
#include <FLAC++/encoder.h>
#include <memory>
#include <vector>
#include <Poco/Mutex.h>
#include <Poco/Format.h>

namespace mimiio{ namespace encoder{

/**
 * @class FlacEncoderImpl
 * @brief flac++ encoder class
 */
class FlacEncoderImpl: public FLAC::Encoder::Stream
{
public:

	typedef std::unique_ptr<FlacEncoderImpl> Ptr;

	/**
	 * @brief C'tor
	 *
	 * @param [in] samplingrate Samplingrate of audio
	 * @param [in] channels Channels of audio
	 * @param [in] compressioLevel Compression level of the audio format
	 */
	FlacEncoderImpl(int samplingrate, int channels, int compressionLevel, Poco::Logger& logger);

	/**
	 * @brief D'tor
	 */
	virtual ~FlacEncoderImpl();

	/**
	 * @brief Finish the encoding process. Flushes the encoding buffer.
	 *
	 * Releases resources, resets the encoder settings to their defaults,
	 * and returns the encoder state to FLAC__STREAM_ENCODER_UNINITIALIZED
	 *
	 * @see libflac documentation
	 */
	void Flush() { finish(); }

	/**
	 * @brief Get Encoded data and clear.
	 *
	 * @param [out] output Encoded data.
	 */
	void GetEncodedData(std::vector<char>& encodedData);

protected:
	/**
	 * @brief flac stream encoder write callback
	 */
	virtual FLAC__StreamEncoderWriteStatus write_callback(const FLAC__byte* buffer, size_t bytes, unsigned int samples, unsigned int current_frame);

private:

	Poco::Mutex mutex_;
	std::vector<FLAC__byte> encodedData_; // FLAC__byte is usually "unsigned char"
	Poco::Logger& logger_;
};

/**
 * @class FlacEncoder
 * @brief FlacEncoder
 */
class FlacEncoder : public Encoder
{
public:

	/**
	 * @brief C'tor
	 *
	 * @param [in] samplingrate Samplingrate of audio
	 * @param [in] channels Channels of audio
	 * @param [in] compressioLevel Compression level of the audio format
	 */
	FlacEncoder(int samplingrate, int channels, int compressionLevel, Poco::Logger& logger) :
		Encoder(samplingrate, channels, compressionLevel, logger),
		impl_(new FlacEncoderImpl(samplingrate, channels, compressionLevel, logger))
	{}

	/**
	 * @brief Get Content-Type string
	 *
	 * @return Returns Content-Type string
	 */
	virtual std::string ContentType() { return Poco::format("audio/x-flac;bit=16;rate=%d;channels=%d", samplingrate_, channels_); }

	/**
	 * @brief Encode to the format
	 *
	 * @param [in] input Raw PCM audio specified params in C'tor.
	 * @param [out] output Encoded audio.
	 * @param [in] finish Flag for final input.
	 */
	virtual void Encode(const std::vector<char>& input);

	/**
	 * @brief Declare input finish and flush all internal buffer
	 */
	virtual void Flush();

	/**
	 * @brief Get Encoded data and clear
	 *
	 * @param [out] output Encoded data
	 */
	virtual void GetEncodedData(std::vector<char>& output);

private:

	FlacEncoderImpl::Ptr impl_;

};

}}

#endif /* FLAC_HPP_ */
