
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

void CTextToSpeechJob::OnAuthReceived( HttpRequestResults_t &data ) {
    Json::Value args;
    Json::Reader jsonReader;

    if ( data.m_failure || !jsonReader.parse( (const char *)data.m_outputBuffer->Base(),
            (const char *)data.m_outputBuffer->Base() + data.m_outputBuffer->TellPut(), args ) 
            || !args.isMember( "access_token" ) )
    {
        m_state = TTS_ST_DONE;
        if ( !data.m_failure ) {
            m_failure = 1;
            if ( !args.isMember( "access_token" ) ) {
                m_failureReason = "no access token found";
            } else {
                m_failureReason = "invalid json received";
            }
        }

    } else { 
        // success
        m_accessToken = args[ "access_token" ].asCString();
        m_state = TTS_ST_WAV;
    }

    DoExecute();
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

    case TTS_ST_AUTH: {
        HttpRequestParams_t params;

        params.m_requestType = HTTP_POST;
        params.m_url = m_params.m_authUrl;

        params.m_headers.AddToTail(  
            "Content-Type: application/x-www-form-urlencoded" );
        params.m_headers.AddToTail( "Accept: application/json" );

        params.m_inputOpMethod = HTTP_ME_BUFFER;

        CUtlString t = "client_id=";
        t += m_params.m_appKey;
        t += "&client_secret=";
        t += m_params.m_appSecret;
        t += "&scope=TTS&grant_type=client_credentials";

        params.m_inputBuffer.Put( t.Get(), t.Length() );

        params.m_outputOpMethod = HTTP_ME_BUFFER;
        params.m_onDone = boost::bind( &CTextToSpeechJob::OnAuthReceived, this, _1 );

        g_pThreadPool->AddJob( new CHttpRequestJob( params ) );
        break;
    }

    case TTS_ST_WAV: {
        HttpRequestParams_t params;

        params.m_requestType = HTTP_POST;
        params.m_url = m_params.m_ttsUrl;

        CUtlString t = "Authorization: Bearer ";
        t += m_accessToken;

        params.m_headers.AddToTail( t );
        params.m_headers.AddToTail( "Accept: audio/amr" );
        params.m_headers.AddToTail( "Content-Type: text/plain" );
        params.m_headers.AddToTail( "X-Arg: Volume=250,Tempo=-7,VoiceName=mike" );

        params.m_inputOpMethod = HTTP_ME_BUFFER;
        params.m_inputBuffer.Put( m_params.m_text.Get(), m_params.m_text.Length() );

        params.m_outputOpMethod = HTTP_ME_BUFFER;
        params.m_onDone = boost::bind( &CTextToSpeechJob::OnWavReceived, this, _1 );

        g_pThreadPool->AddJob( new CHttpRequestJob( params ) );
        break;
    }

    case TTS_ST_CONV: {
        HttpRequestParams_t params;

        params.m_requestType = HTTP_POST;
        params.m_url = m_params.m_convUrl;

        params.m_headers.AddToTail( "Content-Type: audio/amr" );

        params.m_inputOpMethod = HTTP_ME_BUFFER;
        params.m_inputBuffer.Swap( m_wavBuffer );

        if ( m_params.m_absPath.IsEmpty() ) {
            params.m_outputOpMethod = HTTP_ME_BUFFER;
        } else {
            params.m_outputOpMethod = HTTP_ME_FILE;
            params.m_outputFile = m_params.m_absPath;
        }

        params.m_onDone = boost::bind( &CTextToSpeechJob::OnWavConvReceived, this, _1 );

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

        CallOnDone( results );
        break;
    }};

    return JOB_OK;
}
