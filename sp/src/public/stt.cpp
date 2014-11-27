
#include "stt.h"

#include <jsoncpp/json/json.h>

CWavRecordingJob::CWavRecordingJob( const WavRecordingParams_t &params )
    : CFunctorJob( NULL, "" ), m_params( params )
{
    SetOnDone( params.m_onDone );
}

JobStatus_t CWavRecordingJob::DoExecute( void ) {
    CSoundRecorder *rec = GetSoundRecorder();
    while ( rec->IsRecording() ) {
        ThreadSleep( 100 );
    }

    WavRecordingResults_t results;
    results.m_failure = 0;
    results.m_failureReason = "";
    results.m_wavBuffer = &rec->GetWavBuffer();

    /*
    fwrite( results.m_wavBuffer->Base(),
        1, results.m_wavBuffer->TellPut(), stderr );
        */

    CallOnDone( results );

    return JOB_OK;
}

CSpeechToTextJob::CSpeechToTextJob( const SpeechToTextParams_t &params )
    : CFunctorJob( NULL, "" ), m_params( params ), m_state( STT_ST_WAV )
{
    m_failure = 0;
    m_failureReason = "";

    SetOnDone( params.m_onDone );
}

void CSpeechToTextJob::OnWavReceived( WavRecordingResults_t &data ) {
    if ( data.m_failure ) {
        m_failure = 1;
        m_failureReason = "wav-fetch failured";
        m_state = STT_ST_DONE;
    } else {
        m_wavBuffer.Put( data.m_wavBuffer->Base(), data.m_wavBuffer->TellPut() );
        m_state = STT_ST_AUTH;
    }

    DoExecute();
}

JobStatus_t CSpeechToTextJob::DoExecute( void ) {
    switch ( m_state ) {

    case STT_ST_WAV: {
        ThinkOnWav();
        break;
    }

    case STT_ST_AUTH: {
        ThinkOnAuth();
        break;
    }

    case STT_ST_TEXT: {
        ThinkOnText();
        break;
    }

    case STT_ST_DONE: {
        SpeechToTextResults_t results;
        results.m_failure = m_failure;
        results.m_failureReason = m_failureReason;

        if ( !m_failure ) {
            results.m_text = m_text;
        }

        CallOnDone( results );
        break;
    }};

    return JOB_OK;
}

void CSpeechToTextJob::ThinkOnWav( void ) {
    WavRecordingParams_t params;
    params.m_onDone = boost::bind( &CSpeechToTextJob::OnWavReceived, this, _1 );
    g_pThreadPool->AddJob( new CWavRecordingJob( params ) );
}

void CSpeechToTextJob::ThinkOnAuth( void ) {
    m_state = STT_ST_TEXT;
    DoExecute();
}
void CSpeechToTextJob::ThinkOnText( void ) {
    m_state = STT_ST_DONE;
    DoExecute();
}

void CSpeechToTextJob::OnAuthReceived( HttpRequestResults_t &data ) {
    m_state = STT_ST_TEXT;
    DoExecute();
}
void CSpeechToTextJob::OnTextReceived( HttpRequestResults_t &data ) {
    m_state = STT_ST_DONE;
    DoExecute();
}
