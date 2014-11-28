#include "att_tts.h"

#undef min
#undef max

#include <jsoncpp/json/json.h>

#include <boost/bind.hpp>
#include <boost/function.hpp>

CAttTextToSpeechJob::CAttTextToSpeechJob( const TextToSpeechParams_t &params )
    : CTextToSpeechJob( params )
{}

void CAttTextToSpeechJob::OnAuthReceived( HttpRequestResults_t &data ) {
    Json::Value args;
    Json::Reader jsonReader;

    if ( data.m_failure || !jsonReader.parse( 
                (const char *)data.m_outputBuffer->Base(),
            (const char *)data.m_outputBuffer->Base() 
            + data.m_outputBuffer->TellPut(), args ) 
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

        m_cachedAccessToken.StopUpdate();

    } else { 
        // success
        m_accessToken = args[ "access_token" ].asCString();
        m_state = TTS_ST_WAV;

        m_cachedAccessToken.Init( m_accessToken, 0 );
            //gpGlobals->curtime + args[ "expires_in" ].asInt() );
    }

    DoExecute();
}

void CAttTextToSpeechJob::ThinkOnAuth( void ) {

    while ( 1 ) {
        /* here is small race  \|/ */
        if ( !m_cachedAccessToken.IsExpired() ) {
            m_accessToken = m_cachedAccessToken.GetToken();
            m_state = TTS_ST_WAV;
            DoExecute();
            return;
        }

        if ( m_cachedAccessToken.StartUpdate() ) {
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
            params.m_onDone = boost::bind( &CAttTextToSpeechJob::OnAuthReceived, 
                    this, _1 );
        
            g_pThreadPool->AddJob( new CHttpRequestJob( params ) );
            return;

        } else {
            /* wait for token updating */
            while ( m_cachedAccessToken.IsUpdating() ) {
                ThreadSleep( 50 );
            }
        }
    }
}

void CAttTextToSpeechJob::ThinkOnWav( void ) {
    HttpRequestParams_t params;

    params.m_requestType = HTTP_POST;
    params.m_url = m_params.m_ttsUrl;

    CUtlString t = "Authorization: Bearer ";
    t += m_accessToken;

    params.m_headers.AddToTail( t );
    params.m_headers.AddToTail( "Accept: audio/wav" );
    params.m_headers.AddToTail( "Content-Type: text/plain" );
    params.m_headers.AddToTail( "X-Arg: Volume=250,Tempo=-7,VoiceName=mike" );

    params.m_inputOpMethod = HTTP_ME_BUFFER;
    params.m_inputBuffer.Put( m_params.m_text.Get(), m_params.m_text.Length() );

    params.m_outputOpMethod = HTTP_ME_BUFFER;
    params.m_onDone = boost::bind(
            &CAttTextToSpeechJob::OnWavReceived, this, _1 );

    g_pThreadPool->AddJob( new CHttpRequestJob( params ) );
}

CAttCachedAccessToken CAttTextToSpeechJob::m_cachedAccessToken;
