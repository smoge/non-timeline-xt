
/*******************************************************************************/
/* Copyright (C) 2008-2021 Jonathan Moore Liles (as "Non-Timeline")            */
/* Copyright (C) 2023- Stazed                                                  */
/*                                                                             */
/* This file is part of Non-Timeline-XT                                        */
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

#include "../../nonlib/Loggable.H"
#include "Sequence_Point.H"

#include <FL/fl_ask.H>

class Annotation_Point : public Sequence_Point
{

protected:

//    const char *class_label ( void ) { return "Annotation_Point"; }

    virtual void  get ( Log_Entry &e ) const override
    {
        Sequence_Point::get( e );

        e.add( ":label", _label );
    }

    void
    set ( Log_Entry &e ) override
    {
        Sequence_Point::set( e );

        for ( int i = 0; i < e.size(); ++i )
        {
            const char *s, *v;

            e.get( i, &s, &v );

            if ( ! strcmp( s, ":label" ) )
                label( v );
        }

//            timeline->redraw();
    }

    Annotation_Point ( )
    {
    }

public:

    /* for loggable */
    LOG_CREATE_FUNC( Annotation_Point );
    SEQUENCE_WIDGET_CLONE_FUNC( Annotation_Point );

    Annotation_Point ( Sequence *sequence, nframes_t when, const char *label )
    {
        _sequence = NULL;

        _r->start = when;

        _label = strdup( label );

        sequence->add( this );

        log_create();
    }

    Annotation_Point ( const Annotation_Point &rhs ) : Sequence_Point( rhs )
    {
        log_create();
    }

    ~Annotation_Point ( )
    {
        log_destroy();
    }


    int
    handle ( int m ) override
    {
        Logger _log( this );

        if ( m == FL_PUSH && Fl::test_shortcut( FL_BUTTON3 ) && ! Fl::event_shift() )
        {
            const char *s = fl_input( "New label for mark:", label() );

            if ( s )
                label( s );

            return 0;
        }

        int r = Sequence_Widget::handle( m );

        if ( m == FL_RELEASE )
        {
            sequence()->sort();
            redraw();
        }

        return r;
    }

};
