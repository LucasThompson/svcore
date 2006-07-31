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

#ifndef _TEXT_MODEL_H_
#define _TEXT_MODEL_H_

#include "SparseModel.h"
#include "base/RealTime.h"

/**
 * Text point type for use in a SparseModel.  This represents a piece
 * of text at a given time and y-value in the [0,1) range (indicative
 * of height on the window).  Intended for casual textual annotations.
 */

struct TextPoint
{
public:
    TextPoint(long _frame) : frame(_frame), height(0.0f) { }
    TextPoint(long _frame, float _height, QString _label) : 
	frame(_frame), height(_height), label(_label) { }

    int getDimensions() const { return 2; }
    
    long frame;
    float height;
    QString label;
    
    QString toXmlString(QString indent = "",
			QString extraAttributes = "") const
    {
	return QString("%1<point frame=\"%2\" height=\"%3\" label=\"%4\" %5/>\n")
	    .arg(indent).arg(frame).arg(height).arg(label).arg(extraAttributes);
    }

    QString toDelimitedDataString(QString delimiter, size_t sampleRate) const
    {
        QStringList list;
        list << RealTime::frame2RealTime(frame, sampleRate).toString().c_str();
        list << QString("%1").arg(height);
        list << label;
        return list.join(delimiter);
    }

    struct Comparator {
	bool operator()(const TextPoint &p1,
			const TextPoint &p2) const {
	    if (p1.frame != p2.frame) return p1.frame < p2.frame;
	    if (p1.height != p2.height) return p1.height < p2.height;
	    return p1.label < p2.label;
	}
    };
    
    struct OrderComparator {
	bool operator()(const TextPoint &p1,
			const TextPoint &p2) const {
	    return p1.frame < p2.frame;
	}
    };
};


// Make this a class rather than a typedef so it can be predeclared.

class TextModel : public SparseModel<TextPoint>
{
public:
    TextModel(size_t sampleRate, size_t resolution, bool notifyOnAdd = true) :
	SparseModel<TextPoint>(sampleRate, resolution, notifyOnAdd)
    { }

    virtual QString toXmlString(QString indent = "",
				QString extraAttributes = "") const
    {
	return SparseModel<TextPoint>::toXmlString
	    (indent,
	     QString("%1 subtype=\"text\"")
	     .arg(extraAttributes));
    }
};


#endif


    