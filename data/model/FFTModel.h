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

#ifndef FFT_MODEL_H
#define FFT_MODEL_H

#include "DenseThreeDimensionalModel.h"
#include "DenseTimeValueModel.h"

#include "base/Window.h"

#include <bqfft/FFT.h>
#include <bqvec/Allocators.h>

#include <set>
#include <vector>
#include <complex>

/**
 * An implementation of DenseThreeDimensionalModel that makes FFT data
 * derived from a DenseTimeValueModel available as a generic data
 * grid.
 */
class FFTModel : public DenseThreeDimensionalModel
{
    Q_OBJECT

    //!!! threading requirements?
    //!!! doubles? since we're not caching much

public:
    /**
     * Construct an FFT model derived from the given
     * DenseTimeValueModel, with the given window parameters and FFT
     * size (which may exceed the window size, for zero-padded FFTs).
     * 
     * If the model has multiple channels use only the given channel,
     * unless the channel is -1 in which case merge all available
     * channels.
     */
    FFTModel(const DenseTimeValueModel *model,
             int channel,
             WindowType windowType,
             int windowSize,
             int windowIncrement,
             int fftSize);
    ~FFTModel();

    // DenseThreeDimensionalModel and Model methods:
    //
    virtual int getWidth() const;
    virtual int getHeight() const;
    virtual float getValueAt(int x, int y) const { return getMagnitudeAt(x, y); }
    virtual bool isOK() const { return m_model && m_model->isOK(); }
    virtual sv_frame_t getStartFrame() const { return 0; }
    virtual sv_frame_t getEndFrame() const {
        return sv_frame_t(getWidth()) * getResolution() + getResolution();
    }
    virtual sv_samplerate_t getSampleRate() const {
        return isOK() ? m_model->getSampleRate() : 0;
    }
    virtual int getResolution() const { return m_windowIncrement; }
    virtual int getYBinCount() const { return getHeight(); }
    virtual float getMinimumLevel() const { return 0.f; } // Can't provide
    virtual float getMaximumLevel() const { return 1.f; } // Can't provide
    virtual Column getColumn(int x) const; // magnitudes
    virtual Column getPhases(int x) const;
    virtual QString getBinName(int n) const;
    virtual bool shouldUseLogValueScale() const { return true; }
    virtual int getCompletion() const {
        int c = 100;
        if (m_model) {
            if (m_model->isReady(&c)) return 100;
        }
        return c;
    }
    virtual QString getError() const { return ""; } //!!!???
    virtual sv_frame_t getFillExtent() const { return getEndFrame(); }

    // FFTModel methods:
    //
    int getChannel() const { return m_channel; }
    WindowType getWindowType() const { return m_windowType; }
    int getWindowSize() const { return m_windowSize; }
    int getWindowIncrement() const { return m_windowIncrement; }
    int getFFTSize() const { return m_fftSize; }

//!!! review which of these are ever actually called
    
    float getMagnitudeAt(int x, int y) const;
    float getMaximumMagnitudeAt(int x) const;
    float getPhaseAt(int x, int y) const;
    void getValuesAt(int x, int y, float &real, float &imaginary) const;
    bool getMagnitudesAt(int x, float *values, int minbin = 0, int count = 0) const;
    bool getPhasesAt(int x, float *values, int minbin = 0, int count = 0) const;
    bool getValuesAt(int x, float *reals, float *imaginaries, int minbin = 0, int count = 0) const;

    /**
     * Calculate an estimated frequency for a stable signal in this
     * bin, using phase unwrapping.  This will be completely wrong if
     * the signal is not stable here.
     */
    virtual bool estimateStableFrequency(int x, int y, double &frequency);

    enum PeakPickType
    {
        AllPeaks,                /// Any bin exceeding its immediate neighbours
        MajorPeaks,              /// Peaks picked using sliding median window
        MajorPitchAdaptivePeaks  /// Bigger window for higher frequencies
    };

    typedef std::set<int> PeakLocationSet; // bin
    typedef std::map<int, double> PeakSet; // bin -> freq

    /**
     * Return locations of peak bins in the range [ymin,ymax].  If
     * ymax is zero, getHeight()-1 will be used.
     */
    virtual PeakLocationSet getPeaks(PeakPickType type, int x,
                                     int ymin = 0, int ymax = 0) const;

    /**
     * Return locations and estimated stable frequencies of peak bins.
     */
    virtual PeakSet getPeakFrequencies(PeakPickType type, int x,
                                       int ymin = 0, int ymax = 0) const;

    QString getTypeName() const { return tr("FFT"); }

public slots:
    void sourceModelAboutToBeDeleted();

private:
    FFTModel(const FFTModel &); // not implemented
    FFTModel &operator=(const FFTModel &); // not implemented

    const DenseTimeValueModel *m_model;
    int m_channel;
    WindowType m_windowType;
    int m_windowSize;
    int m_windowIncrement;
    int m_fftSize;
    Window<float> m_windower;
    mutable breakfastquay::FFT m_fft;
    
    int getPeakPickWindowSize(PeakPickType type, sv_samplerate_t sampleRate,
                              int bin, float &percentile) const;

    std::pair<sv_frame_t, sv_frame_t> getSourceSampleRange(int column) const {
        sv_frame_t startFrame = m_windowIncrement * sv_frame_t(column);
        sv_frame_t endFrame = startFrame + m_windowSize;
        // Cols are centred on the audio sample (e.g. col 0 is centred at sample 0)
        startFrame -= m_windowSize / 2;
        endFrame -= m_windowSize / 2;
        return { startFrame, endFrame };
    }

    typedef std::vector<float, breakfastquay::StlAllocator<float>> fvec;
    typedef std::vector<std::complex<float>,
                        breakfastquay::StlAllocator<std::complex<float>>> cvec;
    
    const cvec &getFFTColumn(int column) const; // returns ref for immediate use only
    fvec getSourceSamples(int column) const;
    fvec getSourceData(std::pair<sv_frame_t, sv_frame_t>) const;
    fvec getSourceDataUncached(std::pair<sv_frame_t, sv_frame_t>) const;

    struct SavedSourceData {
        std::pair<sv_frame_t, sv_frame_t> range;
        fvec data;
    };
    mutable SavedSourceData m_savedData;

    struct SavedColumn {
        int n;
        cvec col;
    };
    mutable std::vector<SavedColumn> m_cached;
    mutable size_t m_cacheWriteIndex;
    size_t m_cacheSize;
};

#endif
