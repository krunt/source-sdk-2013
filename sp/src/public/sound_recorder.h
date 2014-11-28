#if !defined( IENGINESNDRECORDER_H )
#define IENGINESNDRECORDER_H

#ifdef _WIN32
#pragma once
#endif

#include "tier1/tier1.h"
#include "tier1/refcount.h"
#include "tier1/utlbuffer.h"
#include "tier0/threadtools.h"

#include <portaudio.h>

#include "http.h"

/* mono for now, 16-bit */

struct CSoundRecordParams_t {
    int m_sampleRate; // hz
    int m_maxDuration; // seconds
    int m_autoStop; // if 1 second is lower than |A|/32
};

class CSoundRecorder {
public:
    CSoundRecorder( const CSoundRecordParams_t &params );
    ~CSoundRecorder( void );

    void Release( void );

    bool StartRecording( void );
    bool StopRecording( void ); // thread safe
    bool IsRecording( void ) const { return m_recording; } // thread safe

    CUtlBuffer &GetWavBuffer( void );

    int OnRecordingCallback( const short *input, int sampleCount );

private:
    void OnWavStart( const HttpRequestResults_t &r );
    void OnWavStop( const HttpRequestResults_t &r );

    bool NeedTerminate( void ) const;
    void BuildWavBuffer( void );

    bool m_inError;

    CInterlockedIntT<int> m_recording;
    CSoundRecordParams_t m_params;

    CInterlockedIntT<int> m_toStop;

    int m_samplesPos;
    CUtlMemory<short> m_samples;

    bool m_wavBuilt;
    CUtlBuffer m_wavBuffer;

    PaStream *m_stream;
};

extern CSoundRecorder *GetSoundRecorder( void );

#endif // IENGINESNDRECORDER_H
