#if !defined( IENGINETTS_H )
#define IENGINETTS_H

#ifdef _WIN32
#pragma once
#endif

#include "tier1/tier1.h"
#include "tier1/refcount.h"
#include "tier1/utlbuffer.h"

#include "http.h"

struct TextToSpeechResults_t {
    int        m_failure;
    CUtlString m_failureReason;

    CUtlBuffer *m_wavBuffer;
};

struct TextToSpeechParams_t {
    CUtlString m_text;

    CUtlString m_authUrl;
    CUtlString m_ttsUrl;
    CUtlString m_convUrl;

    CUtlString m_appKey;
    CUtlString m_appSecret;

    CUtlString m_absPath;

    boost::function1<void, TextToSpeechResults_t> m_onDone;
};

enum TextToSpeechState_t 
     { TTS_ST_INIT, 
       TTS_ST_AUTH, 
       TTS_ST_WAV,  
       TTS_ST_CONV,  
       TTS_ST_DONE,  
};


class CTextToSpeechJob : public CFunctorJob, public COnDoneCallback<TextToSpeechResults_t> {
public:
    CTextToSpeechJob( const TextToSpeechParams_t &params );
    virtual ~CTextToSpeechJob() {}

    virtual JobStatus_t DoExecute( void );

protected:
    virtual void OnAuthReceived( HttpRequestResults_t &data );
    virtual void OnWavReceived( HttpRequestResults_t &data );
    virtual void OnWavConvReceived( HttpRequestResults_t &data );

    virtual void ThinkOnAuth( void );
    virtual void ThinkOnWav( void );
    virtual void ThinkOnConv( void );

    TextToSpeechState_t m_state;
    TextToSpeechParams_t m_params;

    int m_failure;
    CUtlString m_failureReason;
    CUtlBuffer m_wavBuffer;
};

#endif // IENGINETTS_H
