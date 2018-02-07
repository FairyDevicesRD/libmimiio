/**
 * @file pcm.hpp
 * @brief Raw PCM noop encoder
 * @author Copyright (C) 2015 Fairy Devices Inc. http://www.fairydevices.jp/
 * @author Masato Fujino
 */

#ifndef MIMIIO_PCM_HPP_
#define MIMIIO_PCM_HPP_

#include "encoder/encoder.hpp"

namespace mimiio{ namespace encoder{

/**
 * @class PCMEncoder
 * @brief Raw PCM encoder (pass through), noop.
 */
class PCMEncoder : public Encoder
{
public:

	/**
	 * @brief C'tor
	 *
	 * @param [in] samplingrate Samplingrate of audio
	 * @param [in] channels Channels of audio
	 * @param [in] compressioLevel Compression level of the audio format
	 */
	PCMEncoder(int samplingrate, int channels, int compressionLevel, Poco::Logger& logger) :
		Encoder(samplingrate, channels, compressionLevel, logger)
	{}

	/**
	 * @brief Get Content-Type string
	 *
	 * @return Returns Content-Type string
	 */
	virtual std::string ContentType() { return Poco::format("audio/x-pcm;bit=16;rate=%d;channels=%d", samplingrate_, channels_); }

	/**
	 * @brief Encode to the format
	 *
	 * @param [in] input Raw PCM audio specified params in C'tor.
	 */
	virtual void Encode(const std::vector<char>& input)
	{
		for(size_t i=0;i<input.size();++i){
			encodedData_.push_back(input[i]);
		}
	}

	/**
	 * @brief Declare input finish and flush all internal buffer
	 */
	virtual void Flush(){}

};

}}

#endif /* PCM_HPP_ */
