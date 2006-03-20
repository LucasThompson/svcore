/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

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

#ifndef _DSSIPLUGININSTANCE_H_
#define _DSSIPLUGININSTANCE_H_

#define DSSI_API_LEVEL 2

#include <vector>
#include <set>
#include <map>
#include <QString>
#include <QMutex>
#include <QThread>

#include "api/dssi.h"

#include "base/RingBuffer.h"
#include "RealTimePluginInstance.h"
#include "base/Scavenger.h"

class DSSIPluginInstance : public RealTimePluginInstance
{
public:
    virtual ~DSSIPluginInstance();

    virtual bool isOK() const { return m_instanceHandle != 0; }

    int getClientId() const { return m_client; }
    virtual QString getIdentifier() const { return m_identifier; }
    int getPosition() const { return m_position; }

    virtual void run(const RealTime &);

    virtual unsigned int getParameterCount() const;
    virtual void setParameterValue(unsigned int parameter, float value);
    virtual float getParameterValue(unsigned int parameter) const;
    virtual float getParameterDefault(unsigned int parameter) const;
    virtual QString configure(QString key, QString value);
    virtual void sendEvent(const RealTime &eventTime,
			   const void *event);
    virtual void clearEvents();

    virtual size_t getBufferSize() const { return m_blockSize; }
    virtual size_t getAudioInputCount() const { return m_audioPortsIn.size(); }
    virtual size_t getAudioOutputCount() const { return m_idealChannelCount; }
    virtual sample_t **getAudioInputBuffers() { return m_inputBuffers; }
    virtual sample_t **getAudioOutputBuffers() { return m_outputBuffers; }

    virtual QStringList getPrograms() const;
    virtual QString getCurrentProgram() const;
    virtual QString getProgram(int bank, int program) const;
    virtual unsigned long getProgram(QString name) const;
    virtual void selectProgram(QString program);

    virtual bool isBypassed() const { return m_bypassed; }
    virtual void setBypassed(bool bypassed) { m_bypassed = bypassed; }

    virtual size_t getLatency();

    virtual void silence();
    virtual void discardEvents();
    virtual void setIdealChannelCount(size_t channels); // may re-instantiate

    virtual bool isInGroup() const { return m_grouped; }
    virtual void detachFromGroup();

protected:
    // To be constructed only by DSSIPluginFactory
    friend class DSSIPluginFactory;

    // Constructor that creates the buffers internally
    // 
    DSSIPluginInstance(RealTimePluginFactory *factory,
		       int client,
		       QString identifier,
		       int position,
		       unsigned long sampleRate,
		       size_t blockSize,
		       int idealChannelCount,
		       const DSSI_Descriptor* descriptor);
    
    // Constructor that uses shared buffers
    // 
    DSSIPluginInstance(RealTimePluginFactory *factory,
		       int client,
		       QString identifier,
		       int position,
		       unsigned long sampleRate,
		       size_t blockSize,
		       sample_t **inputBuffers,
		       sample_t **outputBuffers,
		       const DSSI_Descriptor* descriptor);

    void init();
    void instantiate(unsigned long sampleRate);
    void cleanup();
    void activate();
    void deactivate();
    void connectPorts();

    bool handleController(snd_seq_event_t *ev);
    void setPortValueFromController(unsigned int portNumber, int controlValue);
    void selectProgramAux(QString program, bool backupPortValues);
    void checkProgramCache() const;

    void initialiseGroupMembership();
    void runGrouped(const RealTime &);

    // For use in DSSIPluginFactory (set in the DSSI_Host_Descriptor):
    static int requestMidiSend(LADSPA_Handle instance,
			       unsigned char ports,
			       unsigned char channels);
    static void midiSend(LADSPA_Handle instance,
			 snd_seq_event_t *events,
			 unsigned long eventCount);
    static int requestNonRTThread(LADSPA_Handle instance,
				  void (*runFunction)(LADSPA_Handle));

    int                        m_client;
    int                        m_position;
    LADSPA_Handle              m_instanceHandle;
    const DSSI_Descriptor     *m_descriptor;

    std::vector<std::pair<unsigned long, LADSPA_Data*> > m_controlPortsIn;
    std::vector<std::pair<unsigned long, LADSPA_Data*> > m_controlPortsOut;

    std::vector<LADSPA_Data>  m_backupControlPortsIn;

    std::map<int, int>        m_controllerMap;

    std::vector<int>          m_audioPortsIn;
    std::vector<int>          m_audioPortsOut;

    struct ProgramControl {
	int msb;
	int lsb;
	int program;
    };
    ProgramControl m_pending;

    struct ProgramDescriptor {
	int bank;
	int program;
	QString name;
    };
    mutable std::vector<ProgramDescriptor> m_cachedPrograms;
    mutable bool m_programCacheValid;

    RingBuffer<snd_seq_event_t> m_eventBuffer;

    size_t                    m_blockSize;
    sample_t                **m_inputBuffers;
    sample_t                **m_outputBuffers;
    bool                      m_ownBuffers;
    size_t                    m_idealChannelCount;
    size_t                    m_outputBufferCount;
    size_t                    m_sampleRate;
    float                    *m_latencyPort;
    bool                      m_run;
    
    bool                      m_bypassed;
    QString                   m_program;
    bool                      m_grouped;
    RealTime                  m_lastRunTime;

    RealTime                  m_lastEventSendTime;
    bool                      m_haveLastEventSendTime;

    QMutex                    m_processLock;

    typedef std::set<DSSIPluginInstance *> PluginSet;
    typedef std::map<QString, PluginSet> GroupMap;
    static GroupMap m_groupMap;
    static snd_seq_event_t **m_groupLocalEventBuffers;
    static size_t m_groupLocalEventBufferCount;

    static Scavenger<ScavengerArrayWrapper<snd_seq_event_t *> > m_bufferScavenger;

    class NonRTPluginThread : public QThread
    {
    public:
	NonRTPluginThread(LADSPA_Handle handle,
			  void (*runFunction)(LADSPA_Handle)) :
	    m_handle(handle),
	    m_runFunction(runFunction),
	    m_exiting(false) { }

	virtual void run();
	void setExiting() { m_exiting = true; }

    protected:
	LADSPA_Handle m_handle;
	void (*m_runFunction)(LADSPA_Handle);
	bool m_exiting;
    };
    static std::map<LADSPA_Handle, std::set<NonRTPluginThread *> > m_threads;
};

#endif // _DSSIPLUGININSTANCE_H_

