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

#include "Pitch.h"

#include <cmath>

float
Pitch::getFrequencyForPitch(int midiPitch,
			    float centsOffset,
			    float concertA)
{
    float p = float(midiPitch) + (centsOffset / 100);
    return concertA * powf(2.0, (p - 69.0) / 12.0);
}

int
Pitch::getPitchForFrequency(float frequency,
			    float *centsOffsetReturn,
			    float concertA)
{
    float p = 12.0 * (log(frequency / (concertA / 2.0)) / log(2.0)) + 57.0;

    int midiPitch = int(p + 0.00001);
    float centsOffset = (p - midiPitch) * 100.0;

    if (centsOffset >= 50.0) {
	midiPitch = midiPitch + 1;
	centsOffset = -(100.0 - centsOffset);
    }
    
    if (centsOffsetReturn) *centsOffsetReturn = centsOffset;
    return midiPitch;
}

static QString notes[] = {
    "C%1",  "C#%1", "D%1",  "D#%1",
    "E%1",  "F%1",  "F#%1", "G%1",
    "G#%1", "A%1",  "A#%1", "B%1"
};

static QString flatNotes[] = {
    "C%1",  "Db%1", "D%1",  "Eb%1",
    "E%1",  "F%1",  "Gb%1", "G%1",
    "Ab%1", "A%1",  "Bb%1", "B%1"
};

QString
Pitch::getPitchLabel(int midiPitch,
		     float centsOffset,
		     bool useFlats)
{
    int octave = -2;

    if (midiPitch < 0) {
	while (midiPitch < 0) {
	    midiPitch += 12;
	    --octave;
	}
    } else {
	octave = midiPitch / 12 - 2;
    }

    QString plain = (useFlats ? flatNotes : notes)[midiPitch % 12].arg(octave);

    int ic = lrintf(centsOffset);
    if (ic == 0) return plain;
    else if (ic > 0) return QString("%1+%2c").arg(plain).arg(ic);
    else return QString("%1%2c").arg(plain).arg(ic);
}

QString
Pitch::getPitchLabelForFrequency(float frequency,
				 float concertA,
				 bool useFlats)
{
    float centsOffset = 0.0;
    int midiPitch = getPitchForFrequency(frequency, &centsOffset, concertA);
    return getPitchLabel(midiPitch, centsOffset, useFlats);
}
