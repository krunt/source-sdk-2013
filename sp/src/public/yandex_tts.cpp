#include "yandex_tts.h"

#undef min
#undef max

#include <curl/curl.h>

#include <boost/bind.hpp>
#include <boost/function.hpp>

CYandexTextToSpeechJob::CYandexTextToSpeechJob( const TextToSpeechParams_t &params )
    : CTextToSpeechJob( params )
{}

static CUtlString urlEncode( const char *s, int length ) {
    CURL *curl = curl_easy_init();
    char *p = curl_easy_escape( curl, s, length );
    CUtlString result = p;
    curl_free( p );
    curl_easy_cleanup( curl );
    return result;
}

/* think about utf-16 and utf-8 conversions */

void CYandexTextToSpeechJob::ThinkOnWav( void ) {
    HttpRequestParams_t params;

    params.m_requestType = HTTP_GET;
    params.m_url = m_params.m_ttsUrl;

    params.m_url += "?text=";
    params.m_url += urlEncode( m_params.m_text.Get(), m_params.m_text.Length() );
    params.m_url += "&lang=ru-RU";
    params.m_url += "&speaker=zahar";
    params.m_url += CUtlString( "&key=" );
    params.m_url += m_params.m_appKey;
    params.m_url += "&format=wav";

    params.m_inputOpMethod = HTTP_ME_BUFFER;

    params.m_outputOpMethod = HTTP_ME_BUFFER;
    params.m_onDone = boost::bind(
            &CYandexTextToSpeechJob::OnWavReceived, this, _1 );

    g_pThreadPool->AddJob( new CHttpRequestJob( params ) );
}

