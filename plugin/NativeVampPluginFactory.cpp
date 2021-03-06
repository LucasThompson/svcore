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

#include "NativeVampPluginFactory.h"
#include "PluginIdentifier.h"

#include <vamp-hostsdk/PluginHostAdapter.h>
#include <vamp-hostsdk/PluginWrapper.h>

#include "system/System.h"

#include "PluginScan.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

#include <iostream>

#include "base/Profiler.h"

#include <QMutex>
#include <QMutexLocker>

using namespace std;

//#define DEBUG_PLUGIN_SCAN_AND_INSTANTIATE 1

class PluginDeletionNotifyAdapter : public Vamp::HostExt::PluginWrapper {
public:
    PluginDeletionNotifyAdapter(Vamp::Plugin *plugin,
                                NativeVampPluginFactory *factory) :
        PluginWrapper(plugin), m_factory(factory) { }
    virtual ~PluginDeletionNotifyAdapter();
protected:
    NativeVampPluginFactory *m_factory;
};

PluginDeletionNotifyAdapter::~PluginDeletionNotifyAdapter()
{
    // see notes in vamp-sdk/hostext/PluginLoader.cpp from which this is drawn
    Vamp::Plugin *p = m_plugin;
    delete m_plugin;
    m_plugin = 0;
    // acceptable use after free here, as pluginDeleted uses p only as
    // pointer key and does not deref it
    if (m_factory) m_factory->pluginDeleted(p);
}

vector<QString>
NativeVampPluginFactory::getPluginPath()
{
    if (!m_pluginPath.empty()) return m_pluginPath;

    vector<string> p = Vamp::PluginHostAdapter::getPluginPath();
    for (size_t i = 0; i < p.size(); ++i) m_pluginPath.push_back(p[i].c_str());
    return m_pluginPath;
}

static
QList<PluginScan::Candidate>
getCandidateLibraries()
{
#ifdef HAVE_PLUGIN_CHECKER_HELPER
    return PluginScan::getInstance()->getCandidateLibrariesFor
        (PluginScan::VampPlugin);
#else
    auto path = Vamp::PluginHostAdapter::getPluginPath();
    QList<PluginScan::Candidate> candidates;
    for (string dirname: path) {
        SVDEBUG << "NativeVampPluginFactory: scanning directory myself: "
                << dirname << endl;
#if defined(_WIN32)
#define PLUGIN_GLOB "*.dll"
#elif defined(__APPLE__)
#define PLUGIN_GLOB "*.dylib *.so"
#else
#define PLUGIN_GLOB "*.so"
#endif
        QDir dir(dirname.c_str(), PLUGIN_GLOB,
                 QDir::Name | QDir::IgnoreCase,
                 QDir::Files | QDir::Readable);

        for (unsigned int i = 0; i < dir.count(); ++i) {
            QString soname = dir.filePath(dir[i]);
            candidates.push_back({ soname, "" });
        }
    }

    return candidates;
#endif
}

vector<QString>
NativeVampPluginFactory::getPluginIdentifiers(QString &)
{
    Profiler profiler("NativeVampPluginFactory::getPluginIdentifiers");

    QMutexLocker locker(&m_mutex);

    if (!m_identifiers.empty()) {
        return m_identifiers;
    }

    auto candidates = getCandidateLibraries();
    
    SVDEBUG << "INFO: Have " << candidates.size() << " candidate Vamp plugin libraries" << endl;
        
    for (auto candidate : candidates) {

        QString soname = candidate.libraryPath;

        SVDEBUG << "INFO: Considering candidate Vamp plugin library " << soname << endl;
        
        void *libraryHandle = DLOPEN(soname, RTLD_LAZY | RTLD_LOCAL);
            
        if (!libraryHandle) {
            SVDEBUG << "WARNING: NativeVampPluginFactory::getPluginIdentifiers: Failed to load library " << soname << ": " << DLERROR() << endl;
            continue;
        }

        VampGetPluginDescriptorFunction fn = (VampGetPluginDescriptorFunction)
            DLSYM(libraryHandle, "vampGetPluginDescriptor");

        if (!fn) {
            SVDEBUG << "WARNING: NativeVampPluginFactory::getPluginIdentifiers: No descriptor function in " << soname << endl;
            if (DLCLOSE(libraryHandle) != 0) {
                SVDEBUG << "WARNING: NativeVampPluginFactory::getPluginIdentifiers: Failed to unload library " << soname << endl;
            }
            continue;
        }

#ifdef DEBUG_PLUGIN_SCAN_AND_INSTANTIATE
            cerr << "NativeVampPluginFactory::getPluginIdentifiers: Vamp descriptor found" << endl;
#endif

        const VampPluginDescriptor *descriptor = 0;
        int index = 0;

        map<string, int> known;
        bool ok = true;

        while ((descriptor = fn(VAMP_API_VERSION, index))) {

            if (known.find(descriptor->identifier) != known.end()) {
                SVDEBUG << "WARNING: NativeVampPluginFactory::getPluginIdentifiers: Plugin library "
                     << soname
                     << " returns the same plugin identifier \""
                     << descriptor->identifier << "\" at indices "
                     << known[descriptor->identifier] << " and "
                     << index << endl;
                    SVDEBUG << "NativeVampPluginFactory::getPluginIdentifiers: Avoiding this library (obsolete API?)" << endl;
                ok = false;
                break;
            } else {
                known[descriptor->identifier] = index;
            }

            ++index;
        }

        if (ok) {

            index = 0;

            while ((descriptor = fn(VAMP_API_VERSION, index))) {

                QString id = PluginIdentifier::createIdentifier
                    ("vamp", soname, descriptor->identifier);
                m_identifiers.push_back(id);
#ifdef DEBUG_PLUGIN_SCAN_AND_INSTANTIATE
                cerr << "NativeVampPluginFactory::getPluginIdentifiers: Found plugin id " << id << " at index " << index << endl;
#endif
                ++index;
            }
        }
            
        if (DLCLOSE(libraryHandle) != 0) {
            SVDEBUG << "WARNING: NativeVampPluginFactory::getPluginIdentifiers: Failed to unload library " << soname << endl;
        }
    }

    generateTaxonomy();

    // Plugins can change the locale, revert it to default.
    RestoreStartupLocale();

    return m_identifiers;
}

QString
NativeVampPluginFactory::findPluginFile(QString soname, QString inDir)
{
    QString file = "";

#ifdef DEBUG_PLUGIN_SCAN_AND_INSTANTIATE
    cerr << "NativeVampPluginFactory::findPluginFile(\""
              << soname << "\", \"" << inDir << "\")"
              << endl;
#endif

    if (inDir != "") {

        QDir dir(inDir, PLUGIN_GLOB,
                 QDir::Name | QDir::IgnoreCase,
                 QDir::Files | QDir::Readable);
        if (!dir.exists()) return "";

        file = dir.filePath(QFileInfo(soname).fileName());

        if (QFileInfo(file).exists() && QFileInfo(file).isFile()) {

#ifdef DEBUG_PLUGIN_SCAN_AND_INSTANTIATE
            cerr << "NativeVampPluginFactory::findPluginFile: "
                      << "found trivially at " << file << endl;
#endif

            return file;
        }

	for (unsigned int j = 0; j < dir.count(); ++j) {
            file = dir.filePath(dir[j]);
            if (QFileInfo(file).baseName() == QFileInfo(soname).baseName()) {

#ifdef DEBUG_PLUGIN_SCAN_AND_INSTANTIATE
                cerr << "NativeVampPluginFactory::findPluginFile: "
                          << "found \"" << soname << "\" at " << file << endl;
#endif

                return file;
            }
        }

#ifdef DEBUG_PLUGIN_SCAN_AND_INSTANTIATE
        cerr << "NativeVampPluginFactory::findPluginFile (with dir): "
                  << "not found" << endl;
#endif

        return "";

    } else {

        QFileInfo fi(soname);

        if (fi.isAbsolute() && fi.exists() && fi.isFile()) {
#ifdef DEBUG_PLUGIN_SCAN_AND_INSTANTIATE
            cerr << "NativeVampPluginFactory::findPluginFile: "
                      << "found trivially at " << soname << endl;
#endif
            return soname;
        }

        if (fi.isAbsolute() && fi.absolutePath() != "") {
            file = findPluginFile(soname, fi.absolutePath());
            if (file != "") return file;
        }

        vector<QString> path = getPluginPath();
        for (vector<QString>::iterator i = path.begin();
             i != path.end(); ++i) {
            if (*i != "") {
                file = findPluginFile(soname, *i);
                if (file != "") return file;
            }
        }

#ifdef DEBUG_PLUGIN_SCAN_AND_INSTANTIATE
        cerr << "NativeVampPluginFactory::findPluginFile: "
                  << "not found" << endl;
#endif

        return "";
    }
}

Vamp::Plugin *
NativeVampPluginFactory::instantiatePlugin(QString identifier,
                                           sv_samplerate_t inputSampleRate)
{
    Profiler profiler("NativeVampPluginFactory::instantiatePlugin");

    Vamp::Plugin *rv = 0;
    Vamp::PluginHostAdapter *plugin = 0;

    const VampPluginDescriptor *descriptor = 0;
    int index = 0;

    QString type, soname, label;
    PluginIdentifier::parseIdentifier(identifier, type, soname, label);
    if (type != "vamp") {
#ifdef DEBUG_PLUGIN_SCAN_AND_INSTANTIATE
        cerr << "NativeVampPluginFactory::instantiatePlugin: Wrong factory for plugin type " << type << endl;
#endif
	return 0;
    }

    QString found = findPluginFile(soname);

    if (found == "") {
        SVDEBUG << "NativeVampPluginFactory::instantiatePlugin: Failed to find library file " << soname << endl;
        return 0;
    } else if (found != soname) {

#ifdef DEBUG_PLUGIN_SCAN_AND_INSTANTIATE
        cerr << "NativeVampPluginFactory::instantiatePlugin: Given library name was " << soname << ", found at " << found << endl;
        cerr << soname << " -> " << found << endl;
#endif

    }        

    soname = found;

    void *libraryHandle = DLOPEN(soname, RTLD_LAZY | RTLD_LOCAL);
            
    if (!libraryHandle) {
        SVDEBUG << "NativeVampPluginFactory::instantiatePlugin: Failed to load library " << soname << ": " << DLERROR() << endl;
        return 0;
    }

    VampGetPluginDescriptorFunction fn = (VampGetPluginDescriptorFunction)
        DLSYM(libraryHandle, "vampGetPluginDescriptor");
    
    if (!fn) {
        SVDEBUG << "NativeVampPluginFactory::instantiatePlugin: No descriptor function in " << soname << endl;
        goto done;
    }

    while ((descriptor = fn(VAMP_API_VERSION, index))) {
        if (label == descriptor->identifier) break;
        ++index;
    }

    if (!descriptor) {
        SVDEBUG << "NativeVampPluginFactory::instantiatePlugin: Failed to find plugin \"" << label << "\" in library " << soname << endl;
        goto done;
    }

    plugin = new Vamp::PluginHostAdapter(descriptor, float(inputSampleRate));

    if (plugin) {
        m_handleMap[plugin] = libraryHandle;
        rv = new PluginDeletionNotifyAdapter(plugin, this);
    }

//    SVDEBUG << "NativeVampPluginFactory::instantiatePlugin: Constructed Vamp plugin, rv is " << rv << endl;

    //!!! need to dlclose() when plugins from a given library are unloaded

done:
    if (!rv) {
        if (DLCLOSE(libraryHandle) != 0) {
            SVDEBUG << "WARNING: NativeVampPluginFactory::instantiatePlugin: Failed to unload library " << soname << endl;
        }
    }

#ifdef DEBUG_PLUGIN_SCAN_AND_INSTANTIATE
    cerr << "NativeVampPluginFactory::instantiatePlugin: Instantiated plugin " << label << " from library " << soname << ": descriptor " << descriptor << ", rv "<< rv << ", label " << rv->getName() << ", outputs " << rv->getOutputDescriptors().size() << endl;
#endif
    
    return rv;
}

void
NativeVampPluginFactory::pluginDeleted(Vamp::Plugin *plugin)
{
    void *handle = m_handleMap[plugin];
    if (handle) {
#ifdef DEBUG_PLUGIN_SCAN_AND_INSTANTIATE
        cerr << "unloading library " << handle << " for plugin " << plugin << endl;
#endif
        DLCLOSE(handle);
    }
    m_handleMap.erase(plugin);
}

QString
NativeVampPluginFactory::getPluginCategory(QString identifier)
{
    return m_taxonomy[identifier];
}

void
NativeVampPluginFactory::generateTaxonomy()
{
    vector<QString> pluginPath = getPluginPath();
    vector<QString> path;

    for (size_t i = 0; i < pluginPath.size(); ++i) {
	if (pluginPath[i].contains("/lib/")) {
	    QString p(pluginPath[i]);
            path.push_back(p);
	    p.replace("/lib/", "/share/");
	    path.push_back(p);
	}
	path.push_back(pluginPath[i]);
    }

    for (size_t i = 0; i < path.size(); ++i) {

	QDir dir(path[i], "*.cat");

//	SVDEBUG << "LADSPAPluginFactory::generateFallbackCategories: directory " << path[i] << " has " << dir.count() << " .cat files" << endl;
	for (unsigned int j = 0; j < dir.count(); ++j) {

	    QFile file(path[i] + "/" + dir[j]);

//	    SVDEBUG << "LADSPAPluginFactory::generateFallbackCategories: about to open " << (path[i]+ "/" + dir[j]) << endl;

	    if (file.open(QIODevice::ReadOnly)) {
//		    cerr << "...opened" << endl;
		QTextStream stream(&file);
		QString line;

		while (!stream.atEnd()) {
		    line = stream.readLine();
//		    cerr << "line is: \"" << line << "\"" << endl;
		    QString id = PluginIdentifier::canonicalise
                        (line.section("::", 0, 0));
		    QString cat = line.section("::", 1, 1);
		    m_taxonomy[id] = cat;
//		    cerr << "NativeVampPluginFactory: set id \"" << id << "\" to cat \"" << cat << "\"" << endl;
		}
	    }
	}
    }
}    

piper_vamp::PluginStaticData
NativeVampPluginFactory::getPluginStaticData(QString identifier)
{
    QMutexLocker locker(&m_mutex);

    if (m_pluginData.find(identifier) != m_pluginData.end()) {
        return m_pluginData[identifier];
    }
    
    QString type, soname, label;
    PluginIdentifier::parseIdentifier(identifier, type, soname, label);
    std::string pluginKey = (soname + ":" + label).toStdString();

    std::vector<std::string> catlist;
    for (auto s: getPluginCategory(identifier).split(" > ")) {
        catlist.push_back(s.toStdString());
    }
    
    Vamp::Plugin *p = instantiatePlugin(identifier, 44100);
    if (!p) return {};

    auto psd = piper_vamp::PluginStaticData::fromPlugin(pluginKey,
                                                        catlist,
                                                        p);

    delete p;
    
    m_pluginData[identifier] = psd;
    return psd;
}

