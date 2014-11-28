#if !defined( IENGINESPEECHBOT_H )
#define IENGINESPEECHBOT_H

#ifdef _WIN32
#pragma once
#endif

#include "tier1/tier1.h"
#include "chatbot.h"
#include "stt.h"
#include "tts.h"

struct SpeechBotResults_t {
    int        m_failure;
    CUtlString m_failureReason;

    CUtlBuffer *m_wavBuffer;
    CUtlString  m_text;
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
    CSpeechBotJob( const SpeechBotParams_t &params );
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

#endif // IENGINESPEECHBOT_H
