#if !defined( IENGINEATTTOKEN_H )
#define IENGINEATTTOKEN_H

#include "tier1/tier1.h"
#include "tier0/threadtools.h"

class CAttCachedAccessToken {
public:
    CAttCachedAccessToken() : m_expiredTime( 0 ) {
        m_isUpdating = 0;
    }

    void Init( const CUtlString &token, float time ) {
        m_accessToken = token;
        //m_expiredTime = time;
        m_expiredTime = 1;
        m_isUpdating = 0;
    }

    bool IsExpired() const {
        //return m_expiredTime > gpGlobals->curtime + 128; /* for safety */
        return m_expiredTime ? 0 : 1; // hack gpGlobals->curtime is unavaiable
    }

    bool StartUpdate( void ) {
        //int oldValue = 0;
        //int newValue = 1;
        //return m_isUpdating.AssignIf( &oldValue, &newValue );
        return 1;
    }

    bool IsUpdating( void ) const {
        return m_isUpdating;
    }

    void StopUpdate( void ) {
        m_isUpdating = 0;
    }

    CUtlString GetToken() const {
        return m_accessToken;
    }

private:
    CUtlString m_accessToken;
    float m_expiredTime;

    CInterlockedInt m_isUpdating;
};

#endif //IENGINEATTTOKEN_H
