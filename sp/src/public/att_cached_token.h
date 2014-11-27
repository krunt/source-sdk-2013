#if !defined( IENGINEATTTOKEN_H )
#define IENGINEATTTOKEN_H

class CAttCachedAccessToken {
public:
    CCachedAccessToken() : m_expiredTime( 0 ) {
        m_isUpdating = 0;
    }

    void Init( const CUtlString &token, float time ) {
        m_accessToken = token;
        m_expiredTime = time;
        m_isUpdating = 0;
    }

    bool IsExpired() const {
        return m_expiredTime > gpGlobals->curtime + 128; /* for safety */
    }

    bool StartUpdate( void ) {
        return m_isUpdating.AssignIf( 0, 1 );
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
