/**
 * @file flac.cpp
 * @brief flac encoder implementation
 * @author Copyright (C) 2015 Fairy Devices Inc. http://www.fairydevices.jp/
 * @author Masato Fujino
 */

#include "encoder/flac.hpp"
#include <Poco/Format.h>
#include <Poco/Mutex.h>
#include <Poco/ScopedLock.h>

namespace mimiio{ namespace encoder{

FlacEncoderImpl::FlacEncoderImpl(
		int samplingrate,
		int channels,
		int compressionLevel,
		Poco::Logger& logger) :
		FLAC::Encoder::Stream(),
		logger_(logger)
{
	set_verify(false); // Do not verify encoded data. The verification process cause performance to be double slow.
	set_compression_level(compressionLevel); // Compression Level see mimiio.h enum ::MIMIIO_AUDIO_FORMAT
	set_channels(channels);
	set_bits_per_sample(16); // Fixed to 16bit depth
	set_sample_rate(samplingrate);
	logger_.debug("lmio: FlacEncoderImpl: compressionLevel=%d, channels=%d, samplerate=%d", compressionLevel, channels, samplingrate);
	FLAC__StreamEncoderInitStatus init_status = init();
	if(init_status != FLAC__STREAM_ENCODER_INIT_STATUS_OK){
		std::string errstr = FLAC__StreamEncoderInitStatusString[init_status];
		throw EncoderInitException(Poco::format("%s (%d)", errstr, static_cast<int>(init_status)));
	}
}

FlacEncoderImpl::~FlacEncoderImpl()
{
	finish();
}

FLAC__StreamEncoderWriteStatus FlacEncoderImpl::write_callback(
		const FLAC__byte* buffer,
		size_t bytes,
		unsigned int samples,
		unsigned int current_frame)
{
	Poco::Mutex::ScopedLock lock(mutex_);
	for(size_t i=0;i<bytes;++i){
		encodedData_.push_back(buffer[i]);
	}
	return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
}

void FlacEncoderImpl::GetEncodedData(std::vector<char>& encodedData)
{
	Poco::Mutex::ScopedLock lock(mutex_);
	for(size_t i=0;i<encodedData_.size();++i){
		encodedData.push_back(static_cast<char>(encodedData_[i]));
	}
	encodedData_.clear();
}

void FlacEncoder::Encode(const std::vector<char>& input)
{
	if(input.size() % (impl_->get_bits_per_sample() / 8) != 0){ //1byte == 8bit
		throw EncoderProcessException("The length of encoder input is not multiple of bits per sample.");
	}
	logger_.debug("lmio: FlacEncoder: Encode input size = %d bytes", static_cast<int>(input.size()));
	size_t pcm_samples = input.size() / (impl_->get_bits_per_sample() / 8); //1byte == 8bit
	std::unique_ptr<FLAC__int32[]> pcm(new FLAC__int32[pcm_samples]);
	for(size_t i=0;i<pcm_samples;++i){
		pcm[i] = (FLAC__int32)(((FLAC__int16)(FLAC__int8)static_cast<unsigned char>(input[2*i+1]) << 8) | (FLAC__int16)static_cast<unsigned char>(input[2*i]));
	}

	impl_->process_interleaved(pcm.get(), pcm_samples /  impl_->get_channels());
}

void FlacEncoder::Flush()
{
	impl_->finish();
}

void FlacEncoder::GetEncodedData(std::vector<char>& output)
{
	impl_->GetEncodedData(output);
}

}}



