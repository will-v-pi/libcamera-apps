/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 *
 * encoder.cpp - Video encoder class.
 */

#include <cstring>

#include "encoder.hpp"
#include "h264_encoder.hpp"
#include "mjpeg_encoder.hpp"
#include "null_encoder.hpp"

#if LIBAV_PRESENT
#include "libav_encoder.hpp"
#endif

Encoder::Encoder(VideoOptions const *options) : options_(options), buf_metadata_(std::cout.rdbuf()), of_metadata_()
{
	if (!options->metadata.empty())
	{
		const std::string &filename = options_->metadata;

		if (filename.compare("-"))
		{
			of_metadata_.open(filename, std::ios::out);
			buf_metadata_ = of_metadata_.rdbuf();
			start_metadata_output(buf_metadata_, options_->metadata_format);
		}
	}
}

Encoder::~Encoder()
{
	if (!options_->metadata.empty())
		stop_metadata_output(buf_metadata_, options_->metadata_format);
}

Encoder *Encoder::Create(VideoOptions const *options, const StreamInfo &info)
{
	if (strcasecmp(options->codec.c_str(), "yuv420") == 0)
		return new NullEncoder(options);
	else if (strcasecmp(options->codec.c_str(), "h264") == 0)
		return new H264Encoder(options, info);
#if LIBAV_PRESENT
	else if (strcasecmp(options->codec.c_str(), "libav") == 0)
		return new LibAvEncoder(options, info);
#endif
	else if (strcasecmp(options->codec.c_str(), "mjpeg") == 0)
		return new MjpegEncoder(options);
	throw std::runtime_error("Unrecognised codec " + options->codec);
}

void Encoder::metadataReady(const libcamera::ControlList &metadata)
{
	if (options_->metadata.empty())
		return;

	write_metadata(buf_metadata_, options_->metadata_format, metadata, !metadata_started_);
	metadata_started_ = true;
}

void start_metadata_output(std::streambuf *buf, std::string fmt)
{
	std::ostream out(buf);
	if (fmt == "json")
		out << "[" << std::endl;
}

void write_metadata(std::streambuf *buf, std::string fmt, const libcamera::ControlList &metadata, bool first_write)
{
	std::ostream out(buf);
	const libcamera::ControlIdMap *id_map = metadata.idMap();
	if (fmt == "txt")
	{
		for (auto const &[id, val] : metadata)
			out << id_map->at(id)->name() << "=" << val.toString() << std::endl;
		out << std::endl;
	}
	else
	{
		if (!first_write)
			out << "," << std::endl;
		out << "{";
		bool first_done = false;
		for (auto const &[id, val] : metadata)
		{
			std::string arg_quote = (val.toString().find('/') != std::string::npos) ? "\"" : "";
			out << (first_done ? "," : "") << std::endl
				<< "    \"" << id_map->at(id)->name() << "\": " << arg_quote << val.toString() << arg_quote;
			first_done = true;
		}
		out << std::endl << "}";
	}
}

void stop_metadata_output(std::streambuf *buf, std::string fmt)
{
	std::ostream out(buf);
	if (fmt == "json")
		out << std::endl << "]" << std::endl;
}
