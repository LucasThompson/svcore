/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2006 Chris Cannam and QMUL.
   
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _MODEL_TRANSFORMER_FACTORY_H_
#define _MODEL_TRANSFORMER_FACTORY_H_

#include "Transform.h"
#include "TransformDescription.h"

#include "ModelTransformer.h"

#include "PluginTransformer.h"

#include <map>
#include <set>

namespace Vamp { class PluginBase; }

class AudioCallbackPlaySource;

class ModelTransformerFactory : public QObject
{
    Q_OBJECT

public:
    virtual ~ModelTransformerFactory();

    static ModelTransformerFactory *getInstance();

    /**
     * Get a configuration XML string for the given transform (by
     * asking the user, most likely).  Returns the selected input
     * model if the transform is acceptable, 0 if the operation should
     * be cancelled.  Audio callback play source may be used to
     * audition effects plugins, if provided.
     */
    Model *getConfigurationForTransformer(TransformId identifier,
                                          const std::vector<Model *> &candidateInputModels,
                                          Model *defaultInputModel,
                                          PluginTransformer::ExecutionContext &context,
                                          QString &configurationXml,
                                          AudioCallbackPlaySource *source = 0,
                                          size_t startFrame = 0,
                                          size_t duration = 0);

    /**
     * Get the default execution context for the given transform
     * and input model (if known).
     */
    PluginTransformer::ExecutionContext getDefaultContextForTransformer(TransformId identifier,
                                                                        Model *inputModel = 0);

    /**
     * Return the output model resulting from applying the named
     * transform to the given input model.  The transform may still be
     * working in the background when the model is returned; check the
     * output model's isReady completion status for more details.
     *
     * If the transform is unknown or the input model is not an
     * appropriate type for the given transform, or if some other
     * problem occurs, return 0.
     * 
     * The returned model is owned by the caller and must be deleted
     * when no longer needed.
     */
    Model *transform(TransformId identifier, Model *inputModel,
                     const PluginTransformer::ExecutionContext &context,
                     QString configurationXml = "");

protected slots:
    void transformerFinished();

    void modelAboutToBeDeleted(Model *);

protected:
    ModelTransformer *createTransformer(TransformId identifier, Model *inputModel,
                                        const PluginTransformer::ExecutionContext &context,
                                        QString configurationXml);

    typedef std::map<TransformId, QString> TransformerConfigurationMap;
    TransformerConfigurationMap m_lastConfigurations;

    typedef std::set<ModelTransformer *> TransformerSet;
    TransformerSet m_runningTransformers;

    bool getChannelRange(TransformId identifier,
                         Vamp::PluginBase *plugin, int &min, int &max);

    static ModelTransformerFactory *m_instance;
};


#endif
