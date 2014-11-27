
#include "tts.h"

#include <jsoncpp/json/json.h>

#include <boost/bind.hpp>

CTextToSpeechJob::CTextToSpeechJob( const TextToSpeechParams_t &params )
    : CFunctorJob( NULL, "" ), m_params( params ), m_state( TTS_ST_AUTH )
{
    m_failure = 0;
    m_failureReason = "";

    SetOnDone( params.m_onDone );
}


void CTextToSpeechJob::OnWavReceived( HttpRequestResults_t &data ) {
    if ( data.m_failure ) {
        m_failure = data.m_failure;
        m_failureReason = *data.m_failureReason;

        m_state = TTS_ST_DONE;

    } else { 
        // success
        m_state = TTS_ST_CONV;
        m_wavBuffer.Swap( *data.m_outputBuffer );
    }

    DoExecute();
}

void CTextToSpeechJob::OnWavConvReceived( HttpRequestResults_t &data ) {
    if ( data.m_failure ) {
        m_failure = data.m_failure;
        m_failureReason = *data.m_failureReason;
    } else {
        m_wavBuffer.Swap( *(data.m_outputBuffer) );
    }

    m_state = TTS_ST_DONE;

    DoExecute();
}

JobStatus_t CTextToSpeechJob::DoExecute( void ) {

    switch ( m_state ) {

    case TTS_ST_AUTH:
        ThinkOnAuth();
        break;

    case TTS_ST_WAV:
        ThinkOnWav();
        break;

    case TTS_ST_CONV:
        ThinkOnConv();
        break;

    case TTS_ST_DONE: {
        TextToSpeechResults_t results;
        results.m_failure = m_failure;
        results.m_failureReason = m_failureReason;

        if ( !m_failure ) {
            results.m_wavBuffer = &m_wavBuffer;
        }

        CallOnDone( results );
        break;
    }};

    return JOB_OK;
}

void CTextToSpeechJob::OnAuthReceived( HttpRequestResults_t &data ) {
    m_state = TTS_ST_WAV;
    DoExecute();
}

void CTextToSpeechJob::ThinkOnAuth( void ) {
    m_state = TTS_ST_WAV;
    DoExecute();
}

void CTextToSpeechJob::ThinkOnWav( void ) {
    m_state = TTS_ST_CONV;
    DoExecute();
}

void CTextToSpeechJob::ThinkOnConv( void ) {
    HttpRequestParams_t params;

    params.m_requestType = HTTP_POST;
    params.m_url = m_params.m_convUrl;

    params.m_headers.AddToTail( "Content-Type: audio/wav" );

    params.m_inputOpMethod = HTTP_ME_BUFFER;
    params.m_inputBuffer.Swap( m_wavBuffer );

    if ( m_params.m_absPath.IsEmpty() ) {
        params.m_outputOpMethod = HTTP_ME_BUFFER;
    } else {
        params.m_outputOpMethod = HTTP_ME_FILE;
        params.m_outputFile = m_params.m_absPath;
    }

    params.m_onDone 
        = boost::bind( &CTextToSpeechJob::OnWavConvReceived, this, _1 );
    g_pThreadPool->AddJob( new CHttpRequestJob( params ) );
}

