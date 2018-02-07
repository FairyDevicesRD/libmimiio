/**
 * @file encoder.hpp
 * @brief Encoder base class
 * @author Copyright (C) 2015 Fairy Devices Inc. http://www.fairydevices.jp/
 * @author Masato Fujino
 */

#ifndef MIMIIO_ENCODER_HPP_
#define MIMIIO_ENCODER_HPP_

#include <vector>
#include <string>
#include <exception>
#include <stdexcept>
#include <memory>
#include <Poco/Logger.h>

namespace mimiio{ namespace encoder{

/**
 * @class EncoderInitException
 * @brief Exception while encoder initialization process
 */
class EncoderInitException : public std::runtime_error
{
public:
	explicit EncoderInitException(const std::string& s) : std::runtime_error(s){}
};

/**
 * @class EncoderProcessException
 * @brief Exception while encoder encoding process
 */
class EncoderProcessException : public std::runtime_error
{
public:
	explicit EncoderProcessException(const std::string& s) : std::runtime_error(s){}
};

/**
 * @class Encoder
 * @brief Encoder base class
 */
class Encoder
{
public:

	typedef std::unique_ptr<Encoder> Ptr;

	/**
	 * @brief C'tor
	 *
	 * @param [in] samplingrate Samplingrate of audio
	 * @param [in] channels Channels of audio
	 * @param [in] compressioLevel Compression level of the audio format
	 */
	Encoder(int samplingrate, int channels, int compressionLevel, Poco::Logger& logger) :
		samplingrate_(samplingrate),
		channels_(channels),
		compressionLevel_(compressionLevel),
		logger_(logger)
	{}

	virtual ~Encoder(){}

	/**
	 * @brief Get Content-Type string
	 *
	 * @return Returns Content-Type string
	 */
	virtual std::string ContentType() = 0;

	/**
	 * @brief Encode to the format
	 * @attention This function assumes \e input has little-endian.
	 *
	 * @param [in] input Raw PCM audio specified params in C'tor.
	 */
	virtual void Encode(const std::vector<char>& input) = 0;

	/**
	 * @brief Declare input finish and flush all internal buffer
	 */
	virtual void Flush() = 0;

	/**
	 * @brief Get Encoded data and clear, default implementation.
	 *
	 * @param [out] output Encoded data.
	 */
	virtual void GetEncodedData(std::vector<char>& output)
	{
		for(size_t i=0;i<encodedData_.size();++i){
			output.push_back(encodedData_[i]);
		}
		encodedData_.clear();
	}

protected:

	int samplingrate_;
	int channels_;
	int compressionLevel_;
	std::vector<char> encodedData_;
	Poco::Logger& logger_;
};

}}


#endif /* ENCODER_HPP_ */
