
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

#include "Audio_File_SF.H"
// #include "Timeline.H"

#include <sndfile.h>

#include <stdlib.h>
#include <string.h>

#include <assert.h>

#include "Peaks.H"

// #define HAS_SF_FORMAT_VORBIS

#include "const.h"
#include "../../../nonlib/debug.h"
#include <stdio.h>

_Pragma("GCC diagnostic push")
_Pragma("GCC diagnostic ignored \"-Wmissing-field-initializers\"")
const Audio_File::format_desc Audio_File_SF::supported_formats[] =
{
    {      "Wav 24",       "wav",   SF_FORMAT_WAV    | SF_FORMAT_PCM_24    | SF_ENDIAN_FILE },
    {      "Wav 16",       "wav",   SF_FORMAT_WAV    | SF_FORMAT_PCM_16    | SF_ENDIAN_FILE },
    {      "Wav f32",      "wav",   SF_FORMAT_WAV    | SF_FORMAT_FLOAT     | SF_ENDIAN_FILE },
    {      "W64 24",       "w64",   SF_FORMAT_W64    | SF_FORMAT_PCM_24    | SF_ENDIAN_FILE },
    {      "W64 16",       "w64",   SF_FORMAT_W64    | SF_FORMAT_PCM_16    | SF_ENDIAN_FILE },
    {      "W64 f32",      "w64",   SF_FORMAT_W64    | SF_FORMAT_FLOAT     | SF_ENDIAN_FILE },
    {      "Au 24",        "au",    SF_FORMAT_AU     | SF_FORMAT_PCM_24    | SF_ENDIAN_FILE },
    {      "Au 16",        "au",    SF_FORMAT_AU     | SF_FORMAT_PCM_16    | SF_ENDIAN_FILE },
    {      "FLAC",         "flac",  SF_FORMAT_FLAC   | SF_FORMAT_PCM_24 },
#ifdef HAVE_SF_FORMAT_VORBIS
    {      "Vorbis q10",   "ogg",   SF_FORMAT_OGG    | SF_FORMAT_VORBIS, 10 },
    {      "Vorbis q6",    "ogg",   SF_FORMAT_OGG    | SF_FORMAT_VORBIS, 6 },
    {      "Vorbis q3",    "ogg",   SF_FORMAT_OGG    | SF_FORMAT_VORBIS, 3 },
#endif
    {      0,            0          }
};
_Pragma("GCC diagnostic pop")

Audio_File_SF *
Audio_File_SF::from_file ( const char *filename )
{
    SNDFILE *in;
    SF_INFO si;

    Audio_File_SF *c = NULL;

    memset( &si, 0, sizeof( si ) );

    char *fp = path( filename );

    if ( ! ( in = sf_open( fp, SFM_READ, &si ) ) )
        return NULL;

    /*     if ( si.samplerate != timeline->sample_rate() ) */
    /*     { */
    /*         printf( "error: samplerate mismatch!\n" ); */
    /*         goto invalid; */
    /*     } */

    c = new Audio_File_SF;

    //    c->_peak_writer  = NULL;
    c->_current_read = 0;
    c->_filename     = strdup( filename );
    c->_path         = fp;
    c->_length       = si.frames;
    c->_samplerate   = si.samplerate;
    c->_channels     = si.channels;

    c->_in = in;
    //    sf_close( in );

    return c;

    //invalid:

    // sf_close( in );
    // return NULL;
}

Audio_File_SF *
Audio_File_SF::create ( const char *filename, nframes_t samplerate, int channels, const char *format )
{
    SF_INFO si;
    SNDFILE *out;

    memset( &si, 0, sizeof( si ) );

    const Audio_File::format_desc *fd = Audio_File::find_format( Audio_File_SF::supported_formats, format );

    if ( ! fd )
    {
        DMESSAGE( "Unsupported capture format: %s", format );
        return NULL;
    }

    si.samplerate =  samplerate;
    si.channels   =  channels;
    si.format = fd->id;

    char *name;
    asprintf( &name, "%s.%s", filename, fd->extension );

    char *filepath = path( name );

    if ( ! ( out = sf_open( filepath, SFM_WRITE, &si ) ) )
    {
        WARNING( "couldn't create soundfile \"%s\": libsndfile says: %s", filepath, sf_strerror(NULL) );
        free( name );
        return NULL;
    }

    if ( !strcmp( fd->extension, "ogg" ) )
    {
        /* set high quality encoding for vorbis */
        double quality = ( fd->quality + 1 ) / (float)11;

        sf_command( out, SFC_SET_VBR_ENCODING_QUALITY, &quality, sizeof( double ) );
    }

    Audio_File_SF *c = new Audio_File_SF;

    c->_path       = filepath;
    c->_filename   = name;
    c->_length     = 0;
    c->_samplerate = samplerate;
    c->_channels   = channels;

    c->_in         = out;

    c->_peaks.prepare_for_writing();

    return c;
}

bool
Audio_File_SF::open ( void )
{
    SF_INFO si;

    assert( _in == NULL );

    memset( &si, 0, sizeof( si ) );

    if ( ! ( _in = sf_open( _path, SFM_READ, &si ) ) )
        return false;

    _current_read = 0;
    _length       = si.frames;
    _samplerate   = si.samplerate;
    _channels     = si.channels;

    //    seek( 0 );
    return true;
}

void
Audio_File_SF::close ( void )
{
    if ( _in )
        sf_close( _in );

    _in = NULL;
}

void
Audio_File_SF::seek ( nframes_t offset )
{
    lock();

    if ( offset != _current_read )
        sf_seek( _in, _current_read = offset, SEEK_SET | SFM_READ );

    unlock();
}

/* if channels is -1, then all channels are read into buffer
 (interleaved).  buf should be big enough to hold them all */
nframes_t
Audio_File_SF::read ( sample_t *buf, int channel, nframes_t len )
{
    if ( len > 256 * 100 )
        WARNING( "warning: attempt to read an insane number of frames (%lu) from soundfile\n", (unsigned long)len );

    //    printf( "len = %lu, channels = %d\n", len, _channels );

    lock();

    nframes_t rlen;

    if ( _channels == 1 || channel == -1 )
        rlen = sf_readf_float( _in, buf, len );
    else
    {
        sample_t *tmp = new sample_t[ len * _channels ];

        rlen = sf_readf_float( _in, tmp, len );

        /* extract the requested channel */
        for ( unsigned int i = channel; i < rlen * _channels; i += _channels )
            * (buf++) = tmp[ i ];

        delete[] tmp;
    }

    _current_read += rlen;

    unlock();

    return rlen;
}

/** read samples from /start/ to /end/ into /buf/ */
nframes_t
Audio_File_SF::read ( sample_t *buf, int channel, nframes_t start, nframes_t len )
{
    lock();
    //    open();

    seek( start );

    nframes_t cnt = read( buf, channel, len );

    unlock();

    //    close();

    return cnt;
}

/** write /nframes/ from /buf/ to soundfile. Should be interleaved for
 * the appropriate number of channels */
nframes_t
Audio_File_SF::write ( sample_t *buf, nframes_t nframes )
{
    _peaks.write( buf, nframes );

    lock();

    nframes_t l = sf_writef_float( _in, buf, nframes );

    _length += l;

    unlock();

    return l;
}
