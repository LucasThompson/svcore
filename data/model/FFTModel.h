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

#ifndef _FFT_MODEL_H_
#define _FFT_MODEL_H_

#include "data/fft/FFTDataServer.h"
#include "DenseThreeDimensionalModel.h"

class FFTModel : public DenseThreeDimensionalModel
{
public:
    FFTModel(const DenseTimeValueModel *model,
             int channel,
             WindowType windowType,
             size_t windowSize,
             size_t windowIncrement,
             size_t fftSize,
             bool polar,
             size_t fillFromColumn = 0);
    ~FFTModel();

    size_t getWidth() const {
        return m_server->getWidth() >> m_xshift;
    }
    size_t getHeight() const {
        return m_server->getHeight() >> m_yshift;
    }
    float getMagnitudeAt(size_t x, size_t y) {
        return m_server->getMagnitudeAt(x << m_xshift, y << m_yshift);
    }
    float getNormalizedMagnitudeAt(size_t x, size_t y) {
        return m_server->getNormalizedMagnitudeAt(x << m_xshift, y << m_yshift);
    }
    float getMaximumMagnitudeAt(size_t x) {
        return m_server->getMaximumMagnitudeAt(x << m_xshift);
    }
    float getPhaseAt(size_t x, size_t y) {
        return m_server->getPhaseAt(x << m_xshift, y << m_yshift);
    }
    void getValuesAt(size_t x, size_t y, float &real, float &imaginary) {
        m_server->getValuesAt(x << m_xshift, y << m_yshift, real, imaginary);
    }
    bool isColumnReady(size_t x) {
        return m_server->isColumnReady(x << m_xshift);
    }
    bool isLocalPeak(size_t x, size_t y) {
        float mag = getMagnitudeAt(x, y);
        if (y > 0 && mag < getMagnitudeAt(x, y - 1)) return false;
        if (y < getHeight() - 1 && mag < getMagnitudeAt(x, y + 1)) return false;
        return true;
    }
    bool isOverThreshold(size_t x, size_t y, float threshold) {
        return getMagnitudeAt(x, y) > threshold;
    }

    size_t getFillExtent() const { return m_server->getFillExtent(); }

    // DenseThreeDimensionalModel and Model methods:
    //
    virtual bool isOK() const {
        return m_server && m_server->getModel();
    }
    virtual size_t getStartFrame() const {
        return 0;
    }
    virtual size_t getEndFrame() const {
        return getWidth() * getResolution() + getResolution();
    }
    virtual size_t getSampleRate() const;
    virtual size_t getResolution() const {
        return m_server->getWindowIncrement() << m_xshift;
    }
    virtual size_t getYBinCount() const {
        return getHeight();
    }
    virtual float getMinimumLevel() const {
        return 0.f; // Can't provide
    }
    virtual float getMaximumLevel() const {
        return 1.f; // Can't provide
    }
    virtual void getBinValues(long windowStartFrame, BinValueSet &result) const;
    virtual float getBinValue(long windowStartFrame, size_t n) const;
    virtual QString getBinName(size_t n) const;

    virtual int getCompletion() const { return m_server->getFillCompletion(); }

    virtual Model *clone() const;

    virtual void suspend() { m_server->suspend(); }
    virtual void suspendWrites() { m_server->suspendWrites(); }
    virtual void resume() { m_server->resume(); }

private:
    FFTModel(const FFTModel &);
    FFTModel &operator=(const FFTModel &); // not implemented

    FFTDataServer *m_server;
    int m_xshift;
    int m_yshift;
};

#endif