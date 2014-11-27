#include "chatbot.h"

extern CChatBot *GetBotForLanguage( const UtlString &lang ) {
    if ( lang == "en-US" ) {
        return new CEngChatBot();
    } else if ( lang == "ru-RU" ) {
        return new CRusChatBot();
    }
    assert( 0 );
}

CChatBotJob::CChatBotJob( const ChatBotParams_t &params )
    : m_params( params )
{
    SetOnDone( m_params.m_onDone );
}

JobStatus_t CChatBotJob::DoExecute( void ) {
    CChatBot *bot = GetBotForLanguage( m_params.m_language );
    CUtlString text = bot->Answer( m_params.m_text );

    ChatBotResults_t results;
    results.m_failure = 0;
    results.m_failureReason = "";
    results.m_text = text;

    CallOnDone( results );

    delete bot;

    return JOB_OK;
}
