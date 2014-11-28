#if !defined( IENGINECHATBOT_H )
#define IENGINECHATBOT_H

#ifdef _WIN32
#pragma once
#endif

#include "tier1/tier1.h"
#include "tier1/utlstring.h"
#include "tier1/functors.h"

#include "http.h"

#undef max
#undef min
#include <boost/bind.hpp>
#include <boost/function.hpp>

class CChatBot {
public:
    ~CChatBot() {}
    virtual CUtlString Answer( const CUtlString &text ) = 0;
};

class CEngChatBot : public CChatBot {
public:
    virtual CUtlString Answer( const CUtlString &text ) { return text; }
};

class CRusChatBot : public CChatBot {
public:
    virtual CUtlString Answer( const CUtlString &text ) { return text; }
};

struct ChatBotResults_t {
    int        m_failure;
    CUtlString m_failureReason;

    CUtlString m_text;
};

struct ChatBotParams_t {
    CUtlString m_text;
    CUtlString m_language;
    boost::function1<void, ChatBotResults_t> m_onDone;
};

class CChatBotJob : public CFunctorJob, public COnDoneCallback<ChatBotResults_t> {
public:
    CChatBotJob( const ChatBotParams_t &params );
    virtual ~CChatBotJob() {}

    virtual JobStatus_t DoExecute( void );

private:
    int m_failure;
    CUtlString m_failureReason;
    ChatBotParams_t m_params;
};

extern CChatBot *GetBotForLanguage( const CUtlString &language );

#endif // IENGINECHATBOT_H
