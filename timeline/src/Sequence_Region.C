
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

#include "Sequence_Region.H"
#include "Track.H"
#include "../../nonlib/debug.h"

#include <stdint.h>



Sequence_Region::Sequence_Region ( )
{
    color( FL_CYAN );
}

Sequence_Region::Sequence_Region ( const Sequence_Region &rhs ) : Sequence_Widget( rhs )
{
}

Sequence_Region::~Sequence_Region ( )
{
}



void
Sequence_Region::get ( Log_Entry &e ) const
{
    e.add( ":color",  _box_color );
    e.add( ":length", _r->length );

    Sequence_Widget::get( e );
}

void
Sequence_Region::set ( Log_Entry &e )
{
    for ( int i = 0; i < e.size(); ++i )
    {
        const char *s, *v;

        e.get( i, &s, &v );

        if ( ! strcmp( s, ":color" ) )
            _box_color = (Fl_Color)atoll( v );
        else if ( ! strcmp( s, ":length" ) )
            _r->length = atoll( v );

    }

    Sequence_Widget::set( e );
}

void
Sequence_Region::trim_left ( nframes_t where )
{
    nframes_t f = where;

    /* snap to beat/bar lines if not bypassed */
    if ( timeline->nearest_line( &f ) && !Timeline::snap_toggle_bypass )
        where = f;

    if ( where > _r->start + _r->length )
        where = _r->start + _r->length;

    if ( where < _r->start && _r->offset < _r->start - where )
        where = _r->start - _r->offset;

    _r->set_left( where );
}

void
Sequence_Region::trim_right ( nframes_t where )
{
    nframes_t f = where;

    /* snap to beat/bar lines if not bypassed */
    if ( timeline->nearest_line( &f ) && !Timeline::snap_toggle_bypass )
        where = f;

    if ( where < _r->start )
        where = _r->start;

    _r->set_right( where );
}

void
Sequence_Region::trim ( enum trim_e t, int X )
{
    redraw();

    nframes_t where = timeline->x_to_offset( X );

    switch ( t )
    {
        case LEFT:
            trim_left( where );
            break;
        case RIGHT:
            trim_right( where );
            break;
        default:
            break;
    }
}

/** split region at absolute frame /where/. due to inheritance issues,
 * the copy must be made in the derived classed and passed in */
void
Sequence_Region::split ( Sequence_Region * copy, nframes_t where )
{
    trim_right( where );
    copy->trim_left( where );
    sequence()->add( copy );
    copy->redraw();
}

#include "../../FL/test_press.H"

int
Sequence_Region::handle ( int m )
{
    static enum trim_e trimming;

    int X = Fl::event_x();
    int Y = Fl::event_y();

    if ( !active_r() )
        /* don't mess with anything while recording... */
        return 0;

    Logger _log( this );

    switch ( m )
    {
        case FL_PUSH:
        {
            /* trimming */
            if ( Fl::event_shift() && ! Fl::event_ctrl() )
            {
                switch ( Fl::event_button() )
                {
                    case 1:
                        trim( trimming = LEFT, X );
                        begin_drag( Drag( X, Y ) );
                        _log.hold();
                        break;
                    case 3:
                        trim( trimming = RIGHT, X );
                        begin_drag( Drag( X, Y) );
                        _log.hold();
                        break;
                    default:
                        return 0;
                        break;
                }

                fl_cursor( FL_CURSOR_WE );
                return 1;
            }
            else if ( test_press( FL_BUTTON2 ) )
            {
                if ( Sequence_Widget::current() == this )
                {
                    if ( selected() )
                        deselect();
                    else
                        select();
                }

                redraw();
                return 1;
            }

            /*             else if ( test_press( FL_CTRL + FL_BUTTON1 ) ) */
            /*             { */
            /*                 /\* duplication *\/ */
            /*                 fl_cursor( FL_CURSOR_MOVE ); */
            /*                 return 1; */
            /*             } */

            else
                return Sequence_Widget::handle( m );
        }
        case FL_RELEASE:
        {
            Sequence_Widget::handle( m );

            trimming = NO;

            return 1;
        }
        case FL_DRAG:
        {
            if ( ! _drag )
            {
                begin_drag( Drag( X, Y, x_to_offset( X ) ) );
                _log.hold();
            }

            /* trimming */
            if ( Fl::event_shift() )
            {
                if ( trimming )
                {
                    trim( trimming, X );
                    return 1;
                }
                else
                    return 0;
            }

            return Sequence_Widget::handle( m );
        }
        default:
            return Sequence_Widget::handle( m );
            break;
    }

    return 0;

}

void
Sequence_Region::draw_box ( void )
{
    Fl_Color c = selected() ? selection_color() : box_color();
#if defined(FLTK_SUPPORT) || defined (FLTK14_SUPPORT)
    fl_draw_box( box(), x(), y(), w(), h(), c );
#else
    fl_draw_box( box(), x(), y(), w(), h(), fl_color_add_alpha( c, 127 ) );
#endif
}

void
Sequence_Region::draw ( void )
{

}

void
Sequence_Region::draw_label ( const char *label, Fl_Align align, Fl_Color /* color */, int /* xo*/, int /* yo */ )
{
    fl_color( active_r() ? FL_FOREGROUND_COLOR : fl_inactive( FL_FOREGROUND_COLOR ) );
    fl_font( FL_HELVETICA_ITALIC, 10 );
    fl_draw( label, line_x() + Fl::box_dx( box() ), y() + Fl::box_dy( box() ), abs_w() - Fl::box_dw( box() ), h() - Fl::box_dh( box() ), align );
}
