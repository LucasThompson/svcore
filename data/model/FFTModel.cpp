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

#include "FFTModel.h"
#include "DenseTimeValueModel.h"

#include "base/Profiler.h"
#include "base/Pitch.h"
#include "base/HitCount.h"

#include <algorithm>

#include <cassert>
#include <deque>

using namespace std;

static HitCount inSmallCache("FFTModel: Small FFT cache");
static HitCount inSourceCache("FFTModel: Source data cache");

FFTModel::FFTModel(const DenseTimeValueModel *model,
                   int channel,
                   WindowType windowType,
                   int windowSize,
                   int windowIncrement,
                   int fftSize) :
    m_model(model),
    m_channel(channel),
    m_windowType(windowType),
    m_windowSize(windowSize),
    m_windowIncrement(windowIncrement),
    m_fftSize(fftSize),
    m_windower(windowType, windowSize),
    m_fft(fftSize),
    m_cacheWriteIndex(0),
    m_cacheSize(3)
{
    while (m_cached.size() < m_cacheSize) {
        m_cached.push_back({ -1, cvec(m_fftSize / 2 + 1) });
    }
    
    if (m_windowSize > m_fftSize) {
        cerr << "ERROR: FFTModel::FFTModel: window size (" << m_windowSize
             << ") must be at least FFT size (" << m_fftSize << ")" << endl;
        throw invalid_argument("FFTModel window size must be at least FFT size");
    }

    m_fft.initFloat();

    connect(model, SIGNAL(modelChanged()), this, SIGNAL(modelChanged()));
    connect(model, SIGNAL(modelChangedWithin(sv_frame_t, sv_frame_t)),
            this, SIGNAL(modelChangedWithin(sv_frame_t, sv_frame_t)));
}

FFTModel::~FFTModel()
{
}

void
FFTModel::sourceModelAboutToBeDeleted()
{
    if (m_model) {
        cerr << "FFTModel[" << this << "]::sourceModelAboutToBeDeleted(" << m_model << ")" << endl;
        m_model = 0;
    }
}

int
FFTModel::getWidth() const
{
    if (!m_model) return 0;
    return int((m_model->getEndFrame() - m_model->getStartFrame())
               / m_windowIncrement) + 1;
}

int
FFTModel::getHeight() const
{
    return m_fftSize / 2 + 1;
}

QString
FFTModel::getBinName(int n) const
{
    sv_samplerate_t sr = getSampleRate();
    if (!sr) return "";
    QString name = tr("%1 Hz").arg((n * sr) / ((getHeight()-1) * 2));
    return name;
}

FFTModel::Column
FFTModel::getColumn(int x) const
{
    auto cplx = getFFTColumn(x);
    Column col;
    col.reserve(cplx.size());
    for (auto c: cplx) col.push_back(abs(c));
    return col;
}

FFTModel::Column
FFTModel::getPhases(int x) const
{
    auto cplx = getFFTColumn(x);
    Column col;
    col.reserve(cplx.size());
    for (auto c: cplx) {
        col.push_back(arg(c));
    }
    return col;
}

float
FFTModel::getMagnitudeAt(int x, int y) const
{
    if (x < 0 || x >= getWidth() || y < 0 || y >= getHeight()) return 0.f;
    auto col = getFFTColumn(x);
    return abs(col[y]);
}

float
FFTModel::getMaximumMagnitudeAt(int x) const
{
    Column col(getColumn(x));
    float max = 0.f;
    int n = int(col.size());
    for (int i = 0; i < n; ++i) {
        if (col[i] > max) max = col[i];
    }
    return max;
}

float
FFTModel::getPhaseAt(int x, int y) const
{
    if (x < 0 || x >= getWidth() || y < 0 || y >= getHeight()) return 0.f;
    return arg(getFFTColumn(x)[y]);
}

void
FFTModel::getValuesAt(int x, int y, float &re, float &im) const
{
    auto col = getFFTColumn(x);
    re = col[y].real();
    im = col[y].imag();
}

bool
FFTModel::getMagnitudesAt(int x, float *values, int minbin, int count) const
{
    if (count == 0) count = getHeight();
    auto col = getFFTColumn(x);
    for (int i = 0; i < count; ++i) {
        values[i] = abs(col[minbin + i]);
    }
    return true;
}

bool
FFTModel::getPhasesAt(int x, float *values, int minbin, int count) const
{
    if (count == 0) count = getHeight();
    auto col = getFFTColumn(x);
    for (int i = 0; i < count; ++i) {
        values[i] = arg(col[minbin + i]);
    }
    return true;
}

bool
FFTModel::getValuesAt(int x, float *reals, float *imags, int minbin, int count) const
{
    if (count == 0) count = getHeight();
    auto col = getFFTColumn(x);
    for (int i = 0; i < count; ++i) {
        reals[i] = col[minbin + i].real();
    }
    for (int i = 0; i < count; ++i) {
        imags[i] = col[minbin + i].imag();
    }
    return true;
}

FFTModel::fvec
FFTModel::getSourceSamples(int column) const
{
    // m_fftSize may be greater than m_windowSize, but not the reverse

//    cerr << "getSourceSamples(" << column << ")" << endl;
    
    auto range = getSourceSampleRange(column);
    auto data = getSourceData(range);

    int off = (m_fftSize - m_windowSize) / 2;

    if (off == 0) {
        return data;
    } else {
        vector<float> pad(off, 0.f);
        fvec padded;
        padded.reserve(m_fftSize);
        padded.insert(padded.end(), pad.begin(), pad.end());
        padded.insert(padded.end(), data.begin(), data.end());
        padded.insert(padded.end(), pad.begin(), pad.end());
        return padded;
    }
}

FFTModel::fvec
FFTModel::getSourceData(pair<sv_frame_t, sv_frame_t> range) const
{
//    cerr << "getSourceData(" << range.first << "," << range.second
//         << "): saved range is (" << m_savedData.range.first
//         << "," << m_savedData.range.second << ")" << endl;

    if (m_savedData.range == range) {
        inSourceCache.hit();
        return m_savedData.data;
    }

    Profiler profiler("FFTModel::getSourceData (cache miss)");
    
    if (range.first < m_savedData.range.second &&
        range.first >= m_savedData.range.first &&
        range.second > m_savedData.range.second) {

        inSourceCache.partial();
        
        sv_frame_t discard = range.first - m_savedData.range.first;

        fvec acc(m_savedData.data.begin() + discard, m_savedData.data.end());

        fvec rest = getSourceDataUncached({ m_savedData.range.second, range.second });

        acc.insert(acc.end(), rest.begin(), rest.end());
        
        m_savedData = { range, acc };
        return acc;

    } else {

        inSourceCache.miss();
        
        auto data = getSourceDataUncached(range);
        m_savedData = { range, data };
        return data;
    }
}

FFTModel::fvec
FFTModel::getSourceDataUncached(pair<sv_frame_t, sv_frame_t> range) const
{
    decltype(range.first) pfx = 0;
    if (range.first < 0) {
        pfx = -range.first;
        range = { 0, range.second };
    }

    auto data = m_model->getData(m_channel,
                                 range.first,
                                 range.second - range.first);

    if (data.empty()) {
        SVDEBUG << "NOTE: empty source data for range (" << range.first << ","
                << range.second << ") (model end frame "
                << m_model->getEndFrame() << ")" << endl;
    }
    
    // don't return a partial frame
    data.resize(range.second - range.first, 0.f);

    if (pfx > 0) {
        vector<float> pad(pfx, 0.f);
        data.insert(data.begin(), pad.begin(), pad.end());
    }
    
    if (m_channel == -1) {
	int channels = m_model->getChannelCount();
	if (channels > 1) {
            int n = int(data.size());
            float factor = 1.f / float(channels);
            // use mean instead of sum for fft model input
	    for (int i = 0; i < n; ++i) {
		data[i] *= factor;
	    }
	}
    }
    
    return data;
}

const FFTModel::cvec &
FFTModel::getFFTColumn(int n) const
{
    // The small cache (i.e. the m_cached deque) is for cases where
    // values are looked up individually, and for e.g. peak-frequency
    // spectrograms where values from two consecutive columns are
    // needed at once. This cache gets essentially no hits when
    // scrolling through a magnitude spectrogram, but 95%+ hits with a
    // peak-frequency spectrogram.
    for (const auto &incache : m_cached) {
        if (incache.n == n) {
            inSmallCache.hit();
            return incache.col;
        }
    }
    inSmallCache.miss();

    Profiler profiler("FFTModel::getFFTColumn (cache miss)");
    
    auto samples = getSourceSamples(n);
    m_windower.cut(samples.data());
    breakfastquay::v_fftshift(samples.data(), m_fftSize);

    cvec &col = m_cached[m_cacheWriteIndex].col;
    
    m_fft.forwardInterleaved(samples.data(),
                             reinterpret_cast<float *>(col.data()));

    m_cached[m_cacheWriteIndex].n = n;

    m_cacheWriteIndex = (m_cacheWriteIndex + 1) % m_cacheSize;

    return col;
}

bool
FFTModel::estimateStableFrequency(int x, int y, double &frequency)
{
    if (!isOK()) return false;

    frequency = double(y * getSampleRate()) / m_fftSize;

    if (x+1 >= getWidth()) return false;

    // At frequency f, a phase shift of 2pi (one cycle) happens in 1/f sec.
    // At hopsize h and sample rate sr, one hop happens in h/sr sec.
    // At window size w, for bin b, f is b*sr/w.
    // thus 2pi phase shift happens in w/(b*sr) sec.
    // We need to know what phase shift we expect from h/sr sec.
    // -> 2pi * ((h/sr) / (w/(b*sr)))
    //  = 2pi * ((h * b * sr) / (w * sr))
    //  = 2pi * (h * b) / w.

    double oldPhase = getPhaseAt(x, y);
    double newPhase = getPhaseAt(x+1, y);

    int incr = getResolution();

    double expectedPhase = oldPhase + (2.0 * M_PI * y * incr) / m_fftSize;

    double phaseError = princarg(newPhase - expectedPhase);

    // The new frequency estimate based on the phase error resulting
    // from assuming the "native" frequency of this bin

    frequency =
        (getSampleRate() * (expectedPhase + phaseError - oldPhase)) /
        (2.0 * M_PI * incr);

    return true;
}

FFTModel::PeakLocationSet
FFTModel::getPeaks(PeakPickType type, int x, int ymin, int ymax) const
{
    Profiler profiler("FFTModel::getPeaks");

    FFTModel::PeakLocationSet peaks;
    if (!isOK()) return peaks;

    if (ymax == 0 || ymax > getHeight() - 1) {
        ymax = getHeight() - 1;
    }

    if (type == AllPeaks) {
        int minbin = ymin;
        if (minbin > 0) minbin = minbin - 1;
        int maxbin = ymax;
        if (maxbin < getHeight() - 1) maxbin = maxbin + 1;
        const int n = maxbin - minbin + 1;
        float *values = new float[n];
        getMagnitudesAt(x, values, minbin, maxbin - minbin + 1);
        for (int bin = ymin; bin <= ymax; ++bin) {
            if (bin == minbin || bin == maxbin) continue;
            if (values[bin - minbin] > values[bin - minbin - 1] &&
                values[bin - minbin] > values[bin - minbin + 1]) {
                peaks.insert(bin);
            }
        }
        delete[] values;
        return peaks;
    }

    Column values = getColumn(x);
    int nv = int(values.size());

    float mean = 0.f;
    for (int i = 0; i < nv; ++i) mean += values[i];
    if (nv > 0) mean = mean / float(values.size());
    
    // For peak picking we use a moving median window, picking the
    // highest value within each continuous region of values that
    // exceed the median.  For pitch adaptivity, we adjust the window
    // size to a roughly constant pitch range (about four tones).

    sv_samplerate_t sampleRate = getSampleRate();

    deque<float> window;
    vector<int> inrange;
    float dist = 0.5;

    int medianWinSize = getPeakPickWindowSize(type, sampleRate, ymin, dist);
    int halfWin = medianWinSize/2;

    int binmin;
    if (ymin > halfWin) binmin = ymin - halfWin;
    else binmin = 0;

    int binmax;
    if (ymax + halfWin < nv) binmax = ymax + halfWin;
    else binmax = nv - 1;

    int prevcentre = 0;

    for (int bin = binmin; bin <= binmax; ++bin) {

        float value = values[bin];

        window.push_back(value);

        // so-called median will actually be the dist*100'th percentile
        medianWinSize = getPeakPickWindowSize(type, sampleRate, bin, dist);
        halfWin = medianWinSize/2;

        while ((int)window.size() > medianWinSize) {
            window.pop_front();
        }

        int actualSize = int(window.size());

        if (type == MajorPitchAdaptivePeaks) {
            if (ymax + halfWin < nv) binmax = ymax + halfWin;
            else binmax = nv - 1;
        }

        deque<float> sorted(window);
        sort(sorted.begin(), sorted.end());
        float median = sorted[int(float(sorted.size()) * dist)];

        int centrebin = 0;
        if (bin > actualSize/2) centrebin = bin - actualSize/2;
        
        while (centrebin > prevcentre || bin == binmin) {

            if (centrebin > prevcentre) ++prevcentre;

            float centre = values[prevcentre];

            if (centre > median) {
                inrange.push_back(centrebin);
            }

            if (centre <= median || centrebin+1 == nv) {
                if (!inrange.empty()) {
                    int peakbin = 0;
                    float peakval = 0.f;
                    for (int i = 0; i < (int)inrange.size(); ++i) {
                        if (i == 0 || values[inrange[i]] > peakval) {
                            peakval = values[inrange[i]];
                            peakbin = inrange[i];
                        }
                    }
                    inrange.clear();
                    if (peakbin >= ymin && peakbin <= ymax) {
                        peaks.insert(peakbin);
                    }
                }
            }

            if (bin == binmin) break;
        }
    }

    return peaks;
}

int
FFTModel::getPeakPickWindowSize(PeakPickType type, sv_samplerate_t sampleRate,
                                int bin, float &percentile) const
{
    percentile = 0.5;
    if (type == MajorPeaks) return 10;
    if (bin == 0) return 3;

    double binfreq = (sampleRate * bin) / m_fftSize;
    double hifreq = Pitch::getFrequencyForPitch(73, 0, binfreq);

    int hibin = int(lrint((hifreq * m_fftSize) / sampleRate));
    int medianWinSize = hibin - bin;
    if (medianWinSize < 3) medianWinSize = 3;

    percentile = 0.5f + float(binfreq / sampleRate);

    return medianWinSize;
}

FFTModel::PeakSet
FFTModel::getPeakFrequencies(PeakPickType type, int x,
                             int ymin, int ymax) const
{
    Profiler profiler("FFTModel::getPeakFrequencies");

    PeakSet peaks;
    if (!isOK()) return peaks;
    PeakLocationSet locations = getPeaks(type, x, ymin, ymax);

    sv_samplerate_t sampleRate = getSampleRate();
    int incr = getResolution();

    // This duplicates some of the work of estimateStableFrequency to
    // allow us to retrieve the phases in two separate vertical
    // columns, instead of jumping back and forth between columns x and
    // x+1, which may be significantly slower if re-seeking is needed

    vector<float> phases;
    for (PeakLocationSet::iterator i = locations.begin();
         i != locations.end(); ++i) {
        phases.push_back(getPhaseAt(x, *i));
    }

    int phaseIndex = 0;
    for (PeakLocationSet::iterator i = locations.begin();
         i != locations.end(); ++i) {
        double oldPhase = phases[phaseIndex];
        double newPhase = getPhaseAt(x+1, *i);
        double expectedPhase = oldPhase + (2.0 * M_PI * *i * incr) / m_fftSize;
        double phaseError = princarg(newPhase - expectedPhase);
        double frequency =
            (sampleRate * (expectedPhase + phaseError - oldPhase))
            / (2 * M_PI * incr);
        peaks[*i] = frequency;
        ++phaseIndex;
    }

    return peaks;
}

