#if !defined( IENGINESTT_H )
#define IENGINESTT_H

#ifdef _WIN32
#pragma once
#endif

#include "tier1/tier1.h"
#include "tier1/refcount.h"
#include "tier1/utlbuffer.h"

#include "http.h"

#include "sound_recorder.h"

#undef min
#undef max
#include <boost/bind.hpp>
#include <boost/function.hpp>

struct WavRecordingResults_t {
    int        m_failure;
    CUtlString m_failureReason;

    CUtlBuffer m_wavBuffer;
};

struct WavRecordingParams_t {
    boost::function1<void, WavRecordingResults_t> m_onDone;
};

class CWavRecordingJob : public CFunctorJob, 
    public COnDoneCallback<WavRecordingResults_t> 
{
public:
    CWavRecordingJob( const WavRecordingParams_t &params );
    virtual JobStatus_t DoExecute( void );

private:
    WavRecordingParams_t m_params;
};

struct SpeechToTextResults_t {
    int        m_failure;
    CUtlString m_failureReason;

    CUtlString m_text;
};

struct SpeechToTextParams_t {
    CUtlString m_authUrl;
    CUtlString m_sttUrl;

    CUtlString m_appKey;
    CUtlString m_appSecret;

    boost::function1<void, SpeechToTextResults_t> m_onDone;
};

enum SpeechToTextState_t 
     { STT_ST_INIT, 
       STT_ST_WAV, 
       STT_ST_AUTH, 
       STT_ST_TEXT,  
       STT_ST_DONE,  
};

class CSpeechToTextJob : public CFunctorJob, 
    public COnDoneCallback<SpeechToTextResults_t> 
{
public:
    CSpeechToTextJob( const SpeechToTextParams_t &params );
    virtual JobStatus_t DoExecute( void );

private:
    void OnWavReceived( WavRecordingResults_t &data );
    void OnAuthReceived( HttpRequestResults_t &data );
    void OnTextReceived( HttpRequestResults_t &data );

    SpeechToTextState_t m_state;
    SpeechToTextParams_t m_params;

    int m_failure;
    CUtlString m_failureReason;

    CUtlString m_accessToken;

    CUtlBuffer m_wavBuffer;
    CUtlString m_text;
};

#endif // IENGINESTT_H
