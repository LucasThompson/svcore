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

#ifndef _AUDIO_FILE_READER_H_
#define _AUDIO_FILE_READER_H_

#include <QString>

#include "base/BaseTypes.h"
#include "FileSource.h"

#include <vector>
#include <map>

class AudioFileReader : public QObject
{
    Q_OBJECT

public:
    virtual ~AudioFileReader() { }

    /**
     * Return true if the file was opened successfully and no error
     * has subsequently occurred.
     */
    bool isOK() const { return (m_channelCount > 0); }

    /**
     * If isOK() is false, return an error string.
     */
    virtual QString getError() const { return ""; }

    /**
     * Return the number of audio sample frames (i.e. samples per
     * channel) in the file.
     */
    sv_frame_t getFrameCount() const { return m_frameCount; }

    /**
     * Return the number of channels in the file.
     */
    int getChannelCount() const { return m_channelCount; }

    /**
     * Return the samplerate at which the file is being read. This is
     * the rate requested when the file was opened, which may differ
     * from the native rate of the file (in which case the file will
     * be resampled as it is read).
     */
    sv_samplerate_t getSampleRate() const { return m_sampleRate; }

    /**
     * Return the native samplerate of the file. This will differ from
     * getSampleRate() if the file is being resampled because it was
     * requested to open at a different rate from native.
     */
    virtual sv_samplerate_t getNativeRate() const { return m_sampleRate; }

    /**
     * Return the location of the audio data in the reader (as passed
     * in to the FileSource constructor, for example).
     */
    virtual QString getLocation() const { return ""; }
    
    /**
     * Return the title of the work in the audio file, if known.  This
     * may be implemented by subclasses that support file tagging.
     * This is not the same thing as the file name.
     */
    virtual QString getTitle() const { return ""; }

    /**
     * Return the "maker" of the work in the audio file, if known.
     * This could represent almost anything (band, composer,
     * conductor, artist etc).
     */
    virtual QString getMaker() const { return ""; }

    /**
     * Return the local file path of the audio data. This is the
     * location most likely to contain readable audio data: it may be
     * in a different place or format from the originally specified
     * location, for example if the file has been retrieved and
     * decoded. In some cases there may be no local file path, and
     * this will return "" if there is none.
     */
    virtual QString getLocalFilename() const { return ""; }
    
    typedef std::map<QString, QString> TagMap;
    virtual TagMap getTags() const { return TagMap(); }

    /**
     * Return true if this file supports fast seek and random
     * access. Typically this will be true for uncompressed formats
     * and false for compressed ones.
     */
    virtual bool isQuicklySeekable() const = 0;

    /** 
     * Return interleaved samples for count frames from index start.
     * The resulting vector will contain count * getChannelCount()
     * samples (or fewer if end of file is reached).
     *
     * The subclass implementations of this function must be
     * thread-safe -- that is, safe to call from multiple threads with
     * different arguments on the same object at the same time.
     */
    virtual floatvec_t getInterleavedFrames(sv_frame_t start,
                                            sv_frame_t count) const = 0;

    /**
     * Return de-interleaved samples for count frames from index
     * start.  Implemented in this class (it calls
     * getInterleavedFrames and de-interleaves).  The resulting vector
     * will contain getChannelCount() sample blocks of count samples
     * each (or fewer if end of file is reached).
     */
    virtual std::vector<floatvec_t> getDeInterleavedFrames(sv_frame_t start,
                                                           sv_frame_t count) const;

    // only subclasses that do not know exactly how long the audio
    // file is until it's been completely decoded should implement this
    virtual int getDecodeCompletion() const { return 100; } // %

    virtual bool isUpdating() const { return false; }

signals:
    void frameCountChanged();
    
protected:
    sv_frame_t m_frameCount;
    int m_channelCount;
    sv_samplerate_t m_sampleRate;
};

#endif
