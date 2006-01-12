/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
    
    This is experimental software.  Not for distribution.
*/

#include "Model.h"

const int Model::COMPLETION_UNKNOWN = -1;

QString
Model::toXmlString(QString indent, QString extraAttributes) const
{
    QString s;
    
    s += indent;

    s += QString("<model id=\"%1\" name=\"%2\" %3/>\n")
	.arg((intptr_t)this)
	.arg(objectName())
	.arg(extraAttributes);

    return s;
}

#ifdef INCLUDE_MOCFILES
#ifdef INCLUDE_MOCFILES
#include "Model.moc.cpp"
#endif
#endif

