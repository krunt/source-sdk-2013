#if !defined( IENGINEYATTS_H )
#define IENGINEYATTS_H

#ifdef _WIN32
#pragma once
#endif

#include "tts.h"

class CYandexTextToSpeechJob : public CTextToSpeechJob {
public:
    CYandexTextToSpeechJob( const TextToSpeechParams_t &params );

    virtual void ThinkOnWav( void );
};

#endif // IENGINEYATTS_H
