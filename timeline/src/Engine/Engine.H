
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

#include "../../../nonlib/Mutex.H"

class Port;

#include "../../../nonlib/JACK/Client.H"

#include "../../../nonlib/Thread.H"

class Engine : public JACK::Client
{
    Thread _thread;                                            /* only used for thread checking */

    int _buffers_dropped;                                       /* buffers dropped because of locking */
    /*     int _buffers_dropped;                                       /\* buffers dropped because of locking *\/ */

    void shutdown ( void ) override;
    int process ( nframes_t nframes ) override;
    int sync ( jack_transport_state_t state, jack_position_t *pos ) override;
    int xrun ( void ) override;
    void timebase ( jack_transport_state_t state, jack_nframes_t nframes, jack_position_t *pos, int new_pos ) override;
    void freewheel ( bool yes ) override;
    int buffer_size ( nframes_t nframes ) override;
    void thread_init ( void ) override;

    Engine ( const Engine &rhs );
    Engine & operator = ( const Engine &rhs );

    void request_locate ( nframes_t frame );

private:

    friend class Port;
    friend class Transport;

public:

    Engine ( );
    virtual ~Engine ( );

    int dropped ( void ) const
    {
        return _buffers_dropped;
    }

    nframes_t system_latency ( void ) const
    {
        return nframes();
    }
    nframes_t playback_latency ( void ) const;

    float frames_to_milliseconds ( nframes_t frames )
    {
        return ( frames * 1000 ) / (float)sample_rate();
    }
};

extern Engine * engine;
