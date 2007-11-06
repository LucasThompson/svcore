TEMPLATE = lib

SV_UNIT_PACKAGES = vamp vamp-hostsdk lrdf raptor
load(../sv.prf)

CONFIG += sv staticlib qt thread warn_on stl rtti exceptions
QT += xml

TARGET = svplugin

# Doesn't work with this library, which contains C99 as well as C++
PRECOMPILED_HEADER =

DEPENDPATH += . .. api plugins api/alsa api/alsa/sound transform
INCLUDEPATH += . .. api api/alsa plugins api/alsa/sound transform
OBJECTS_DIR = tmp_obj
MOC_DIR = tmp_moc

# Input
HEADERS += DSSIPluginFactory.h \
           DSSIPluginInstance.h \
           FeatureExtractionPluginFactory.h \
           LADSPAPluginFactory.h \
           LADSPAPluginInstance.h \
           PluginIdentifier.h \
           PluginXml.h \
           RealTimePluginFactory.h \
           RealTimePluginInstance.h \
           api/dssi.h \
           api/ladspa.h \
           plugins/SamplePlayer.h \
           api/alsa/asoundef.h \
           api/alsa/asoundlib.h \
           api/alsa/seq.h \
           api/alsa/seq_event.h \
           api/alsa/seq_midi_event.h \
           api/alsa/sound/asequencer.h \
           transform/FeatureExtractionPluginTransformer.h \
           transform/PluginTransformer.h \
           transform/RealTimePluginTransformer.h \
           transform/Transform.h \
           transform/TransformDescription.h \
           transform/TransformFactory.h \
           transform/Transformer.h \
           transform/TransformerFactory.h
SOURCES += DSSIPluginFactory.cpp \
           DSSIPluginInstance.cpp \
           FeatureExtractionPluginFactory.cpp \
           LADSPAPluginFactory.cpp \
           LADSPAPluginInstance.cpp \
           PluginIdentifier.cpp \
           PluginXml.cpp \
           RealTimePluginFactory.cpp \
           RealTimePluginInstance.cpp \
           api/dssi_alsa_compat.c \
           plugins/SamplePlayer.cpp \
           transform/FeatureExtractionPluginTransformer.cpp \
           transform/PluginTransformer.cpp \
           transform/RealTimePluginTransformer.cpp \
           transform/Transform.cpp \
           transform/TransformFactory.cpp \
           transform/Transformer.cpp \
           transform/TransformerFactory.cpp
