/**
 * @file mimiioEncoderFactory.hpp
 * @brief Encoder factory class definition
 * @author Copyright (C) 2015 Fairy Devices Inc. http://www.fairydevices.jp/
 * @author Masato Fujino
 */

#ifndef MIMIIOENCODERFACTORY_HPP_
#define MIMIIOENCODERFACTORY_HPP_

#include "mimiio.h"
#include "encoder/encoder.hpp"
#include <Poco/Logger.h>
#include <memory>

namespace mimiio{

/**
 * @class mimiioEncoderFactory
 * @brief Factory class for audio encoder
 */
class mimiioEncoderFactory
{
public:

	typedef std::unique_ptr<mimiioEncoderFactory> Ptr;

	/**
	 * @brief C'tor
	 *
	 * @param [in] logger Logger
	 */
	explicit mimiioEncoderFactory(Poco::Logger& logger) : logger_(logger) {}

	/**
	 * @brief Create audio encoder
	 *
	 * @param [in] samplingrate Samplingrate of audio
	 * @param [in] channels Channels of audio
	 * @param [in] compressioLevel Compression level of the audio format
	 *
	 * @return Encoder() or NULL when format is ::MIMIIO_RAW_PCM or undefined.
	 */
	encoder::Encoder* createEncoder(MIMIIO_AUDIO_FORMAT format, int samplingrate, int channels);

private:
	Poco::Logger& logger_;
};

}

#endif /* MIMIIOENCODERFACTORY_HPP_ */
