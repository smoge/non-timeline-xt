
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

#include "Disk_Stream.H"

class Playback_DS : public Disk_Stream
{

    void read_block ( sample_t *buf, nframes_t nframes );
    void disk_thread ( void ) override;

    void flush ( void ) override
    {
        base_flush( true );
    }

    volatile nframes_t _undelay; /* number of frames this diskstream
                                  * should be undelayed by */

public:

    Playback_DS ( Track *th, float frame_rate, nframes_t nframes, int channels ) :
        Disk_Stream( th, frame_rate, nframes, channels ),
        _undelay(0)
    {
        run();
    }

    virtual ~Playback_DS ( )
    {
        shutdown();
    }

    bool seek_pending ( void );
    void seek ( nframes_t frame );
    nframes_t process ( nframes_t nframes ) override;

    void undelay ( nframes_t v );

};
