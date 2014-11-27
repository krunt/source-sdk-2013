#if !defined( IENGINEATTSTT_H )
#define IENGINEATTSTT_H

#ifdef _WIN32
#pragma once
#endif

#include "stt.h"

class CAttSpeechToTextJob : public CSpeechToTextJob {
public:
    CAttSpeechToTextJob( const SpeechToTextParams_t &params );

protected:
    virtual void OnAuthReceived( HttpRequestResults_t &data );
    virtual void OnTextReceived( HttpRequestResults_t &data );

    virtual void ThinkOnAuth( void );
    virtual void ThinkOnText( void );

private: 
    CUtlString m_accessToken;
};

#endif // IENGINEATTSTT_H
