#include "att_stt.h"

void CAttSpeechToTextJob::OnAuthReceived( HttpRequestResults_t &data ) {
    Json::Value args;
    Json::Reader jsonReader;

    if ( data.m_failure || !jsonReader.parse( 
                (const char *)data.m_outputBuffer->Base(),
            (const char *)data.m_outputBuffer->Base() 
            + data.m_outputBuffer->TellPut(), args )
            || !args.isMember( "access_token" ) )
    {
        m_state = STT_ST_DONE;
        if ( !data.m_failure ) {
            m_failure = 1;
            if ( !args.isMember( "access_token" ) ) {
                m_failureReason = "no access token found";
            } else {
                m_failureReason = "invalid json received";
            }
        }

        m_cachedAccessToken.StopUpdate();

    } else { 
        // success
        m_accessToken = args[ "access_token" ].asCString();
        m_state = STT_ST_TEXT;

        m_cachedAccessToken.Init( m_accessToken, 
            gpGlobals->curtime + args[ "expires_in" ].asInt() );
    }

    DoExecute();
}

void CAttSpeechToTextJob::OnTextReceived( HttpRequestResults_t &data ) {
    if ( data.m_failure ) {
        m_failure = data.m_failure;
        m_failureReason = *data.m_failureReason;

        m_state = STT_ST_DONE;

    } else { 
        // success
        m_state = STT_ST_DONE;

        Json::Value args;
        Json::Reader jsonReader;

        /*
        fwrite( data.m_outputBuffer->Base(), 
            1, data.m_outputBuffer->TellPut(), stderr );
            */

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

void CAttSpeechToTextJob::ThinkOnAuth( void ) {

    /* strategy here: try update once for yourself */

    while ( 1 ) {
        /* here is small race  \|/ */
        if ( !m_cachedAccessToken.IsExpired() ) {
            m_accessToken = m_cachedAccessToken.GetToken();
            m_state = STT_ST_TEXT;
            return;
        }

        if ( m_cachedAccessToken().StartUpdate() ) {
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
            params.m_onDone = boost::bind( 
                    &CAttSpeechToTextJob::OnAuthReceived, this, _1 );
        
            g_pThreadPool->AddJob( new CHttpRequestJob( params ) );
            return;

        } else {
            /* wait for token updating */
            while ( m_cachedAccessToken().IsUpdating() ) {
                ThreadSleep( 50 );
            }
        }
    }
}

void CAttSpeechToTextJob::ThinkOnText( void ) {
    HttpRequestParams_t params;

    params.m_requestType = HTTP_POST;
    params.m_url = m_params.m_sttUrl;

    CUtlString t = "Authorization: Bearer ";
    t += m_accessToken;

    params.m_headers.AddToTail( t );
    params.m_headers.AddToTail( "Accept: application/json" );
    params.m_headers.AddToTail( "Content-Type: audio/wav" );

    params.m_inputOpMethod = HTTP_ME_BUFFER;
    params.m_inputBuffer.Put( m_wavBuffer.Base(), m_wavBuffer.TellPut() );

    params.m_outputOpMethod = HTTP_ME_BUFFER;
    params.m_onDone = boost::bind( &CAttSpeechToTextJob::OnTextReceived, 
            this, _1 );

    g_pThreadPool->AddJob( new CHttpRequestJob( params ) );
}

CachedAccessToken CAttSpeechToTextJob::m_accessToken;

