
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

#include <FL/Fl_Widget.H>
#include <FL/Fl_Group.H>
#include <FL/Fl.H>

#include <stdio.h>

#include "../../nonlib/Loggable.H"

#include <assert.h>

#include <list>
#include <queue>

class Track;
class Sequence_Widget;

#include "types.h"

/* This is the base class for all track types. */

class Sequence : public Fl_Group, public Loggable
{

    /* not permitted  */
    Sequence ( const Sequence &rhs );
    Sequence & operator= ( const Sequence &rhs );

    static std::queue <Sequence_Widget *> _delete_queue;

    void init ( void );

protected:

    Track *_track;                                              /* track this sequence belongs to */

    char *_name;

    friend class Timeline; // for draw_measure
    std::list <Sequence_Widget *> _widgets;
    Sequence_Widget *widget_at ( nframes_t ts, int Y );
    Sequence_Widget *event_widget ( void );

public:

    virtual void log_children ( void ) const override;

    /* child classes should implement this if they need to take
     special action when a widget is changed/moved/resized. /start/
     and /length/ define the affected region */
    virtual void handle_widget_change ( nframes_t start, nframes_t length );

    /* welcome to C++ */
    LOG_NAME_FUNC( Sequence );

    Sequence ( Track *track=0, const char *name = 0 );

    Sequence ( int X, int Y, int W, int H );

    virtual ~Sequence (  );

    /* override this to provide cursor  */
    virtual Fl_Cursor cursor ( void ) const = 0;

    nframes_t x_to_offset ( int X );

    const char * name ( void ) const
    {
        return _name;
    }
    void name ( const char *s )
    {
        if ( _name ) free( _name );
        _name = s ? strdup( s ) : NULL;
        label( _name );

        /* for outside label */
        if ( (Fl_Widget*)track() )
            ((Fl_Widget*)track())->redraw();
    }

    void sort ( void );
    void clear ( void );

    int drawable_x ( void ) const;
    int drawable_w ( void ) const;

    const Sequence_Widget *next ( nframes_t from ) const;
    const Sequence_Widget *prev ( nframes_t from ) const;

    Track *track ( void ) const
    {
        return _track;
    }
    void track ( Track *t )
    {
        _track = t;
    }

    void remove ( Sequence_Widget *r );
    void add ( Sequence_Widget *r );

    void select_range ( int X, int W, int Y = -1, int H = -1 );

    void log_seq_nudges();
    void nudge_selected(bool left);
    void pan_selected(bool left);

    void log_control_nudges();
    void nudge_control_selected_X(bool left);
    void nudge_control_selected_Y(bool up);

    void remove_selected ( void );

    Fl_Color color ( void ) const
    {
        return this == Fl::focus() ? fl_color_average( FL_FOREGROUND_COLOR, Fl_Widget::color(), 0.20f ) : Fl_Widget::color();
    }
    void color ( Fl_Color v )
    {
        Fl_Widget::color( v );
    }

    const std::list <Sequence_Widget *> widgets ( void ) const
    {
        return _widgets;
    }

    void queue_delete ( Sequence_Widget *r )
    {
        _delete_queue.push( r );
    }

    Sequence_Widget * overlaps ( Sequence_Widget *r );

    nframes_t length ( void ) const;

    virtual Sequence * clone ( void )
    {
        assert( 0 );
        return NULL;
    }

    virtual Sequence * clone_empty ( void )
    {
        return NULL;
    }

    virtual void snap ( Sequence_Widget *r );
    virtual int handle ( int m ) override;
    virtual void draw ( void ) override;
    virtual void draw_box ( void );

    virtual nframes_t process ( nframes_t )
    {
        return 0;
    }

};
