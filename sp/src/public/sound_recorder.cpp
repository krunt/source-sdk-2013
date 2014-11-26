
#include "sound_recorder.h"

static CSoundRecorder soundRecorder;
g_soundRecorder = &soundRecorder;

CSoundRecorder::CSoundRecorder( const CSoundRecordParams_t &params )
    : m_params( params ), m_recording( false ), m_samplesPos( 0 ), 
        m_wavBuilt( false )
{
    int numBytes = m_params.m_format 
        * m_params.m_sampleRate * m_params.m_maxDuration;
    m_samples.Grow( numBytes );

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

static int PaRecordCallback( 
        const void *input, void *output,
        unsigned long frameCount,
        const PaStreamCallbackTimeInfo *timeInfo,
        PaStreamCallbackFlags statusFlags,
        void *userData )
{
    CSoundRecorder *rec = (CSoundRecord *)userData;
    return rec->OnRecordingCallback( static_cast<unsigned short *>( input ),
        frameCount );
}

bool CSoundRecorder::NeedTerminate( void ) const {
    bool atEnd = m_samplesPos == m_samples.Count();

    if ( !atEnd ) {
        const unsigned short kStopLimit = ( 1<<16 ) / 32;
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

int CSoundRecorder::OnRecordingCallback( const unsigned short *input, 
        int inSampleCount )
{
    int samplesToCopy = std::max( 0, std::min( inSampleCount, 
        m_samples.Count() - m_samplesPos ) );

    if ( samplesToCopy ) {
        memcpy( m_samples.Base(), input,
            samplesToCopy * sizeof( unsigned short ) );
    }

    m_samplesPos += samplesToCopy;

    if ( NeedTerminate() ) {
        m_recording = false;
        m_wavBuilt = false;

        return paComplete;
    }

    return paContinue;
}

bool CSoundRecorder::StartRecording( void ) {
    if ( m_inError ) {
        DevWarning( "StartRecording() can't execute (failed state)\n" );
        return false;
    }

    if ( IsRecording() ) {
        return false;
    }

    PaStreamParameters  params;
    params.device = Pa_GetDefaultInputDevice();

    if ( params.device == paNoDevice ) {
        DevWarning( "StartRecording(): Pa_GetDefaultInputDevice no default d-ce" );
        return false;
    }

    params.channelCount = 1;
    params.sampleFormat = paInt16;
    params.suggestedLatency 
        = Pa_GetDeviceInfo( params.device )->defaultLowInputLatency;
    params.hostApiSpecificStreamInfo = NULL;

    PaError err = Pa_OpenStream(
              &m_stream,
              &params,
              NULL,
              m_params.m_sampleRate,
              512, // frames per buffer
              paClipOff,
              &PaRecordCallback,
              this );
    if( err != paNoError ) {
        DevWarning( "Pa_OpenStream(): failed open sound stream\n" );
        return false;
    }

    err = Pa_StartStream( &m_stream );
    if( err != paNoError ) {
        DevWarning( "Pa_StartStream(): failed start sound stream\n" );
        return false;
    }

    m_samplesPos = 0;
    m_wavBuffer.Clear();
    m_recording = true;
    m_wavBuilt = false;

    return true;
}

bool CSoundRecorder::StopRecording( void ) {
    if ( m_inError ) {
        DevWarning( "StopRecording() can't execute (failed state)\n" );
        return false;
    }

    if ( !IsRecording() ) {
        return false;
    }

    PaError err = Pa_CloseStream( m_stream );
    if ( err != paNoError ) {
        DevWarning( "Pa_CloseStream() failed\n" );
    }

    m_recording = false;
    BuildWavBuffer();

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
    OutFileRIFF rf( "stub", inmem );
    IterateOutputRIFF outRiff( rf );

	WAVEFORMATEX format;
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = 1;
	format.nSamplesPerSec = m_params.m_sampleRate;
	format.nAvgBytesPerSec = m_params.m_sampleRate * sizeof(unsigned short);
	format.nBlockAlign = 2;
	format.wBitsPerSample = 16;
	format.cbSize = sizeof( format );
	store.ChunkWrite( WAVE_FMT, &format, sizeof( format ) );

    store.ChunkStart( WAVE_DATA );
    for ( int i = 0; i < m_nsamplesPos; ++i ) {
        store.ChunkWriteData( &m_samples[i], sizeof( unsigned short ) );
    }
    store.ChunkFinish();

    m_wavBuffer.Swap( inmem.GetBuffer() );

    m_wavBuilt = true;
}

CUtlBuffer &CSoundRecorder::GetWavBuffer( void ) {
    if ( !m_wavBuilt ) {
        BuildWavBuffer();
    }
    return m_wavBuffer;
}

