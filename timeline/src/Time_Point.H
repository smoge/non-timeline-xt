
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

#include "Sequence_Point.H"
#include "../../nonlib/Loggable.H"

struct time_sig
{
    int beats_per_bar;
    int beat_type;

    time_sig ( ) : beats_per_bar( 0 ), beat_type( 0 )
    {
    }

    time_sig ( int bpb, int note ) : beats_per_bar( bpb ), beat_type( note )
    {
    }
};

class Time_Point : public Sequence_Point
{
    time_sig _time;

    void
    _make_label ( void )
    {
        char l[10];

        snprintf( l, sizeof(l), "%d/%d", _time.beats_per_bar, _time.beat_type );

        label( l );
    }


protected:

//    const char *class_name ( void ) { return "Time_Point"; }

    virtual void get ( Log_Entry &e ) const override;
    void set ( Log_Entry &e ) override;

    Time_Point ( );

public:

    LOG_CREATE_FUNC( Time_Point );
    SEQUENCE_WIDGET_CLONE_FUNC( Time_Point );

    static bool edit ( time_sig *sig );

    Time_Point ( nframes_t when, int bpb, int note );
    Time_Point ( const Time_Point &rhs );

    ~Time_Point ( );

    void
    time ( int bpb, int note )
    {
        _time.beats_per_bar = bpb;
        _time.beat_type = note;
        _make_label();
        redraw();
    }
    time_sig time ( void ) const
    {
        return _time;
    }

    int handle ( int m ) override;

};
