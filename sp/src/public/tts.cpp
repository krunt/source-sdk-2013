
#include "tts.h"

TextToSpeechParams_t::TextToSpeechParams_t() 
    : m_onDone( NULL )
{}

CTextToSpeechJob::CTextToSpeechJob( const  &params )
    : CFunctorJob( NULL, "" ), m_params( params ), m_state( TTS_ST_AUTH )
{
    SetOnDone( params.m_onDone );
}

void CTextToSpeechJob::OnAuthReceived( HttpRequestResults_t *data ) {
    Json::Value args;
    Json::Reader jsonReader;

    if ( data->m_failure || !jsonReader.parse( data->m_outputBuffer->Base(),
            data->m_outputBuffer->Base() + data->m_outputBuffer->TellPut() ) 
            || !args.isMember( "access_token" ) )
    {
        m_state = TTS_ST_DONE;
        if ( !data->m_failure ) {
            m_failure = 1;
            m_failureReason = "invalid json received";
        }

    } else { 
        // success
        m_accessToken = args[ "access_token" ].asCString();
        m_state = TTS_ST_WAV;
    }

    DoExecute();
}

void CTextToSpeechJob::OnWavReceived( HttpRequestResults_t *data ) {
    if ( data->m_failure ) {
        m_state = TTS_ST_DONE;

    } else { 
        // success
        m_state = TTS_ST_CONV;
        m_wavBuffer.Swap( *data->m_outputBuffer );
    }

    DoExecute();
}

void CTextToSpeechJob::OnWavConvReceived( HttpRequestResults_t *data ) {
    if ( !data->m_failure ) {
        m_wavBuffer.Swap( *data->m_outputBuffer );
    }

    m_state = TTS_ST_DONE;

    DoExecute();
}

JobStatus_t CTextToSpeechJob::DoExecute( void ) {

    switch ( m_state ) {

    case TTS_ST_AUTH: {
        HTTPRequestsParams_t params;

        params.m_requestType = HTTP_POST;
        params.m_url = m_params.m_ttsUrl;

        params.m_headers.AddToTail(  
            "Content-Type: application/x-www-form-urlencoded" );
        params.m_headers.AddToTail( "Accept: application/json" );

        params.m_inputOpMethod = HTTP_ME_BUFFER;
        params.m_inputBuffer = "client_id=" + m_params.m_appKey
            + "&client_secret=" + m_params.m_appSecret
            + "&scope=TTS&grant_type=client_credentials";

        params.m_outputOpMethod = HTTP_ME_BUFFER;
        params.m_onDone = CreateFunctor( this, CTextToSpeechJob::OnAuthReceived );

        g_pThreadPool->AddJob( new CHttpRequestJob( params ) );
        break;
    }

    case TTS_ST_WAV: {
        HTTPRequestsParams_t params;

        params.m_requestType = HTTP_POST;
        params.m_url = m_params.m_ttsUrl;

        params.m_headers.AddToTail( "Authorization: Bearer " + m_accessToken );
        params.m_headers.AddToTail( "Accept: audio/amr" );
        params.m_headers.AddToTail( "Content-Type: text/plain" );

        params.m_inputOpMethod = HTTP_ME_BUFFER;
        params.m_inputBuffer = m_params.m_text;

        params.m_outputOpMethod = HTTP_ME_BUFFER;
        params.m_onDone = CreateFunctor( this, CTextToSpeechJob::OnWavReceived );

        g_pThreadPool->AddJob( new CHttpRequestJob( params ) );
        break;
    }

    case TTS_ST_CONV: {
        HTTPRequestsParams_t params;

        params.m_requestType = HTTP_POST;
        params.m_url = m_params.m_convUrl;

        params.m_inputOpMethod = HTTP_ME_BUFFER;
        params.m_inputBuffer.Swap( m_wavBuffer );

        params.m_outputOpMethod = HTTP_ME_BUFFER;
        params.m_onDone = CreateFunctor( this, 
            CTextToSpeechJob::OnWavConvReceived );

        g_pThreadPool->AddJob( new CHttpRequestJob( params ) );
        break;
    }

    case TTS_ST_DONE: {
        TextToSpeechResults_t results;
        results.m_failure = m_failure;
        results.m_failureReason = m_failureReason;

        if ( !m_failure ) {
            results.m_wavBuffer = &m_wavBuffer;
        }

        CallOnDone( &results );
        break;
    }};

    return JOB_OK;
}
