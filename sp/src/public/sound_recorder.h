#if !defined( IENGINESNDRECORDER_H )
#define IENGINESNDRECORDER_H

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

    bool StartRecording( void );
    bool StopRecording( void );
    bool IsRecording( void ) const { return m_recording; }

    CUtlBuffer &GetWavBuffer( void );

private:
    bool NeedTerminate( void ) const;
    int OnRecordingCallback( const unsigned short *input, int sampleCount );
    void BuildWavBuffer( void );

    bool m_inError;

    bool m_recording;
    CSoundRecordParams_t m_params;

    int m_samplesPos;
    CUtlMemory<unsigned short> m_samples;

    bool m_wavBuilt;
    CUtlBuffer m_wavBuffer;

    PaStream *m_stream;
};

extern CSoundRecorder *g_soundRecorder;

#endif // IENGINESNDRECORDER_H
