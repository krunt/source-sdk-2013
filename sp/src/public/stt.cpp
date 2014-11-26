
#include "stt.h"

#include <jsoncpp/json/json.h>

#include <boost/bind.hpp>

CSpeechToTextJob::CSpeechToTextJob( const SpeechToTextParams_t &params )
    : CFunctorJob( NULL, "" ), m_params( params ), m_state( TTS_ST_AUTH )
{
    m_failure = 0;
    m_failureReason = "";

    SetOnDone( params.m_onDone );
}

void CSpeechToTextJob::OnAuthReceived( HttpRequestResults_t &data ) {
    Json::Value args;
    Json::Reader jsonReader;

    if ( data.m_failure || !jsonReader.parse( 
                (const char *)data.m_outputBuffer->Base(),
            (const char *)data.m_outputBuffer->Base() 
            + data.m_outputBuffer->TellPut(), args )
            || !args.isMember( "access_token" ) )
    {
        m_state = SST_ST_DONE;
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
        m_state = SST_ST_TEXT;
    }

    DoExecute();
}

void CSpeechToTextJob::OnTextReceived( HttpRequestResults_t &data ) {
    if ( data.m_failure ) {
        m_failure = data.m_failure;
        m_failureReason = *data.m_failureReason;

        m_state = SST_ST_DONE;

    } else { 
        // success
        m_state = SST_ST_DONE;

        Json::Value args;
        Json::Reader jsonReader;

        if ( !jsonReader.parse( 
                (const char *)data.m_outputBuffer->Base(),
                (const char *)data.m_outputBuffer->Base() 
                    + data.m_outputBuffer->TellPut(), args )
            || !args.isMember( "Recognition" )
            || !args["Recognition"].isMember( "NBest" )
            || !args["Recognition"]["NBest"].isArray()
            || !args["Recognition"]["NBest"].size()
            || !args["Recognition"]["NBest"][0].isMember( "Hypothesis" )
           )
        {
            m_failure = 1;
            m_failureReason = "invalid json received";

        } else {
            m_text = args["Recognition"]["NBest"][0]["Hypothesis"].asCString();
        }
    }

    DoExecute();
}

JobStatus_t CSpeechToTextJob::DoExecute( void ) {
    switch ( m_state ) {

    case STT_ST_AUTH: {
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
        t += "&scope=SPEECH&grant_type=client_credentials";

        params.m_inputBuffer.Put( t.Get(), t.Length() );

        params.m_outputOpMethod = HTTP_ME_BUFFER;
        params.m_onDone = boost::bind( &CSpeechToTextJob::OnAuthReceived, 
                this, _1 );

        g_pThreadPool->AddJob( new CHttpRequestJob( params ) );
        break;
    }

    case STT_ST_TEXT: {
        HttpRequestParams_t params;

        params.m_requestType = HTTP_POST;
        params.m_url = m_params.m_sttUrl;

        CUtlString t = "Authorization: Bearer ";
        t += m_accessToken;

        params.m_headers.AddToTail( t );
        params.m_headers.AddToTail( "Accept: application/json" );
        params.m_headers.AddToTail( "Content-Type: audio/wav" );

        params.m_inputOpMethod = HTTP_ME_BUFFER;
        params.m_inputBuffer.Swap( m_params.m_wavBuffer );

        params.m_outputOpMethod = HTTP_ME_BUFFER;
        params.m_onDone = boost::bind( &CSpeechToTextJob::OnTextReceived, 
                this, _1 );

        g_pThreadPool->AddJob( new CHttpRequestJob( params ) );
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

