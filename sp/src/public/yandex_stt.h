#if !defined( IENGINEYASTT_H )
#define IENGINEYASTT_H

#ifdef _WIN32
#pragma once
#endif

#include "stt.h"

class CYandexSpeechToTextJob : public CSpeechToTextJob {
public:
    CYandexSpeechToTextJob( const SpeechToTextParams_t &params );

protected:
    virtual void OnTextReceived( HttpRequestResults_t &data );
    virtual void ThinkOnText( void );
};

#endif // IENGINEYASTT_H
