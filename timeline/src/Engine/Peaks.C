
/*******************************************************************************/
/* Copyright (C) 2008-2021 Jonathan Moore Liles (as "Non-Timeline")            */
/* Copyright (C) 2023- Stazed                                                  */
/*                                                                             */
/* This file is part of Non-Timeline-XT                                        */
/*                                                                             */
/*                                                                             */
/* This program is free software; you can redistribute it and/or modify it     */
/* under the terms of the GNU General Public License as published by the       */
/* Free Software Foundation; either version 2 of the License, or (at your      */
/* option) any later version.                                                  */
/*                                                                             */
/* This program is distributed in the hope that it will be useful, but WITHOUT */
/* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       */
/* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for   */
/* more details.                                                               */
/*                                                                             */
/* You should have received a copy of the GNU General Public License along     */
/* with This program; see the file COPYING.  If not,write to the Free Software */
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */
/*******************************************************************************/

/*
  peakfile reading/writing.
*/

/* Code for peakfile reading, resampling, generation and streaming */

#include <FL/Fl.H>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "Audio_File.H"
#include "Peaks.H"

#include "assert.h"
#include "const.h"
#include "../../../nonlib/debug.h"
#include "../../../nonlib/Thread.H"
#include "../../../nonlib/file.h"

#include <errno.h>

#include <list>
#include <algorithm>
using std::min;
using std::max;

#include <math.h>

#include <stdint.h>

struct peak_thread_data
{
    void(*callback)(void*);
    void *userdata;
    Peaks *peaks;
};



/* whether to cache peaks at multiple resolutions on disk to
 * drastically improve performance */
bool Peaks::mipmapped_peakfiles = true;

const int Peaks::cache_minimum = 256;          /* minimum chunksize to build peakfiles for */
const int Peaks::cache_levels  = 8;           /* number of sampling levels in peak cache */
const int Peaks::cache_step    = 1;            /* powers of two between each level. 4 == 256, 2048, 16384, ... */

Peaks::peakbuffer Peaks::_peakbuf;



static
char *
peakname ( const char *filename )
{
    char *file;

    asprintf( &file, "%s.peak", filename );

    return file;
}



struct peakfile_block_header
{
    uint32_t chunksize;
    uint32_t skip;
} __attribute__ (( packed ));

class Peakfile
{

    FILE *_fp;
    nframes_t _chunksize;
    int _channels;   /* number of channels this peakfile represents */
    off_t _offset;

    struct block_descriptor
    {
        nframes_t chunksize;
        off_t pos;

        block_descriptor ( nframes_t chunksize, off_t pos ) : chunksize( chunksize ), pos( pos )
        {
        }

        bool
        operator< ( const block_descriptor &rhs )
        {
            return chunksize < rhs.chunksize;
        }
    };

    std::list <block_descriptor> blocks;

public:

    int
    nblocks ( void ) const
    {
        return blocks.size();
    }

    Peakfile ( ) :
        _fp(NULL),
        _chunksize(0),
        _channels(0),
        _offset(0)
    { }

    ~Peakfile ( )
    {
        if ( _fp )
            close();
    }

    void
    rescan ( void )
    {
        blocks.clear();
    }

    /* int blocks ( void ) const { return blocks.size(); } */
    /** find the best block for /chunksize/ */
    void
    scan ( nframes_t chunksize )
    {
        if ( ! blocks.size() )
        {
            rewind( _fp );
            clearerr( _fp );

            /* scan all blocks */
            for ( ;; )
            {
                peakfile_block_header bh;

                fread( &bh, sizeof( bh ), 1, _fp );

                if ( feof( _fp ) )
                    break;

                DMESSAGE( "Peakfile: chunksize=%lu, skip=%lu", (uint64_t)bh.chunksize, (uint64_t) bh.skip );

                ASSERT( bh.chunksize, "Chucksize of zero. Invalid peak file structure!" );

                blocks.push_back( block_descriptor( bh.chunksize, ftello( _fp ) ) );

                if ( ! bh.skip )
                    /* last block */
                    break;

                if ( fseeko( _fp, bh.skip, SEEK_CUR ) )
                {
                    WARNING( "seek failed: %s (%lu)", strerror( errno ), bh.skip );
                    break;
                }
            }
        }

        if ( ! blocks.size() )
        {
            DWARNING( "Peak file contains no blocks, maybe it's still being generated?");
            return;
        }

        blocks.sort();

        /* fall back on the smallest chunksize */
        fseeko( _fp, blocks.front().pos, SEEK_SET );
        _chunksize = blocks.front().chunksize;

        /* search for the best-fit chunksize */
        for ( std::list <block_descriptor>::const_reverse_iterator i = blocks.rbegin();
            i != blocks.rend(); ++i )
            if ( chunksize >= i->chunksize )
            {
                _chunksize = i->chunksize;
                fseeko( _fp, i->pos, SEEK_SET );
                break;
            }

        //           DMESSAGE( "using peakfile block for chunksize %lu", _chunksize );
        _offset = ftello( _fp );
    }

    /** convert frame number of peak number */
    nframes_t
    frame_to_peak ( nframes_t frame )
    {
        return ( frame / _chunksize ) * (nframes_t)_channels;
    }

    /** return the number of peaks in already open peakfile /fp/ */
    nframes_t
    npeaks ( void ) const
    {
        struct stat st;

        fstat( fileno( _fp ), &st );

        return ( st.st_size - sizeof( peakfile_block_header ) ) / sizeof( Peak );
    }

    /** returns true if the peakfile contains /npeaks/ peaks starting at sample /s/ */
    bool
    ready ( nframes_t start, nframes_t npeaks )
    {
        if ( blocks.size() > 1 )
            return true;
        else
            return this->npeaks() > frame_to_peak( start ) + npeaks;
    }

    /** given soundfile name /name/, try to open the best peakfile for /chunksize/ */
    bool
    open ( const char *name, int channels, nframes_t chunksize )
    {
        assert( ! _fp );
        //            _chunksize = 0;
        _channels = channels;

        char *pn = peakname( name );

        if ( ! ( _fp = fopen( pn, "r" ) ) )
        {
            WARNING( "Failed to open peakfile for reading: %s", strerror(errno) );
            free( pn );
            return false;
        }

        free( pn );

        scan( chunksize );

        /* assert( _chunksize ); */

        if ( blocks.size() )
        {
            return true;
        }
        else
        {
            DWARNING( "Peak file could not be opened: no blocks" );
            fclose(_fp);
            _fp = NULL;
            return false;
        }
    }

    bool
    open ( FILE *fp, int channels, nframes_t chunksize )
    {
        assert( ! _fp );

        _fp = fp;
        _chunksize = 0;
        _channels = channels;

        scan( chunksize );

        assert( _chunksize );

        return true;
    }

    void
    leave_open ( void )
    {
        _fp = NULL;
    }

    void
    close ( void )
    {
        fclose( _fp );
        _fp = NULL;
    }

    /** read /npeaks/ peaks at /chunksize/ starting at sample /s/
     * assuming the peakfile contains data for /channels/
     * channels. Place the result in buffer /peaks/, which must be
     * large enough to fit the entire request. Returns the number of
     * peaks actually read, which may be fewer than were requested. */
    nframes_t
    read_peaks ( Peak *peaks, nframes_t s, nframes_t npeaks, nframes_t chunksize )
    {
        if ( ! _fp )
        {
            DMESSAGE( "No peakfile open, WTF?" );
            return 0;
        }

        const unsigned int ratio = chunksize / _chunksize;

        /* locate to start position */

        if ( fseeko( _fp, _offset + ( frame_to_peak( s ) * sizeof( Peak ) ), SEEK_SET ) )
        {
            DMESSAGE( "failed to seek... peaks not ready?" );
            return 0;
        }

        if ( feof( _fp ) )
            return 0;

        if ( ratio == 1 )
            return fread( peaks, sizeof( Peak ) * _channels, npeaks, _fp );

        Peak *pbuf = new Peak[ ratio * _channels ];

        nframes_t len = 0;

        nframes_t i;

        for ( i = 0; i < npeaks; ++i )
        {
            /* read in a buffer */
            len = fread( pbuf, sizeof( Peak ) * _channels, ratio, _fp );

            Peak *pk = peaks + (i * _channels);

            /* get the peak for each channel */
            for ( int j = 0; j < _channels; ++j )
            {
                Peak *p = &pk[ j ];

                p->min = 0;
                p->max = 0;

                const Peak *pb = pbuf + j;

                for ( int k = len; k--; pb += _channels )
                {
                    if ( pb->max > p->max )
                        p->max = pb->max;
                    if ( pb->min < p->min )
                        p->min = pb->min;
                }

            }

            if ( feof( _fp) || len < ratio )
                break;
        }

        delete[] pbuf;

        return i;
    }
};



Peaks::Peaks ( Audio_File *c )
{
    _rescan_needed = false;
    _first_block_pending = false;
    _mipmaps_pending = false;
    _clip = c;
    _fpp = 0.0f;
    _peak_writer = NULL;
    _peakfile = new Peakfile();
}

Peaks::~Peaks ( )
{
    if ( _peak_writer )
    {
        delete _peak_writer;
        _peak_writer = NULL;
    }

    delete _peakfile;
    _peakfile = NULL;
}



/** Prepare a buffer of peaks from /s/ to /e/ for reading. Must be
 * called before any calls to operator[] */
int
Peaks::fill_buffer ( float fpp, nframes_t s, nframes_t e ) const
{
    _fpp = fpp;

    return read_peaks( s, (e - s) / fpp, fpp );
}

bool
Peaks::ready ( nframes_t s, nframes_t npeaks, nframes_t chunksize ) const
{
    if ( ! _peakfile->open( _clip->filename(), _clip->channels(), chunksize ) )
        return false;

    int r = _peakfile->ready( s, npeaks );

    _peakfile->close();

    return r;
}

/** If this returns false, then the peakfile needs to be built */
bool
Peaks::peakfile_ready ( void ) const
{
    if ( _rescan_needed )
    {
        DMESSAGE( "Rescanning peakfile" );
        _peakfile->rescan();
        if ( _peakfile->open( _clip->filename(), _clip->channels(), 256 ) )
            _peakfile->close();

        _rescan_needed = false;
    }

    return _first_block_pending || current();
}

/** start building peaks and/or peak mipmap in another thread. It is
 * safe to call this again before the thread finishes. /callback/ will
 * be called with /userdata/ FROM THE PEAK BUILDING THREAD when the
 * peaks are finished.  */
void
Peaks::make_peaks_asynchronously ( void(*callback)(void*), void *userdata ) const
{
    if ( _clip->dummy() )
        return;

    /* already working on it... */
    if( _first_block_pending || _mipmaps_pending )
        return;

    /* maybe still building mipmaps... */
    _first_block_pending = _peakfile->nblocks() < 1;
    _mipmaps_pending = _peakfile->nblocks() <= 1;

    peak_thread_data *pd = new peak_thread_data();

    pd->callback = callback;
    pd->userdata = userdata;
    pd->peaks = const_cast<Peaks*>(this);

    _make_peaks_thread.clone( &Peaks::make_peaks, pd );
    _make_peaks_thread.detach();

    DMESSAGE( "Starting new peak building thread" );
}

nframes_t
Peaks::read_peakfile_peaks ( Peak *peaks, nframes_t s, nframes_t npeaks, nframes_t chunksize ) const
{
    if ( ! _peakfile->open( _clip->filename(), _clip->channels(), chunksize ) )
    {
        DMESSAGE( "Failed to open peakfile!" );
        return 0;
    }

    nframes_t l = _peakfile->read_peaks( peaks, s, npeaks, chunksize );

    _peakfile->close();

    return l;
}

nframes_t
Peaks::read_source_peaks ( Peak *peaks, nframes_t npeaks, nframes_t chunksize ) const
{
    int channels = _clip->channels();

    sample_t *fbuf = new sample_t[ chunksize * channels ];

    off_t len;

    nframes_t i;
    for ( i = 0; i < npeaks; ++i )
    {
        /* read in a buffer */
        len = _clip->read( fbuf, -1, chunksize );

        Peak *pk = peaks + (i * channels);

        /* get the peak for each channel */
        for ( int j = 0; j < channels; ++j )
        {
            Peak &p = pk[ j ];

            p.min = 0;
            p.max = 0;

            for ( nframes_t k = j; k < len * channels; k += channels )
            {
                if ( fbuf[ k ] > p.max )
                    p.max = fbuf[ k ];
                if ( fbuf[ k ] < p.min )
                    p.min = fbuf[ k ];
            }

        }

        if ( len < (nframes_t)chunksize )
            break;
    }

    delete[] fbuf;

    return i;
}

nframes_t
Peaks::read_source_peaks ( Peak *peaks, nframes_t s, nframes_t npeaks, nframes_t chunksize ) const
{
    _clip->seek( s );

    return read_source_peaks( peaks, npeaks, chunksize );
}

nframes_t
Peaks::read_peaks ( nframes_t s, nframes_t npeaks, nframes_t chunksize ) const
{
    THREAD_ASSERT( UI );                                        /* because _peakbuf cache is static */

    //    printf( "reading peaks %d @ %d\n", npeaks, chunksize );

    if ( _peakbuf.size < (nframes_t)( npeaks * _clip->channels() ) )
    {
        _peakbuf.size = npeaks * _clip->channels();
        //        printf( "reallocating peak buffer %li\n", _peakbuf.size );
        _peakbuf.buf = static_cast<peakdata*>( realloc( _peakbuf.buf, sizeof( peakdata ) + (_peakbuf.size * sizeof( Peak )) ) );
    }

    _peakbuf.offset = s;
    _peakbuf.buf->chunksize = chunksize;

    /* FIXME: use actual minimum chunksize from peakfile! */
    if ( chunksize < (nframes_t)cache_minimum )
    {
        _peakbuf.len = read_source_peaks( _peakbuf.buf->data, s, npeaks, chunksize );
    }
    else
    {
        _peakbuf.len = read_peakfile_peaks( _peakbuf.buf->data, s, npeaks, chunksize );
    }

    return _peakbuf.len;
}

/** returns false if peak file for /filename/ is out of date  */
bool
Peaks::current ( void ) const
{
    char *pn = peakname( _clip->filename() );

    bool b = newer( pn, _clip->filename() );

    free( pn );

    return b;
}

/* thread entry point */
void *
Peaks::make_peaks ( void *v )
{
    peak_thread_data *pd = static_cast<peak_thread_data*>( v );

    if ( pd->peaks->make_peaks() )
    {
        if ( pd->callback )
            pd->callback( pd->userdata );

        pd->peaks->_rescan_needed = true;
    }

    delete pd;

    return NULL;
}

bool
Peaks::needs_more_peaks ( void ) const
{
    return _peakfile->nblocks() <= 1 && ! ( _first_block_pending || _mipmaps_pending );
}

bool
Peaks::make_peaks ( void ) const
{
    Peaks::Builder pb( this );

    /* make the first block - return not used here */
    bool b = pb.make_peaks();

    _first_block_pending = false;

    b = pb.make_peaks_mipmap();

    _mipmaps_pending = false;

    return b;
}

/** return normalization factor for a single peak, assuming the peak
 * represents a downsampling of the entire range to be normalized. */
float
Peak::normalization_factor( void ) const
{
    float s;

    s = 1.0f / fabs( this->max );

    if ( s * this->min < -1.0 )
        s = 1.0f / fabs( this->min );

    return s;
}

/* wrapper for peak writer */
void
Peaks::prepare_for_writing ( void )
{
    THREAD_ASSERT( Capture );

    assert( ! _peak_writer );

    char *pn = peakname( _clip->filename() );

    _first_block_pending = true;
    _peak_writer = new Peaks::Streamer( pn, _clip->channels(), cache_minimum );

    free( pn );
}

void
Peaks::finish_writing ( void )
{
    assert( _peak_writer );

    delete _peak_writer;
    _peak_writer = NULL;

    _first_block_pending = false;
}

void
Peaks::write ( sample_t *buf, nframes_t nframes )
{
    THREAD_ASSERT( Capture );

    _peak_writer->write( buf, nframes );
}



/*
  The Streamer is for streaming peaks from audio buffers to disk while
  capturing. It works by accumulating a peak value across write()
  calls. The Streamer can only generate peaks at a single
  chunksize--additional cache levels must be appended after the
  Streamer has finished.
*/

Peaks::Streamer::Streamer ( const char *filename, int channels, nframes_t chunksize )
{
    _channels  = channels;
    _chunksize = chunksize;
    _index     = 0;
    _fp = NULL;

    _peak = new Peak[ channels ];
    memset( _peak, 0, sizeof( Peak ) * channels );

    if ( ! ( _fp = fopen( filename, "w" ) ) )
    {
        FATAL( "could not open peakfile for streaming." );
    }

    peakfile_block_header bh;

    bh.chunksize = chunksize;
    bh.skip = 0;

    fwrite( &bh, sizeof( bh ), 1, _fp );

    fflush( _fp );
    fsync( fileno( _fp ) );
}

Peaks::Streamer::~Streamer ( )
{
    /*     fwrite( _peak, sizeof( Peak ) * _channels, 1, _fp ); */

    fflush( _fp );

    touch( fileno( _fp ) );

    fsync( fileno( _fp ) );

    fclose( _fp );

    delete[] _peak;
}

/** append peaks for samples in /buf/ to peakfile */
void
Peaks::Streamer::write ( const sample_t *buf, nframes_t nframes )
{
    while ( nframes )
    {
        const nframes_t remaining = _chunksize - _index;

        if ( ! remaining )
        {
            fwrite( _peak, sizeof( Peak ) * _channels, 1, _fp );

            memset( _peak, 0, sizeof( Peak ) * _channels );

            _index = 0;
        }

        int processed = min( nframes, remaining );

        for ( int i = _channels; i--; )
        {
            Peak *p = _peak + i;

            const sample_t *f = buf + i;

            for ( int j = processed; j--; f += _channels )
            {
                if ( *f > p->max )
                    p->max = *f;
                if ( *f < p->min )
                    p->min = *f;
            }
        }

        _index  += processed;
        nframes -= processed;
    }

    /* FIXME: shouldn't we just use write() instead? */
    fflush( _fp );
}



/*
  The Builder is for generating peaks from imported or updated
  sources, or when the peakfile is simply missing.
*/

void
Peaks::Builder::write_block_header ( nframes_t chunksize )
{
    if ( last_block_pos )
    {
        /* update previous block */
        off_t pos = ftello( fp );

        fseeko( fp, last_block_pos - sizeof( peakfile_block_header ), SEEK_SET );

        peakfile_block_header bh;

        fread( &bh, sizeof( bh ), 1, fp );

        fseeko( fp, last_block_pos - sizeof( peakfile_block_header ), SEEK_SET );
        //                fseeko( fp, 0 - sizeof( bh ), SEEK_CUR );

        //        DMESSAGE( "old block header: chunksize=%lu, skip=%lu", (unsigned long) bh.chunksize, (unsigned long) bh.skip );

        bh.skip = pos - last_block_pos;

        ASSERT( bh.skip, "Attempt to create empty block. pos=%lu, last_block_pos=%lu", pos, last_block_pos );

        //        DMESSAGE( "new block header: chunksize=%lu, skip=%lu", (unsigned long) bh.chunksize, (unsigned long) bh.skip );

        fwrite( &bh, sizeof( bh ), 1, fp );

        fseeko( fp, pos, SEEK_SET );
    }

    peakfile_block_header bh;

    bh.chunksize = chunksize;
    bh.skip = 0;

    fwrite( &bh, sizeof( bh ), 1, fp );

    last_block_pos = ftello( fp );

    fflush( fp );
}

/** generate additional cache levels for a peakfile with only 1 block (ie. that of a new capture) */
bool
Peaks::Builder::make_peaks_mipmap ( void )
{
    if ( ! Peaks::mipmapped_peakfiles )
        return false;

    Audio_File *_clip = _peaks->_clip;

    const char *filename = _clip->filename();
    char *pn = peakname( filename );

    FILE *rfp;

    if ( ! ( rfp = fopen( pn, "r" ) ) )
    {
        WARNING( "could not open peakfile for reading: %s.", strerror( errno ) );
        free( pn );
        return false;
    }

    {
        peakfile_block_header bh;

        fread( &bh, sizeof( peakfile_block_header ), 1, rfp );

        if ( bh.skip )
        {
            WARNING( "Peakfile already has multiple blocks..." );
            fclose( rfp );
            free( pn );
            return false;
        }

    }

    last_block_pos = sizeof( peakfile_block_header );

    /* open for reading */
    //    rfp = fopen( peakname( filename ), "r" );

    /* open the file again for appending */
    if ( ! ( fp = fopen( pn, "r+" ) ) )
    {
        WARNING( "could not open peakfile for appending: %s.", strerror( errno ) );
        fclose( rfp );
        free( pn );
        return false;
    }

    free( pn );

    if ( fseeko( fp, 0, SEEK_END ) )
        FATAL( "error performing seek: %s", strerror( errno ) );

    if ( ftello( fp ) == sizeof( peakfile_block_header ) )
    {
        DWARNING( "truncated peakfile. Programming error?" );
        fclose( rfp );
        return false;
    }

    Peak buf[ _clip->channels() ];

    /* now build the remaining peak levels, each based on the
     * preceding level */

    nframes_t cs = Peaks::cache_minimum << Peaks::cache_step;

    for ( int i = 1; i < Peaks::cache_levels; ++i, cs <<= Peaks::cache_step )
    {
        DMESSAGE( "building level %d peak cache cs=%i", i + 1, cs );

        /*         DMESSAGE( "%lu", _clip->length() / cs ); */

        if ( _clip->length() / cs < 1 )
        {
            DMESSAGE( "source not long enough for any peaks at chunksize %lu", cs );
            break;
        }

        Peakfile pf;

        /* open the peakfile for the previous cache level */

        pf.open( rfp, _clip->channels(), cs >> Peaks::cache_step );

        //        pf.open( _clip->filename(), _clip->channels(), cs >> Peaks::cache_step );

        write_block_header( cs );

        off_t len;
        nframes_t s = 0;
        do
        {
            len = pf.read_peaks( buf, s, 1, cs );

            s += cs;

            fwrite( buf, sizeof( buf ), len, fp );
        }
        while ( len > 0 && s < _clip->length() );

        DMESSAGE( "Last sample was %lu", (unsigned long)s );

        pf.leave_open();
    }

    fclose( rfp );
    fclose( fp );

    DMESSAGE( "done" );

    return true;
}

bool
Peaks::Builder::make_peaks ( void )
{
    Audio_File *_clip = _peaks->_clip;

    const char *filename = _clip->filename();

    if ( _peaks->_peakfile && _peaks->_peakfile->nblocks() > 1 )
    {
        /* this peakfile already has enough blocks */
        return false;
    }
    else
    {
        DMESSAGE( "building peaks for \"%s\"", filename );

        char *pn = peakname( filename );

        if ( ! ( fp  = fopen( pn, "w+" ) ) )
        {
            free( pn );
            return false;
        }

        free( pn );

        _clip->seek( 0 );

        Peak buf[ _clip->channels() ];

        DMESSAGE( "building level 1 peak cache" );

        write_block_header( Peaks::cache_minimum );

        /* build first level from source */
        off_t len;
        do
        {
            len = _peaks->read_source_peaks( buf, 1, Peaks::cache_minimum );

            fwrite( buf, sizeof( buf ), len, fp );
        }
        while ( len );

        fclose( fp );

        DMESSAGE( "done building peaks" );
    }

    return true;
}

Peaks::Builder::Builder ( const Peaks *peaks ) :
    fp( NULL ),
    last_block_pos( 0 ),
    _peaks( peaks )
{ }


