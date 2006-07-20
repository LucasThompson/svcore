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

#include "Preferences.h"

Preferences *
Preferences::m_instance = new Preferences();

Preferences::Preferences() :
    m_smoothSpectrogram(true),
    m_tuningFrequency(440),
    m_propertyBoxLayout(VerticallyStacked),
    m_windowType(HanningWindow)
{
}

Preferences::PropertyList
Preferences::getProperties() const
{
    PropertyList props;
    props.push_back("Smooth Spectrogram");
    props.push_back("Tuning Frequency");
    props.push_back("Property Box Layout");
    props.push_back("Window Type");
    return props;
}

QString
Preferences::getPropertyLabel(const PropertyName &name) const
{
    if (name == "Smooth Spectrogram") {
        return tr("Spectrogram Display Smoothing");
    }
    if (name == "Tuning Frequency") {
        return tr("Tuning Frequency (concert A)");
    }
    if (name == "Property Box Layout") {
        return tr("Arrangement of Layer Properties");
    }
    if (name == "Window Type") {
        return tr("Spectral Analysis Window Shape");
    }
    return name;
}

Preferences::PropertyType
Preferences::getPropertyType(const PropertyName &name) const
{
    if (name == "Smooth Spectrogram") {
        return ToggleProperty;
    }
    if (name == "Tuning Frequency") {
        return RangeProperty;
    }
    if (name == "Property Box Layout") {
        return ValueProperty;
    }
    if (name == "Window Type") {
        return ValueProperty;
    }
    return InvalidProperty;
}

int
Preferences::getPropertyRangeAndValue(const PropertyName &name,
                                      int *min, int *max) const
{
    if (name == "Smooth Spectrogram") {
        if (min) *min = 0;
        if (max) *max = 1;
        return m_smoothSpectrogram ? 1 : 0;
    }

    //!!! freq mapping

    if (name == "Property Box Layout") {
        if (min) *min = 0;
        if (max) *max = 1;
        return m_propertyBoxLayout == Layered ? 1 : 0;
    }        

    if (name == "Window Type") {
        if (min) *min = int(RectangularWindow);
        if (max) *max = int(ParzenWindow);
        return int(m_windowType);
    }

    return 0;
}

QString
Preferences::getPropertyValueLabel(const PropertyName &name,
                                   int value) const
{
    if (name == "Property Box Layout") {
        if (value == 0) return tr("Vertically Stacked");
        else return tr("Layered");
    }
    if (name == "Window Type") {
        switch (WindowType(value)) {
        case RectangularWindow: return tr("Rectangular");
        case BartlettWindow: return tr("Bartlett");
        case HammingWindow: return tr("Hamming");
        case HanningWindow: return tr("Hanning");
        case BlackmanWindow: return tr("Blackman");
        case GaussianWindow: return tr("Gaussian");
        case ParzenWindow: return tr("Parzen");
        }
    }
    return "";
}

QString
Preferences::getPropertyContainerName() const
{
    return tr("Preferences");
}

QString
Preferences::getPropertyContainerIconName() const
{
    return "preferences";
}

void
Preferences::setProperty(const PropertyName &name, int value) 
{
    if (name == "Smooth Spectrogram") {
        setSmoothSpectrogram(value > 0.1);
    } else if (name == "Tuning Frequency") {
        //!!!
    } else if (name == "Property Box Layout") {
        setPropertyBoxLayout(value == 0 ? VerticallyStacked : Layered);
    } else if (name == "Window Type") {
        setWindowType(WindowType(value));
    }
}

void
Preferences::setSmoothSpectrogram(bool smooth)
{
    if (m_smoothSpectrogram != smooth) {
        m_smoothSpectrogram = smooth;
//!!!    emit 
    }
}

void
Preferences::setTuningFrequency(float freq)
{
    if (m_tuningFrequency != freq) {
        m_tuningFrequency = freq;
        //!!! emit
    }
}

void
Preferences::setPropertyBoxLayout(PropertyBoxLayout layout)
{
    if (m_propertyBoxLayout != layout) {
        m_propertyBoxLayout = layout;
        //!!! emit
    }
}

void
Preferences::setWindowType(WindowType type)
{
    if (m_windowType != type) {
        m_windowType = type;
        //!!! emit
    }
}
