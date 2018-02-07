/**
 * @file flacPT.hpp
 * @brief Flac pass through noop encoder
 * @author Copyright (C) 2015 Fairy Devices Inc. http://www.fairydevices.jp/
 * @author Masato Fujino
 */

#ifndef MIMIIO_FLACPT_HPP_
#define MIMIIO_FLACPT_HPP_

#include "encoder/encoder.hpp"

namespace mimiio{ namespace encoder{

/**
 * @class FLACPTEncoder
 * @brief FLAC noop encoder (pass through)
 */
class FlacPTEncoder : public Encoder
{
public:

	/**
	 * @brief C'tor
	 *
	 * @param [in] samplingrate Samplingrate of audio
	 * @param [in] channels Channels of audio
	 * @param [in] compressioLevel Compression level of the audio format
	 */
	FlacPTEncoder(int samplingrate, int channels, int compressionLevel, Poco::Logger& logger) :
		Encoder(samplingrate, channels, compressionLevel, logger)
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
