#if !defined( IENGINEHTTP_H )
#define IENGINEHTTP_H

#include "tier1/tier1.h"
#include "tier1/functors.h"
#include "tier1/utlbuffer.h"
#include "vstdlib/jobthread.h"

#undef min
#undef max
#include <boost/function.hpp>

#ifdef _WIN32
#pragma once
#endif

template <typename T>
class COnDoneCallback {
public:
    virtual void SetOnDone( const boost::function1<void, T> &func ) { m_func = func; }
            void CallOnDone( T &arg ) { m_func( arg ); }
private:
    boost::function1<void, T> m_func;
};

enum CHttpRequestType_t { HTTP_GET, HTTP_POST };
enum CHttpOpMethod_t { HTTP_ME_FILE, HTTP_ME_BUFFER };

struct HttpRequestResults_t {
    CUtlBuffer *m_outputBuffer;

    int m_failure;
    CUtlString *m_failureReason;
};

struct HttpRequestParams_t {
    HttpRequestParams_t( void ) {}
    HttpRequestParams_t( const HttpRequestParams_t &params );

    CHttpRequestType_t m_requestType;

    CUtlString m_url;
    CCopyableUtlVector<CUtlString> m_headers;

    CHttpOpMethod_t m_inputOpMethod;
    CUtlBuffer m_inputBuffer;
    CUtlString m_inputFile;

    CHttpOpMethod_t m_outputOpMethod;
    CUtlBuffer m_outputBuffer;
    CUtlString m_outputFile;

    boost::function1<void, HttpRequestResults_t> m_onDone;
};


class CHttpRequestJob : public CFunctorJob, public COnDoneCallback<HttpRequestResults_t> {
public:
    CHttpRequestJob( const HttpRequestParams_t &params );
    virtual JobStatus_t DoExecute( void );

private:
    HttpRequestParams_t m_params;
    int m_failure;
    CUtlString m_failureReason;
};

#endif // IENGINEHTTP_H
