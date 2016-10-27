/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2006-2016 Chris Cannam and QMUL.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef SV_PIPER_VAMP_PLUGIN_FACTORY_H
#define SV_PIPER_VAMP_PLUGIN_FACTORY_H

#include "FeatureExtractionPluginFactory.h"

#include <QMutex>
#include <vector>
#include <map>

#include "base/Debug.h"

/**
 * FeatureExtractionPluginFactory type for Vamp plugins hosted in a
 * separate process using Piper protocol.
 */
class PiperVampPluginFactory : public FeatureExtractionPluginFactory
{
public:
    PiperVampPluginFactory();

    virtual ~PiperVampPluginFactory() { }

    virtual std::vector<QString> getPluginIdentifiers(QString &errorMessage)
        override;

    virtual piper_vamp::PluginStaticData getPluginStaticData(QString identifier)
        override;
    
    virtual Vamp::Plugin *instantiatePlugin(QString identifier,
                                            sv_samplerate_t inputSampleRate)
        override;

    virtual QString getPluginCategory(QString identifier) override;

protected:
    QMutex m_mutex;
    std::string m_serverName;
    std::map<QString, piper_vamp::PluginStaticData> m_pluginData; // identifier -> data
    std::map<QString, QString> m_taxonomy; // identifier -> category string
    void populate(QString &errorMessage);
};

#endif