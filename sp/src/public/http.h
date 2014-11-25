#if !defined( IENGINEHTTP_H )
#define IENGINEHTTP_H

#ifdef _WIN32
#pragma once
#endif

class COnDoneCallback {
public:
    virtual void SetOnDone( CFunctor *func ) { m_func = func; }
            void CallOnDone() { if ( m_func ) { (*m_func)(); } }
            void CallOnDone( void *arg1 ) { if ( m_func ) { (*m_func)( arg1 ); } }
private:
    CRefPtr<CFunctor> m_func;
};

enum { HTTP_GET, HTTP_POST } CHttpRequestType_t;
enum { HTTP_ME_FILE, HTTP_ME_BUFFER } CHttpOpMethod_t;

struct HttpRequestParams_t {
    HttpRequestParams_t();

    CHttpRequestType_t m_requestType;

    CUtlString m_url;
    CUtlVector<CUtlString> m_headers;

    CHttpOpMethod_t m_inputOpMethod;
    CUtlBuffer m_inputBuffer;
    CUtlString m_inputFile;

    CHttpOpMethod_t m_outputOpMethod;
    CUtlBuffer m_outputBuffer;
    CUtlString m_outputFile;

    CRefPtr<CFunctor> m_onDone;
};

struct HttpRequestResults_t {
    CUtlBuffer *m_outputBuffer;

    int m_failure;
    CUtlString *m_failureReason;
};


class CHttpRequestJob : public CFunctorJob, public COnDoneCallback {
public:
    CHttpRequestJob( HttpRequestParams_t &params );
    virtual JobStatus_t DoExecute( void );

private:
    HttpRequestParams_t m_params;
    int m_failure;
    CUtlString m_failureReason;
};

#endif // IENGINEHTTP_H
