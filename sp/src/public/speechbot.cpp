#include "chatbot.h"


CSpeechBotJob::CSpeechBotJob( const SpeechBotParams_t &params )
    : CFunctorJob( NULL, "" ), m_params( params ), m_state( SPEECH_ST_STT )
{
    SetOnDone( m_params.m_onDone );
}

CSpeechBotJob::~CSpeechBotJob( void ) {
}

void CSpeechBotJob::OnSpeechToTextBotReceived( const SpeechToTextResults_t &res ) {
    if ( res.failure ) {
        m_state = SPEECH_ST_DONE;
        m_failure=  res.failure;
        m_failureReason = res.m_failureReason;
    } else {
        m_state = SPEECH_ST_T2T;
        m_text = res.m_text;
    }
    DoExecute();
}

void CSpeechBotJob::OnTextToSpeechBotReceived( const TextToSpeechResults_t &res ) {
    if ( res.failure ) {
        m_failure=  res.failure;
        m_failureReason = res.m_failureReason;
    } else {
        m_wavBuffer.Put( (const char *)res.m_wavBuffer->Base(), 
                res.m_wavBuffer->TellPut() );
    }
    m_state = SPEECH_ST_DONE;
    DoExecute();
}

void CSpeechBotJob::OnChatBotReceived( const ChatBotResults_t &res ) {
    if ( res.failure ) {
        m_state = SPEECH_ST_DONE;
        m_failure=  res.failure;
        m_failureReason = res.m_failureReason;
    } else {
        m_state = SPEECH_ST_TTS;
        m_text = res.m_text;
    }
    DoExecute();
}

JobStatus_t CSpeechBotJob::DoExecute( void ) {
    CChatBot *bot = GetBotForLanguage( m_params.m_language );
    CUtlString text = bot->Answer( m_params.m_text );

    switch ( m_state ) {
    case SPEECH_ST_STT: {
        SpeechToTextParams_t params;

        params.m_onDone = boost::bind( &CSpeechBotJob::OnSpeechToTextBotReceived, 
                this, _1 );

        if ( m_params.language == "en-US" ) {
            params.m_authUrl = "https://api.att.com/oauth/v4/token";
            params.m_sttUrl = "https://api.att.com/speech/v3/speechToText";
            params.m_appKey = "yp9d7i3xjqgl1d7cymq5qzba0b9xxpkh";
            params.m_appSecret = "m8pagrvawgwxtqhdyr1fik2ghnkywspp";

            g_pThreadPool->AddJob( new CAttSpeechToTextJob( params ) );

        } else { // ru-RU
            params.m_authUrl = "";
            params.m_sttUrl = "https://asr.yandex.net/asr_xml";

            params.m_appKey = "f0d988b2-93a4-4a5c-945b-42fa3e3a5974";
            params.m_appSecret = "";

            g_pThreadPool->AddJob( new CYandexSpeechToTextJob( params ) );
        }
        break;
    }

    case SPEECH_ST_T2T: {
        ChatBotParams_t params;
        params.m_text = m_text;
        params.m_language = m_params.m_language;
        params.m_onDone = boost::bind( &CSpeechBotJob::OnChatBotReceived, 
                this, _1 );

        g_pThreadPool->AddJob( new CChatBotJob( params ) );
        break;
    }

    case SPEECH_ST_TTS: {
        TextToSpeechParams_t params;

        params.m_text = m_text;
        params.m_absPath = "";
        params.m_onDone = boost::bind( &CSpeechBotJob::OnTextToSpeechBotReceived, 
                this, _1 );

        if ( m_params.language == "en-US" ) {
            params.m_authUrl = "https://api.att.com/oauth/v4/token";
            params.m_ttsUrl = "https://api.att.com/speech/v3/textToSpeech";
            params.m_convUrl = "http://192.168.0.100:19999/";
            params.m_appKey = "5p8eyqkvfynsw9mlngcvrs6t1d1saxmp";
            params.m_appSecret = "0q4z4cgxonq50imw0ohinehpduj8updb";

            g_pThreadPool->AddJob( new CAttSpeechToTextJob( params ) );

        } else { // ru-RU
            params.m_authUrl = "";
            params.m_sttUrl = "http://tts.voicetech.yandex.net/generate";

            params.m_appKey = "f0d988b2-93a4-4a5c-945b-42fa3e3a5974";
            params.m_appSecret = "";

            g_pThreadPool->AddJob( new CYandexSpeechToTextJob( params ) );
        }
        break;
    }

    case SPEECH_ST_DONE: {
        SpeechBotResults_t results;
        results.m_failure = m_failure;
        results.m_failureReason = m_failureReason;
        if ( !results.m_failure ) {
            results.m_wavBuffer = &m_wavBuffer;
        }

        CallOnDone( results );
        break;
    }

    default:
        assert( 0 );
        break;
    };

    return JOB_OK;
}
