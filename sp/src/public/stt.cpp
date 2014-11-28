
#include "stt.h"

#include <jsoncpp/json/json.h>

CWavRecordingJob::CWavRecordingJob( const WavRecordingParams_t &params )
    : CFunctorJob( NULL, "" ), m_params( params )
{
    SetOnDone( params.m_onDone );
}


void CWavRecordingJob::OnWavReceived( HttpRequestResults_t &data ) {
    WavRecordingResults_t results;
    results.m_failure = data.m_failure;
    results.m_failureReason = *data.m_failureReason;
    results.m_wavBuffer = data.m_outputBuffer;

    CallOnDone( results );
}

JobStatus_t CWavRecordingJob::DoExecute( void ) {
    CSoundRecorder *rec = GetSoundRecorder();
    while ( rec->IsRecording() ) {
        ThreadSleep( 100 );
    }

    HttpRequestParams_t params;
        
    params.m_requestType = HTTP_GET;
    params.m_url = "http://localhost:19998/get";
        
    params.m_inputOpMethod = HTTP_ME_BUFFER;

    params.m_outputOpMethod = HTTP_ME_BUFFER;
    params.m_onDone = boost::bind( 
        &CWavRecordingJob::OnWavReceived, this, _1 );
        
    g_pThreadPool->AddJob( new CHttpRequestJob( params ) );

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
        DevMsg( "got %d bytes in wav-buffer\n", data.m_wavBuffer->TellPut() );
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
