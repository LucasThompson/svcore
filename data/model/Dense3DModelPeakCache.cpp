/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2009 QMUL.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "Dense3DModelPeakCache.h"

#include "base/Profiler.h"

#include "base/HitCount.h"

Dense3DModelPeakCache::Dense3DModelPeakCache(const DenseThreeDimensionalModel *source,
					     int columnsPerPeak) :
    m_source(source),
    m_columnsPerPeak(columnsPerPeak)
{
    m_cache = new EditableDenseThreeDimensionalModel
        (source->getSampleRate(),
         getResolution(),
         source->getHeight(),
         EditableDenseThreeDimensionalModel::NoCompression,
         false);

    connect(source, SIGNAL(modelChanged()),
            this, SLOT(sourceModelChanged()));
    connect(source, SIGNAL(aboutToBeDeleted()),
            this, SLOT(sourceModelAboutToBeDeleted()));
}

Dense3DModelPeakCache::~Dense3DModelPeakCache()
{
    if (m_cache) m_cache->aboutToDelete();
    delete m_cache;
}

Dense3DModelPeakCache::Column
Dense3DModelPeakCache::getColumn(int column) const
{
    if (!m_source) return Column();
    if (!haveColumn(column)) fillColumn(column);
    return m_cache->getColumn(column);
}

float
Dense3DModelPeakCache::getValueAt(int column, int n) const
{
    if (!m_source) return 0.f;
    if (!haveColumn(column)) fillColumn(column);
    return m_cache->getValueAt(column, n);
}

void
Dense3DModelPeakCache::sourceModelChanged()
{
    if (!m_source) return;
    if (m_coverage.size() > 0) {
        // The last peak may have come from an incomplete read, which
        // may since have been filled, so reset it
        m_coverage[m_coverage.size()-1] = false;
    }
    m_coverage.resize(getWidth(), false); // retaining data
}

void
Dense3DModelPeakCache::sourceModelAboutToBeDeleted()
{
    m_source = 0;
}

bool
Dense3DModelPeakCache::haveColumn(int column) const
{
    static HitCount count("Dense3DModelPeakCache");
    if (in_range_for(m_coverage, column) && m_coverage[column]) {
        count.hit();
        return true;
    } else {
        count.miss();
        return false;
    }
}

void
Dense3DModelPeakCache::fillColumn(int column) const
{
    Profiler profiler("Dense3DModelPeakCache::fillColumn");

    if (!in_range_for(m_coverage, column)) {
        if (m_coverage.size() > 0) {
            // The last peak may have come from an incomplete read, which
            // may since have been filled, so reset it
            m_coverage[m_coverage.size()-1] = false;
        }
        m_coverage.resize(column + 1, false);
    }

    int sourceWidth = m_source->getWidth();
    
    Column peak;
    int n = 0;
    for (int i = 0; i < m_columnsPerPeak; ++i) {

        int sourceColumn = column * m_columnsPerPeak + i;
        if (sourceColumn >= sourceWidth) break;
        
        Column here = m_source->getColumn(sourceColumn);

//        cerr << "Dense3DModelPeakCache::fillColumn(" << column << "): source col "
//             << sourceColumn << " of " << sourceWidth
//             << " returned " << here.size() << " elts" << endl;
        
        if (i == 0) {
            peak = here;
            n = int(peak.size());
        } else {
            int m = std::min(n, int(here.size()));
            for (int j = 0; j < m; ++j) {
                peak[j] = std::max(here[j], peak[j]);
            }
        }
    }

    m_cache->setColumn(column, peak);
    m_coverage[column] = true;
}


