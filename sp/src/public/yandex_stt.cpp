#include "yandex_stt.h"

#include <tinyxml.h>

void CYandexSpeechToTextJob::OnTextReceived( HttpRequestResults_t &data ) {
    if ( data.m_failure ) {
        m_failure = data.m_failure;
        m_failureReason = *data.m_failureReason;

        m_state = STT_ST_DONE;

    } else { 
        // success
        m_state = STT_ST_DONE;

        /*
        fwrite( data.m_outputBuffer->Base(), 
            1, data.m_outputBuffer->TellPut(), stderr );
            */

        TiXmlDocument doc;

        m_failure = 1;
        doc.Parse( (const char *)data.m_outputBuffer->Base() );

        if ( !doc.Error() ) {
            TiXmlHandle docHandle( &doc );
            TiXmlElement *bestVariant 
                = docHandle.FirstChild( "recognitionResults" )
                .Child( "variant", 0 ).ToElement();


            if ( bestVariant ) {
                failure = 0;
                m_text = bestVariant->GetText();
            } else {
                m_failureReason = "not found variant in recognitionResults";
            }

        } else {
            m_failureReason = "yandex-stt-xml-parse error";
        }
    }

    DoExecute();
}

void CYandexSpeechToTextJob::ThinkOnText( void ) {
    HttpRequestParams_t params;

    params.m_requestType = HTTP_POST;
    params.m_url = m_params.m_sttUrl;

    params.m_url += "?uuid=01ae13cb744628b58fb536d496daa1e5"; // random uuid
    params.m_url += CUtlString( "&key=" ) + m_params.m_appKey;
    params.m_url += "&topic=notes";
    params.m_url += "&lang=ru-RU";

    params.m_headers.AddToTail( "Content-Type: audio/x-wav" );

    params.m_inputOpMethod = HTTP_ME_BUFFER;
    params.m_inputBuffer.Put( m_wavBuffer.Base(), m_wavBuffer.TellPut() );

    params.m_outputOpMethod = HTTP_ME_BUFFER;
    params.m_onDone = boost::bind( &CYandexSpeechToTextJob::OnTextReceived, 
            this, _1 );

    g_pThreadPool->AddJob( new CHttpRequestJob( params ) );
}

