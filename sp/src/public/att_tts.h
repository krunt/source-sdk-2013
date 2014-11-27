#if !defined( IENGINEATTTTS_H )
#define IENGINEATTTTS_H

#ifdef _WIN32
#pragma once
#endif

#include "tts.h"

class CAttTextToSpeechJob : public CTextToSpeechJob {
public:
    CAttTextToSpeechJob( const TextToSpeechParams_t &params );

    virtual void OnAuthReceived( HttpRequestResults_t &data );

    virtual void ThinkOnAuth( void );
    virtual void ThinkOnWav( void );

private:
    CUtlString m_accessToken;
};

#endif // IENGINEATTTTS_H
