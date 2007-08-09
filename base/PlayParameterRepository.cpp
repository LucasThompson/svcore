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

#include "PlayParameterRepository.h"
#include "PlayParameters.h"

//!!! shouldn't be including this here -- restructure needed

//!!! should the AudioGenerator actually implement all this stuff itself?  do we even want this class?
#include "sv/audioio/AudioGenerator.h"

#include <iostream>

PlayParameterRepository *
PlayParameterRepository::m_instance = new PlayParameterRepository;

PlayParameterRepository *
PlayParameterRepository::getInstance()
{
    return m_instance;
}

PlayParameterRepository::~PlayParameterRepository()
{
}

void
PlayParameterRepository::addModel(const Model *model)
{
//    std::cerr << "PlayParameterRepository:addModel " << model <<  std::endl;

    if (!getPlayParameters(model)) {

	// Give all models the same type of play parameters for the
	// moment, provided they can be played at all

	if (AudioGenerator::canPlay(model)) {

//	    std::cerr << "PlayParameterRepository: Adding play parameters for " << model << std::endl;

            PlayParameters *params = new PlayParameters;
	    m_playParameters[model] = params;

            params->setPlayPluginId
                (AudioGenerator::getDefaultPlayPluginId(model));

            params->setPlayPluginConfiguration
                (AudioGenerator::getDefaultPlayPluginConfiguration(model));

	    connect(params, SIGNAL(playParametersChanged()),
		    this, SLOT(playParametersChanged()));

	    connect(params, SIGNAL(playPluginIdChanged(QString)),
		    this, SLOT(playPluginIdChanged(QString)));

	    connect(params, SIGNAL(playPluginConfigurationChanged(QString)),
		    this, SLOT(playPluginConfigurationChanged(QString)));

//            std::cerr << "Connected play parameters " << params << " for model "
//                      << model << " to this " << this << std::endl;

	} else {

//	    std::cerr << "PlayParameterRepository: Model " << model << " not playable" <<  std::endl;
	}	    
    }
}    

void
PlayParameterRepository::removeModel(const Model *model)
{
    delete m_playParameters[model];
    m_playParameters.erase(model);
}

void
PlayParameterRepository::copyParameters(const Model *from, const Model *to)
{
    if (!getPlayParameters(from)) {
        std::cerr << "ERROR: PlayParameterRepository::copyParameters: source model unknown" << std::endl;
        return;
    }
    if (!getPlayParameters(to)) {
        std::cerr << "WARNING: PlayParameterRepository::copyParameters: target model unknown, adding it now" << std::endl;
        addModel(to);
    }
    getPlayParameters(to)->copyFrom(getPlayParameters(from));
}

PlayParameters *
PlayParameterRepository::getPlayParameters(const Model *model) 
{
    if (m_playParameters.find(model) == m_playParameters.end()) return 0;
    return m_playParameters.find(model)->second;
}

void
PlayParameterRepository::playParametersChanged()
{
    PlayParameters *params = dynamic_cast<PlayParameters *>(sender());
    emit playParametersChanged(params);
}

void
PlayParameterRepository::playPluginIdChanged(QString id)
{
    PlayParameters *params = dynamic_cast<PlayParameters *>(sender());
    for (ModelParameterMap::iterator i = m_playParameters.begin();
         i != m_playParameters.end(); ++i) {
        if (i->second == params) {
            emit playPluginIdChanged(i->first, id);
            return;
        }
    }
}

void
PlayParameterRepository::playPluginConfigurationChanged(QString config)
{
    PlayParameters *params = dynamic_cast<PlayParameters *>(sender());
//    std::cerr << "PlayParameterRepository::playPluginConfigurationChanged" << std::endl;
    for (ModelParameterMap::iterator i = m_playParameters.begin();
         i != m_playParameters.end(); ++i) {
        if (i->second == params) {
            emit playPluginConfigurationChanged(i->first, config);
            return;
        }
    }
}

void
PlayParameterRepository::clear()
{
//    std::cerr << "PlayParameterRepository: PlayParameterRepository::clear" << std::endl;
    while (!m_playParameters.empty()) {
	delete m_playParameters.begin()->second;
	m_playParameters.erase(m_playParameters.begin());
    }
}

