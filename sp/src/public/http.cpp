#include "http.h"
#include <curl/curl.h>

#include <fstream>
#include <sstream>
#include <string>


HttpRequestParams_t::HttpRequestParams_t() 
    : m_onDone( NULL )
{}

CHttpRequestJob::CHttpRequestJob( const HttpRequestParams_t &params )
    : CFunctorJob( NULL, "" ), m_params( params )
{
    m_failure = 0;
    m_failureReason = "";

    SetOnDone( params.m_onDone );
}

static int MyGetFileSize( std::fstream &fs ) {
    int result;
    std::streampos s = fs.tellg();
    fs.seekg( 0, std::ios_base::end );
    result = fs.tellg();
    fs.seekg( s );
    return result;
}

static size_t MyReadDataViaBuffer(char *ptr, size_t size, 
        size_t nmemb, void *userdata)
{
    std::stringstream *ss = (std::stringstream *)userdata;
    ss->read( ptr, size * nmemb );
    return ss->gcount();
}

static size_t MyWriteDataViaBuffer(char *ptr, size_t size, 
        size_t nmemb, void *userdata)
{
    std::stringstream *ss = (std::stringstream *)userdata;
    ss->write( ptr, size * nmemb );
    return size * nmemb;
}

static size_t MyReadDataViaFile(char *ptr, size_t size, 
        size_t nmemb, void *userdata)
{
    std::fstream *ss = (std::fstream *)userdata;
    ss->read( ptr, size * nmemb );
    return ss->gcount();
}

static size_t MyWriteDataViaFile(char *ptr, size_t size, 
        size_t nmemb, void *userdata)
{
    std::fstream *ss = (std::fstream *)userdata;
    ss->write( ptr, size * nmemb );
    return size * nmemb;
}


JobStatus_t CHttpRequestJob::DoExecute() {
    int i;
    const bool verbose = true;
    struct curl_slist *slist = NULL;
    CURL *curl;

    curl = curl_easy_init();

    if ( !curl ) {
        m_failure = 1;
        m_failureReason = "curl == NULL";
        goto out;
    }

    if ( verbose ) {
        curl_easy_setopt( curl, CURLOPT_HEADER, 1 );
        curl_easy_setopt( curl, CURLOPT_VERBOSE, 1 );
    }

    switch ( m_params.m_request_type ) {
    case HTTP_GET: curl_easy_setopt( curl, CURLOPT_GET, 1 ); break;
    case HTTP_POST: curl_easy_setopt( curl, CURLOPT_POST, 1 ); break;
    default: assert( 0 );
    };

    curl_easy_setopt( curl, CURLOPT_URL, m_params.m_url.Get() );
    curl_easy_setopt( curl, CURLOPT_FOLLOWLOCATION, 1L );

    for ( i = 0; i < m_params.m_headers.Count(); ++i ) {
        slist = curl_slist_append( slist, m_params.m_headers[i].Get() );
    }

    if ( slist ) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
    }

    std::stringstream inputBufStream, outputBufStream;
    std::fstream inputFileStream, outputFileStream;

    if ( m_params.m_inputOpMethod == HTTP_ME_BUFFER ) {
        inputBufStream.write( m_params.m_inputBuffer.Base(), 
                m_params.m_inputBuffer.TellPut() );
        curl_easy_setopt( curl, CURLOPT_READFUNCTION, &MyReadDataViaBuffer );
        curl_easy_setopt( curl, CURLOPT_POSTFIELDSIZE, 
                m_params.m_inputBuffer.TellPut() );
        curl_easy_setopt( curl, CURLOPT_READDATA, &inputBufStream );
    } else {
        inputFileStream.open( m_params.m_inputFile.Get(),
                std::ios_base::in | std::ios_base::binary );
        curl_easy_setopt( curl, CURLOPT_READFUNCTION, &MyReadDataViaFile );
        curl_easy_setopt( curl, CURLOPT_POSTFIELDSIZE, 
            MyGetFileSize( inputFileStream ) );
        curl_easy_setopt( curl, CURLOPT_READDATA, &inputFileStream );
    }

    if ( m_params.m_outputOpMethod == HTTP_ME_BUFFER ) {
        curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, &MyWriteDataViaBuffer );
        curl_easy_setopt( curl, CURLOPT_WRITEDATA, &outputBufStream );
    } else {
        outputFileStream.open( m_params.m_inputFile.Get(),
                std::ios_base::out | std::ios_base::binary );
        curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, &MyWriteDataViaFile );
        curl_easy_setopt( curl, CURLOPT_WRITEDATA, &outputFileStream );
    }

    m_failure = curl_easy_perform( curl );

    curl_slist_free_all( slist );
    curl_easy_cleanup( curl );

    if ( m_failure != CURLE_OK ) {
        m_failureReason = "curl-specific";
    } else if ( m_params.m_outputOpMethod == HTTP_ME_BUFFER ) {
        /* optimize here */
        std::string str = outputBufStream.str();
        m_params.m_outputBuffer.Put( str.data(), str.size() );
    }

out:
    HttpRequestResults_t results;
    results.m_outputBuffer = &m_params.m_outputBuffer;
    results.m_failure = m_failure;
    results.m_failureReason = &m_failureReason;

    CallOnDone( &m_params );

    return JOB_OK;
}

