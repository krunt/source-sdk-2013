#if !defined( IENGINECHATBOT_H )
#define IENGINECHATBOT_H

#ifdef _WIN32
#pragma once
#endif

#include "tier1/tier1.h"

class CChatBot {
public:
    ~CChatBot() {}
    virtual CUtlString Answer( const UtlString &text ) = 0;
};

class CEngChatBot : public CChatBot {
public:
    virtual CUtlString Answer( const UtlString &text ) { return text; }
};

class CRusChatBot : public CChatBot {
public:
    virtual CUtlString Answer( const UtlString &text ) { return text; }
};

struct SpeechBotResults_t {
    int        m_failure;
    CUtlString m_failureReason;

    CUtlBuffer *m_wavBuffer;
};

struct SpeechBotParams_t {
    CUtlString m_language;
    boost::function1<void, SpeechBotResults_t> m_onDone;
};

enum SpeechBotJobState_t 
     { SPEECH_ST_INIT, 
       SPEECH_ST_STT, 
       SPEECH_ST_T2T,  
       SPEECH_ST_TTS,  
       SPEECH_ST_DONE,  
};

class CSpeechBotJob : public CFunctorJob, 
    public COnDoneCallback<SpeechBotResults_t> {
public:
    CSpeechBotJob( const ChatBotParams_t &params );
    virtual ~CSpeechBotJob( void );

    virtual JobStatus_t DoExecute( void );

private:
    void OnTextToSpeechBotReceived( const TextToSpeechResults_t &res );
    void OnChatBotReceived( const ChatBotResults_t &res );
    void OnSpeechToTextBotReceived( const SpeechToTextResults_t &res );

    int m_failure;
    CUtlString m_failureReason;
    SpeechBotJobState_t m_state;

    SpeechBotParams_t m_params;

    CUtlBuffer m_wavBuffer;
    CUtlString m_text;
};

#endif // IENGINECHATBOT_H
