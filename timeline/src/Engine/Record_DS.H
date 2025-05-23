
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

#pragma once

#include "Disk_Stream.H"

#include "Audio_File_SF.H"
class Audio_File;
class Peak_Writer;

class Record_DS : public Disk_Stream
{

    /* not permitted  */
    Record_DS ( const Record_DS &rhs );
    Record_DS & operator= ( const Record_DS &rhs );

    Track::Capture *_capture;

    nframes_t _frames_written;
    volatile nframes_t _stop_frame;
    volatile nframes_t _first_frame;

    volatile bool _recording;

    Audio_File_SF *_af;                             /* capture file */

    void write_block ( sample_t *buf, nframes_t nframes );
    void disk_thread ( void ) override;

    virtual void flush ( void ) override
    {
        base_flush( false );
    }

public:

    Record_DS ( Track *th, float frame_rate, nframes_t nframes, int channels ) :
        Disk_Stream( th, frame_rate, nframes, channels )
    {
        sem_destroy( &_blocks );
        sem_init( &_blocks, 0, 0 );

        _capture = NULL;
        _recording = false;
        _stop_frame = JACK_MAX_FRAMES;
        _frames_written = 0;
        _first_frame = 0;
        _af = NULL;
    }

    virtual ~Record_DS ( )
    {
        shutdown();
    }

    /*     bool seek_pending ( void ); */
    /*     void seek ( nframes_t frame ); */
    const Audio_Region * capture_region ( void ) const;
    Track::Capture * capture ( void );

    void start ( nframes_t frame, nframes_t start_frame, nframes_t stop_frame = 0 );
    void stop ( nframes_t frame );
    nframes_t process ( nframes_t nframes ) override;

};
