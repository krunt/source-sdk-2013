#if !defined( IENGINEMEMORYIO_H )
#define IENGINEMEMORYIO_H

#include "tier2/riff.h"

class InMemoryFileReadBinary : public IFileReadBinary {
public:
    InMemoryFileReadBinary( const char *m_ptr, int size )
    { m_buf.Put( m_ptr, size ); }

    virtual int open( const char *pFileName ) { return 1; }
    virtual int read( void *pOutput, int size, int file ) {
        assert( file == 1 );
        int toRead = std::max( std::min( size, size( 1 ) - tell( 1 ) ), 0 );

        if ( toRead ) {
            m_buf.Get( pOutput, toRead );
        }

        return toRead;

    }
    virtual void close( int file ) { assert( file == 1 ); }
    virtual void seek( int file, int pos ) { 
        assert( file == 1 ); 
        m_buf.SeekGet( CUtlBuffer::SEEK_HEAD, pos );
    }
    virtual unsigned int tell( int file ) { 
        assert( file == 1 ); 
        return m_buf.TellGet();
    }
    virtual unsigned int size( int file ) { 
        assert( file == 1 ); 
        return m_buf.TellPut();
    }

private:
    CUtlBuffer m_buf;
};

class InMemoryFileWriteBinary : public IFileWriteBinary {
public:
    InMemoryFileWriteBinary( void ) {}

	virtual int create( const char *pFileName ) { return 1; }
	virtual int write( void *pData, int size, int file ) {
        assert( file == 1 );
        m_buf.Put( pdata, size );
    }
	virtual void close( int file ) {
        assert( file == 1 );
    }
	virtual void seek( int file, int pos ) {
        assert( file == 1 );
        m_buf.SeekPut( CUtlBuffer::SEEK_HEAD, pos );
    }
	virtual unsigned int tell( int file ) {
        assert( file == 1 );
        return m_buf.TellPut();
    }

    const CUtlBuffer &GetBuffer() const { return m_buf; }

private:
    CUtlBuffer m_buf;
};

#endif // IENGINEMEMORYIO_H
