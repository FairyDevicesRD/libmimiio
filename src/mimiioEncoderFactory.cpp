/**
 * @file mimiioEncoderFactory.cpp
 * @brief Encoder factory class implementation
 * @author Copyright (C) 2015 Fairy Devices Inc. http://www.fairydevices.jp/
 * @author Masato Fujino
 */

#include "mimiioEncoderFactory.hpp"
#include "encoder/flac.hpp"
#include "encoder/pcm.hpp"
#include "encoder/flacPT.hpp"

namespace mimiio{
using namespace encoder;

Encoder* mimiioEncoderFactory::createEncoder(MIMIIO_AUDIO_FORMAT format, int samplingrate, int channels)
{
	switch(format){
	case MIMIIO_RAW_PCM:
		poco_debug(logger_, "lmio: create raw_pcm noop encoder.");
		return new PCMEncoder(samplingrate, channels, 0, logger_);
	case MIMIIO_FLAC_0:
		poco_debug(logger_, "lmio: create flac encoder compression level = 0.");
		return new FlacEncoder(samplingrate, channels, 0, logger_);
	case MIMIIO_FLAC_1:
		poco_debug(logger_, "lmio: create flac encoder compression level = 1.");
		return new FlacEncoder(samplingrate, channels, 1, logger_);
	case MIMIIO_FLAC_2:
		poco_debug(logger_, "lmio: create flac encoder compression level = 2.");
		return new FlacEncoder(samplingrate, channels, 2, logger_);
	case MIMIIO_FLAC_3:
		poco_debug(logger_, "lmio: create flac encoder compression level = 3.");
		return new FlacEncoder(samplingrate, channels, 3, logger_);
	case MIMIIO_FLAC_4:
		poco_debug(logger_, "lmio: create flac encoder compression level = 4.");
		return new FlacEncoder(samplingrate, channels, 4, logger_);
	case MIMIIO_FLAC_5:
		poco_debug(logger_, "lmio: create flac encoder compression level = 5.");
		return new FlacEncoder(samplingrate, channels ,5, logger_);
	case MIMIIO_FLAC_6:
		poco_debug(logger_, "lmio: create flac encoder compression level = 6.");
		return new FlacEncoder(samplingrate, channels, 6, logger_);
	case MIMIIO_FLAC_7:
		poco_debug(logger_, "lmio: create flac encoder compression level = 7.");
		return new FlacEncoder(samplingrate, channels, 7, logger_);
	case MIMIIO_FLAC_8:
		poco_debug(logger_, "lmio: create flac encoder compression level = 8.");
		return new FlacEncoder(samplingrate, channels, 8, logger_);
	case MIMIIO_FLAC_PASS_THROUGH:
		poco_debug(logger_, "lmio: create flac noop pass through encoder.");
		return new FlacPTEncoder(samplingrate, channels, 0, logger_);

	default:
		poco_debug(logger_, "lmio: invalid format"); // not reached here.
		return nullptr;
	}
}

}

