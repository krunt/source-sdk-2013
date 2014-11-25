#if !defined( IENGINETTS_H )
#define IENGINETTS_H

#ifdef _WIN32
#pragma once
#endif

struct TextToSpeechParams_t {
    CUtlString m_text;

    CUtlString m_ttsUrl;
    CUtlString m_convUrl;

    CUtlString m_appKey;
    CUtlString m_appSecret;

    CRefPtr<CFunctor> m_onDone;
};

struct TextToSpeechResults_t {
    int        m_failure;
    CUtlString m_failureReason;

    CUtlBuffer *m_wavBuffer;
};

enum { TTS_ST_INIT, 
       TTS_ST_AUTH, 
       TTS_ST_WAV,  
       TTS_ST_CONV,  
       TTS_ST_DONE,  
} TextToSpeechState_t;


class CTextToSpeechJob : public CFunctorJob, public COnDoneCallback {
public:
    CTextToSpeechJob( TextToSpeechParams_t &params );
    virtual JobStatus_t DoExecute( void );

private:
    void OnAuthReceived( HttpRequestResults_t *data );
    void OnWavReceived( HttpRequestResults_t *data );
    void OnWavConvReceived( HttpRequestResults_t *data );

    TextToSpeechState_t m_state;
    TextToSpeechParams_t m_params;

    int m_failure;
    CUtlString m_failureReason;

    CUtlString m_accessToken;
    CUtlBuffer m_wavBuffer;
};

#endif // IENGINETTS_H
