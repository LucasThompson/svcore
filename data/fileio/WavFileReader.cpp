/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2006 Chris Cannam.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "WavFileReader.h"

#include <iostream>

WavFileReader::WavFileReader(QString path) :
    m_file(0),
    m_path(path),
    m_buffer(0),
    m_bufsiz(0),
    m_lastStart(0),
    m_lastCount(0)
{
    m_frameCount = 0;
    m_channelCount = 0;
    m_sampleRate = 0;

    m_fileInfo.format = 0;
    m_fileInfo.frames = 0;
    m_file = sf_open(m_path.toLocal8Bit(), SFM_READ, &m_fileInfo);

    if (!m_file || m_fileInfo.frames <= 0 || m_fileInfo.channels <= 0) {
	std::cerr << "WavFileReader::initialize: Failed to open file ("
		  << sf_strerror(m_file) << ")" << std::endl;

	if (m_file) {
	    m_error = QString("Couldn't load audio file '%1':\n%2")
		.arg(m_path).arg(sf_strerror(m_file));
	} else {
	    m_error = QString("Failed to open audio file '%1'")
		.arg(m_path);
	}
	return;
    }

    m_frameCount = m_fileInfo.frames;
    m_channelCount = m_fileInfo.channels;
    m_sampleRate = m_fileInfo.samplerate;
}

WavFileReader::~WavFileReader()
{
    if (m_file) sf_close(m_file);
}

void
WavFileReader::getInterleavedFrames(size_t start, size_t count,
				    SampleBlock &results) const
{
    results.clear();
    if (!m_file || !m_channelCount) return;
    if (count == 0) return;

    if ((long)start >= m_fileInfo.frames) {
	return;
    }

    if (long(start + count) > m_fileInfo.frames) {
	count = m_fileInfo.frames - start;
    }

    sf_count_t readCount = 0;

    m_mutex.lock();

    if (start != m_lastStart || count != m_lastCount) {

	if (sf_seek(m_file, start, SEEK_SET) < 0) {
	    m_mutex.unlock();
	    return;
	}
	
	if (count * m_fileInfo.channels > m_bufsiz) {
//	    std::cerr << "WavFileReader: Reallocating buffer for " << count
//		      << " frames, " << m_fileInfo.channels << " channels: "
//		      << m_bufsiz << " floats" << std::endl;
	    m_bufsiz = count * m_fileInfo.channels;
	    delete[] m_buffer;
	    m_buffer = new float[m_bufsiz];
	}
	
	if ((readCount = sf_readf_float(m_file, m_buffer, count)) < 0) {
	    m_mutex.unlock();
	    return;
	}

	m_lastStart = start;
	m_lastCount = readCount;
    }

    for (size_t i = 0; i < count * m_fileInfo.channels; ++i) {
	results.push_back(m_buffer[i]);
    }

    m_mutex.unlock();
    return;
}
