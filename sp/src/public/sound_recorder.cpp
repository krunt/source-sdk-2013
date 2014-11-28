
#include "sound_recorder.h"

#include "inmemoryio.h"

#undef min
#undef max
#include <algorithm> 
#include <boost/bind.hpp>

CSoundRecorder::CSoundRecorder( const CSoundRecordParams_t &params )
    : m_params( params ), m_recording( 0 ), m_toStop( 0 ), m_samplesPos( 0 ), 
        m_wavBuilt( false )
{
    int numSamples = m_params.m_sampleRate * m_params.m_maxDuration;
    m_samples.Grow( numSamples );

    m_inError = false;
    PaError err = Pa_Initialize();
    if ( err != paNoError ) {
        DevWarning( "Pa_Initialize() failed:\n" );
        m_inError = true;
    }
}

CSoundRecorder::~CSoundRecorder( void ) {
    if ( !m_inError ) {
        Pa_Terminate();
    }
}

void CSoundRecorder::Release( void ) {
    if ( !m_inError ) {
        Pa_Terminate();
    }
}

static int PaRecordCallback( 
        const void *input, void *output,
        unsigned long frameCount,
        const PaStreamCallbackTimeInfo *timeInfo,
        PaStreamCallbackFlags statusFlags,
        void *userData )
{
    CSoundRecorder *rec = (CSoundRecorder *)userData;
    return rec->OnRecordingCallback( static_cast<const short *>( input ),
        frameCount );
}

bool CSoundRecorder::NeedTerminate( void ) const {
    if ( m_toStop ) { 
        return true;
    }

    bool atEnd = m_samplesPos >= m_samples.Count();

    if ( !atEnd ) {
        const short kStopLimit = ( 1<<16 ) / 32;
        int testCount = m_params.m_sampleRate; /* one second */

        if ( m_params.m_autoStop && m_samplesPos > testCount )  {
            bool toStop = true;
            for ( int i = 0; i < testCount; ++i ) {
                if ( m_samples[ m_samplesPos - i - 1 ] > kStopLimit ) {
                    toStop = false;
                    break;
                }
            }

            if ( toStop ) {
                atEnd = true;
            }
        }
    }

    return atEnd;
}

int CSoundRecorder::OnRecordingCallback( const short *input, 
        int inSampleCount )
{
    int samplesToCopy = std::max( 0, std::min( inSampleCount, 
        m_samples.Count() - m_samplesPos ) );

    if ( samplesToCopy ) {
        memcpy( m_samples.Base() + m_samplesPos * sizeof( short ), input,
            samplesToCopy * sizeof( short ) );
    }

    m_samplesPos += samplesToCopy;

    if ( NeedTerminate() ) {
        m_recording = 0;
        m_wavBuilt = false;

        return paComplete;
    }

    return paContinue;
}

bool CSoundRecorder::StartRecording( void ) {
    m_recording = 1;

    HttpRequestParams_t params;
        
    params.m_requestType = HTTP_GET;
    params.m_url = "http://localhost:19998/start";
        
    params.m_inputOpMethod = HTTP_ME_BUFFER;

    params.m_outputOpMethod = HTTP_ME_BUFFER;
    params.m_onDone = boost::bind( 
        &CSoundRecorder::OnWavStart, this, _1 );
        
    g_pThreadPool->AddJob( new CHttpRequestJob( params ) );

    return true;
}

void CSoundRecorder::OnWavStart( const HttpRequestResults_t &r ) {
    if ( r.m_failure && r.m_failure != 52 ) {
        DevWarning( "failed wav-start request: %d `%s'\n", 
                r.m_failure, r.m_failureReason->Get() );
        m_recording = 0;
    }
}

void CSoundRecorder::OnWavStop( const HttpRequestResults_t &r ) {
    if ( r.m_failure && r.m_failure != 52 ) {
        DevWarning( "failed wav-stop request: %d `%s'\n", 
                r.m_failure, r.m_failureReason->Get() );
    }

    m_recording = 0;
}

bool CSoundRecorder::StopRecording( void ) {
    HttpRequestParams_t params;
        
    params.m_requestType = HTTP_GET;
    params.m_url = "http://localhost:19998/stop";
        
    params.m_inputOpMethod = HTTP_ME_BUFFER;

    params.m_outputOpMethod = HTTP_ME_BUFFER;
    params.m_onDone = boost::bind( 
        &CSoundRecorder::OnWavStop, this, _1 );
        
    g_pThreadPool->AddJob( new CHttpRequestJob( params ) );

    return true;
}

typedef struct {
        short wFormatTag;        // Integer identifier of the format
        short nChannels;         // Number of audio channels
        int nSamplesPerSec;   // Audio sample rate
        int nAvgBytesPerSec;  // Bytes per second (possibly approximate)
        short nBlockAlign;       // Size in bytes of a sample block (all channels)
        short wBitsPerSample;    // Size in bits of a single per-channel sample
        short cbSize;            // Bytes of extra data appended to this struct
    } WAVEFORMATEX;

void CSoundRecorder::BuildWavBuffer( void ) {
    InMemoryFileWriteBinary inmem;

    {
        OutFileRIFF rf( "stub", inmem );
        IterateOutputRIFF outRiff( rf );
    
    	WAVEFORMATEX format;
    	format.wFormatTag = WAVE_FORMAT_PCM;
    	format.nChannels = 1;
    	format.nSamplesPerSec = m_params.m_sampleRate;
    	format.nAvgBytesPerSec = m_params.m_sampleRate * sizeof(short);
    	format.nBlockAlign = 2;
    	format.wBitsPerSample = 16;
    	format.cbSize = sizeof( format );

        // fmt-header 
        {
            outRiff.ChunkStart( WAVE_FMT );
            outRiff.ChunkWriteData( &format.wFormatTag, 2 );
            outRiff.ChunkWriteData( &format.nChannels, 2 );
            outRiff.ChunkWriteData( &format.nSamplesPerSec, 4 );
            outRiff.ChunkWriteData( &format.nAvgBytesPerSec, 4 );
            outRiff.ChunkWriteData( &format.nBlockAlign, 2 );
            outRiff.ChunkWriteData( &format.wBitsPerSample, 2 );
            outRiff.ChunkFinish();
        }
    
        // data-content
        {
            outRiff.ChunkStart( WAVE_DATA );
            for ( int i = 0; i < m_samplesPos; ++i ) {
                outRiff.ChunkWriteData( &m_samples[i], sizeof( short ) );
            }
            outRiff.ChunkFinish();
        }
    }

    m_wavBuffer.Swap( inmem.GetBuffer() );
    m_wavBuffer.SeekPut( CUtlBuffer::SEEK_TAIL, 0 );

    m_wavBuilt = true;

    DevMsg( "BuildWavBuffer(): built wav-buffer with %d samples\n", m_samplesPos );
}

CUtlBuffer &CSoundRecorder::GetWavBuffer( void ) {
    if ( !m_wavBuilt ) {
        BuildWavBuffer();
    }
    return m_wavBuffer;
}

static CRefPtr<CSoundRecorder> g_soundRecorder;

extern CSoundRecorder *GetSoundRecorder( void ) {
    if ( !g_soundRecorder ) {
        CSoundRecordParams_t params;
        params.m_sampleRate = 16000;
        //params.m_sampleRate = 44100;
        params.m_maxDuration = 10;
        params.m_autoStop = 0;

        g_soundRecorder = new CSoundRecorder( params );
    }
    return g_soundRecorder;
}
