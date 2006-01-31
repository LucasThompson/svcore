/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
    
    This is experimental software.  Not for distribution.
*/

/*
   This is a modified version of a source file from the 
   Rosegarden MIDI and audio sequencer and notation editor.
   This file copyright 2000-2006 Chris Cannam.
*/

#include "DSSIPluginFactory.h"
#include <iostream>

#include <QString>

#include "DSSIPluginInstance.h"
#include "PluginIdentifier.h"

//!!!
#include "plugins/SamplePlayer.h"

#include "base/System.h"

#ifdef HAVE_LIBLRDF
#include "lrdf.h"
#endif // HAVE_LIBLRDF


DSSIPluginFactory::DSSIPluginFactory() :
    LADSPAPluginFactory()
{
    m_hostDescriptor.DSSI_API_Version = 2;
    m_hostDescriptor.request_transport_information = NULL;
    m_hostDescriptor.request_midi_send = DSSIPluginInstance::requestMidiSend;
    m_hostDescriptor.request_non_rt_thread = DSSIPluginInstance::requestNonRTThread;
    m_hostDescriptor.midi_send = DSSIPluginInstance::midiSend;
}

DSSIPluginFactory::~DSSIPluginFactory()
{
    // nothing else to do here either
}

void
DSSIPluginFactory::enumeratePlugins(std::vector<QString> &list)
{
    for (std::vector<QString>::iterator i = m_identifiers.begin();
	 i != m_identifiers.end(); ++i) {

	const DSSI_Descriptor *ddesc = getDSSIDescriptor(*i);
	if (!ddesc) continue;

	const LADSPA_Descriptor *descriptor = ddesc->LADSPA_Plugin;
	if (!descriptor) continue;
	
//	std::cerr << "DSSIPluginFactory::enumeratePlugins: Name " << (descriptor->Name ? descriptor->Name : "NONE" ) << std::endl;

	list.push_back(*i);
	list.push_back(descriptor->Name);
	list.push_back(QString("%1").arg(descriptor->UniqueID));
	list.push_back(descriptor->Label);
	list.push_back(descriptor->Maker);
	list.push_back(descriptor->Copyright);
	list.push_back((ddesc->run_synth || ddesc->run_multiple_synths) ? "true" : "false");
	list.push_back(ddesc->run_multiple_synths ? "true" : "false");
	list.push_back(m_taxonomy[descriptor->UniqueID]);
	list.push_back(QString("%1").arg(descriptor->PortCount));

	for (unsigned long p = 0; p < descriptor->PortCount; ++p) {

	    int type = 0;
	    if (LADSPA_IS_PORT_CONTROL(descriptor->PortDescriptors[p])) {
		type |= PortType::Control;
	    } else {
		type |= PortType::Audio;
	    }
	    if (LADSPA_IS_PORT_INPUT(descriptor->PortDescriptors[p])) {
		type |= PortType::Input;
	    } else {
		type |= PortType::Output;
	    }

	    list.push_back(QString("%1").arg(p));
	    list.push_back(descriptor->PortNames[p]);
	    list.push_back(QString("%1").arg(type));
	    list.push_back(QString("%1").arg(getPortDisplayHint(descriptor, p)));
	    list.push_back(QString("%1").arg(getPortMinimum(descriptor, p)));
	    list.push_back(QString("%1").arg(getPortMaximum(descriptor, p)));
	    list.push_back(QString("%1").arg(getPortDefault(descriptor, p)));
	}
    }

    unloadUnusedLibraries();
}
	
RealTimePluginInstance *
DSSIPluginFactory::instantiatePlugin(QString identifier,
				     int instrument,
				     int position,
				     unsigned int sampleRate,
				     unsigned int blockSize,
				     unsigned int channels)
{
    const DSSI_Descriptor *descriptor = getDSSIDescriptor(identifier);

    if (descriptor) {

	DSSIPluginInstance *instance =
	    new DSSIPluginInstance
	    (this, instrument, identifier, position, sampleRate, blockSize, channels,
	     descriptor);

	m_instances.insert(instance);

	return instance;
    }

    return 0;
}

const DSSI_Descriptor *
DSSIPluginFactory::getDSSIDescriptor(QString identifier)
{
    QString type, soname, label;
    PluginIdentifier::parseIdentifier(identifier, type, soname, label);

    if (soname == PluginIdentifier::BUILTIN_PLUGIN_SONAME) {
	if (label == "sample_player") {
	    const DSSI_Descriptor *descriptor = SamplePlayer::getDescriptor(0);
	    if (descriptor) {
		descriptor->receive_host_descriptor(&m_hostDescriptor);
	    }
	    return descriptor;
	} else {
	    return 0;
	}
    }
    
    bool firstInLibrary = false;

    if (m_libraryHandles.find(soname) == m_libraryHandles.end()) {
	loadLibrary(soname);
	if (m_libraryHandles.find(soname) == m_libraryHandles.end()) {
	    std::cerr << "WARNING: DSSIPluginFactory::getDSSIDescriptor: loadLibrary failed for " << soname.toStdString() << std::endl;
	    return 0;
	}
	firstInLibrary = true;
    }

    void *libraryHandle = m_libraryHandles[soname];

    DSSI_Descriptor_Function fn = (DSSI_Descriptor_Function)
	DLSYM(libraryHandle, "dssi_descriptor");

    if (!fn) {
	std::cerr << "WARNING: DSSIPluginFactory::getDSSIDescriptor: No descriptor function in library " << soname.toStdString() << std::endl;
	return 0;
    }

    const DSSI_Descriptor *descriptor = 0;
    
    int index = 0;
    while ((descriptor = fn(index))) {
	if (descriptor->LADSPA_Plugin->Label == label) {
	    if (firstInLibrary && (descriptor->DSSI_API_Version >= 2)) {
		descriptor->receive_host_descriptor(&m_hostDescriptor);
	    }
	    return descriptor;
	}
	++index;
    }

    std::cerr << "WARNING: DSSIPluginFactory::getDSSIDescriptor: No such plugin as " << label.toStdString() << " in library " << soname.toStdString() << std::endl;

    return 0;
}

const LADSPA_Descriptor *
DSSIPluginFactory::getLADSPADescriptor(QString identifier)
{
    const DSSI_Descriptor *dssiDescriptor = getDSSIDescriptor(identifier);
    if (dssiDescriptor) return dssiDescriptor->LADSPA_Plugin;
    else return 0;
}


std::vector<QString>
DSSIPluginFactory::getPluginPath()
{
    std::vector<QString> pathList;
    std::string path;

    char *cpath = getenv("DSSI_PATH");
    if (cpath) path = cpath;

    if (path == "") {
	path = "/usr/local/lib/dssi:/usr/lib/dssi";
	char *home = getenv("HOME");
	if (home) path = std::string(home) + "/.dssi:" + path;
    }

    std::string::size_type index = 0, newindex = 0;

    while ((newindex = path.find(':', index)) < path.size()) {
	pathList.push_back(path.substr(index, newindex - index).c_str());
	index = newindex + 1;
    }
    
    pathList.push_back(path.substr(index).c_str());

    return pathList;
}


#ifdef HAVE_LIBLRDF
std::vector<QString>
DSSIPluginFactory::getLRDFPath(QString &baseUri)
{
    std::vector<QString> pathList = getPluginPath();
    std::vector<QString> lrdfPaths;

    lrdfPaths.push_back("/usr/local/share/dssi/rdf");
    lrdfPaths.push_back("/usr/share/dssi/rdf");

    lrdfPaths.push_back("/usr/local/share/ladspa/rdf");
    lrdfPaths.push_back("/usr/share/ladspa/rdf");

    for (std::vector<QString>::iterator i = pathList.begin();
	 i != pathList.end(); ++i) {
	lrdfPaths.push_back(*i + "/rdf");
    }

#ifdef DSSI_BASE
    baseUri = DSSI_BASE;
#else
    baseUri = "http://dssi.sourceforge.net/ontology#";
#endif

    return lrdfPaths;
}    
#endif


void
DSSIPluginFactory::discoverPlugins(QString soname)
{
    // Note that soname is expected to be a full path at this point,
    // of a file that is known to exist

    void *libraryHandle = DLOPEN(soname, RTLD_LAZY);

    if (!libraryHandle) {
        std::cerr << "WARNING: DSSIPluginFactory::discoverPlugins: couldn't load plugin library "
                  << soname.toStdString() << " - " << DLERROR() << std::endl;
        return;
    }

    DSSI_Descriptor_Function fn = (DSSI_Descriptor_Function)
	DLSYM(libraryHandle, "dssi_descriptor");

    if (!fn) {
	std::cerr << "WARNING: DSSIPluginFactory::discoverPlugins: No descriptor function in " << soname.toStdString() << std::endl;
	return;
    }

    const DSSI_Descriptor *descriptor = 0;
    
    int index = 0;
    while ((descriptor = fn(index))) {

	const LADSPA_Descriptor *ladspaDescriptor = descriptor->LADSPA_Plugin;
	if (!ladspaDescriptor) {
	    std::cerr << "WARNING: DSSIPluginFactory::discoverPlugins: No LADSPA descriptor for plugin " << index << " in " << soname.toStdString() << std::endl;
	    ++index;
	    continue;
	}

#ifdef HAVE_LIBLRDF
	char *def_uri = 0;
	lrdf_defaults *defs = 0;
		
	QString category = m_taxonomy[ladspaDescriptor->UniqueID];

	if (category == "" && ladspaDescriptor->Name != 0) {
	    std::string name = ladspaDescriptor->Name;
	    if (name.length() > 4 &&
		name.substr(name.length() - 4) == " VST") {
		if (descriptor->run_synth || descriptor->run_multiple_synths) {
		    category = "VST instruments";
		} else {
		    category = "VST effects";
		}
		m_taxonomy[ladspaDescriptor->UniqueID] = category;
	    }
	}
	
//	std::cerr << "Plugin id is " << ladspaDescriptor->UniqueID
//		  << ", category is \"" << (category ? category : QString("(none)"))
//		  << "\", name is " << ladspaDescriptor->Name
//		  << ", label is " << ladspaDescriptor->Label
//		  << std::endl;
	
	def_uri = lrdf_get_default_uri(ladspaDescriptor->UniqueID);
	if (def_uri) {
	    defs = lrdf_get_setting_values(def_uri);
	}
	
	int controlPortNumber = 1;
	
	for (unsigned long i = 0; i < ladspaDescriptor->PortCount; i++) {
	    
	    if (LADSPA_IS_PORT_CONTROL(ladspaDescriptor->PortDescriptors[i])) {
		
		if (def_uri && defs) {
		    
		    for (int j = 0; j < defs->count; j++) {
			if (defs->items[j].pid == controlPortNumber) {
//			    std::cerr << "Default for this port (" << defs->items[j].pid << ", " << defs->items[j].label << ") is " << defs->items[j].value << "; applying this to port number " << i << " with name " << ladspaDescriptor->PortNames[i] << std::endl;
			    m_portDefaults[ladspaDescriptor->UniqueID][i] =
				defs->items[j].value;
			}
		    }
		}
		
		++controlPortNumber;
	    }
	}
#endif // HAVE_LIBLRDF

	QString identifier = PluginIdentifier::createIdentifier
	    ("dssi", soname, ladspaDescriptor->Label);
	m_identifiers.push_back(identifier);

	++index;
    }

    if (DLCLOSE(libraryHandle) != 0) {
        std::cerr << "WARNING: DSSIPluginFactory::discoverPlugins - can't unload " << libraryHandle << std::endl;
        return;
    }
}

    

