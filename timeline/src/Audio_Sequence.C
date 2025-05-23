
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

/* An Audio_Sequence is a sequence of Audio_Regions. Takes and 'track
 * contents' consist of these objects */

#include "../../nonlib/debug.h"

#include <sys/time.h>
#include <FL/fl_ask.H>
#include <FL/Fl.H>

#ifdef FLTK_SUPPORT
#include <cairo.h>
#include <cairo-xlib.h>
#endif

#include "Audio_Sequence.H"
#include "Waveform.H"

#include <list>
using namespace std;

#include "Track.H"

#include "Engine/Audio_File.H" // for ::from_file()
#include "Transport.H" // for locate()
#include "Track_Header.H"
#include <errno.h>

#include <unistd.h> // for symlink()

#include "../../nonlib/string_util.h"

#ifdef FLTK14_SUPPORT
#include "TLE.H"
extern TLE *tle;
#endif

const char *
Audio_Sequence::name ( void ) const
{
    return Sequence::name();
}

void
Audio_Sequence::name ( const char *s )
{
    Sequence::name( s );
    header()->name_input->value( s );
}

void
Audio_Sequence::cb_button ( Fl_Widget *w, void *v )
{
    ((Audio_Sequence*)v)->cb_button( w );
}

void
Audio_Sequence::cb_button ( Fl_Widget *w )
{
    Logger log(this);

    if ( w == header()->name_input )
    {
        Sequence::name( header()->name_input->value() );
    }
    else if ( w == header()->delete_button )
    {
        track()->remove( this );
    }
    else if ( w == header()->promote_button )
    {
        track()->sequence( this );
    }
}

void
Audio_Sequence::init ( void )
{
    labeltype( FL_NO_LABEL );
    {
        Audio_Sequence_Header *o = new Audio_Sequence_Header( x(), y(), Track::width(), 52 );

        o->name_input->callback( cb_button, this );
        o->delete_button->callback( cb_button, this );
        o->promote_button->callback( cb_button, this );

        Fl_Group::add( o );
    }

    resizable(0);
}

Audio_Sequence::Audio_Sequence ( Track *track, const char *name ) : Sequence( track )
{
    _track = track;

    init();

    if ( name )
        Audio_Sequence::name( name );
    else
    {
        struct timeval tv;

        gettimeofday( &tv, NULL );

        time_t t = tv.tv_sec;

        char s[40];

        ctime_r( &t, s );

        s[ strlen( s ) - 1 ] = 0;

        Audio_Sequence::name( s );
    }

    if ( track )
        track->add( this );

    log_create();
}

Audio_Sequence::~Audio_Sequence ( )
{
    Loggable::block_start();

    clear();

    log_destroy();

    track()->remove( this );

    Loggable::block_end();
}



/** return a pointer to the current capture region for this sequence */
const Audio_Region *
Audio_Sequence::capture_region ( void ) const
{
    return track()->capture_region();
}

void
Audio_Sequence::get ( Log_Entry &e ) const
{
    e.add( ":track", _track );
    e.add( ":name", name() );
}

void
Audio_Sequence::set ( Log_Entry &e )
{
    for ( int i = 0; i < e.size(); ++i )
    {
        const char *s, *v;

        e.get( i, &s, &v );

        if ( ! strcmp( ":track", s ) )
        {
            unsigned int ii;
            sscanf( v, "%X", &ii );
            Track *t = static_cast<Track*>( Loggable::find( ii ) );

            assert( t );

            /* FIXME: this causes the sequences to be set twice in the replay logic, first when the sequence is created, then when track sequence is assigned. */
            t->sequence( this );
        }
        else if ( ! strcmp( ":name", s ) )
            name( v );
    }
}

void
Audio_Sequence::handle_widget_change ( nframes_t start, nframes_t length )
{
    Sequence::handle_widget_change( start, length );

    /* a region has changed. we may need to rebuffer... */

    /* trigger rebuffer */
    /* FIXME: we really only need to rebuffer *this* sequence! */
    /* FIXME: how does this fit into the selection? */

    if ( transport->rolling && ( start > transport->frame || start + length > transport->frame ) )
        transport->locate( transport->frame );
}

void
Audio_Sequence::draw ( void )
{

    Sequence::draw();

    int xfades = 0;

    fl_push_clip( drawable_x(), y(), drawable_w(), h() );

    /* draw crossfades */
    for ( list <Sequence_Widget *>::const_iterator r = _widgets.begin();  r != _widgets.end(); ++r )
    {
        Sequence_Widget *o = overlaps( *r );

        if ( o )
        {
            if ( *o <= **r )
            {
                /*                 if ( o->x() == (*r)->x() && o->w() == (*r)->w() ) */
                /*                     printf( "complete superposition\n" ); */

                //  if ( o->contains( *r ) )
                /* completely inside */
                //      continue;

                ++xfades;

                Rectangle b;
                if ( o->contains( *r ) )
                {
                    /* Completely inside, so lets show it overlap anyway, instead of hiding it,
                     * because it is hard to tell if there are more than one region if they are
                     * the same size and no highlighting or duplicated */
                    b.x = (*r)->x();
                    b.y = (*r)->y();
                    b.w = (*r)->w();
                    b.h = (*r)->h();
#if defined(FLTK_SUPPORT) || defined (FLTK14_SUPPORT)
                    // Keep overlap from scrolling before track beginning into the track header region
                    // magic number 200 = track header width
                    if (b.x < 200)
                    {
                        b.w = b.w - (200 - b.x);
                        b.x = 200;
                    }

                    // This is to keep the overlap from scrolling above the audio track into the cursor tracks
                    // magic number 165 = 24 * 5 (cursor tracks) + 45 (transport)
                    b.y = b.y < 165 ? 165 : b.y;
#endif
                }
                else
                {
                    b.x = (*r)->x();
                    b.y = o->y();
                    b.w = (o->x() + o->w()) - (*r)->x();
                    b.h = o->h();

#if defined(FLTK_SUPPORT) || defined (FLTK14_SUPPORT)
                    // Keep overlap from scrolling before track beginning into the track header region
                    if (b.x < 200)
                    {
                        b.w = b.w - (200 - b.x);
                        b.x = 200;
                    }

                    // This is to keep the overlap from scrolling above the audio track into the cursor tracks
                    b.y = b.y < 165 ? 165 : b.y;
#endif
                }
                // This is for overlapping audio regions - changes color yellow shade on overlap
                if ( b.w > 0 )
                {
#ifdef FLTK_SUPPORT
                    cairo_surface_t* Xsurface = cairo_xlib_surface_create
                        (fl_display, fl_window, fl_visual->visual,
                        Fl_Window::current()->w(), Fl_Window::current()->h());

                    cairo_t *cc = cairo_create (Xsurface);
                    cairo_set_source_rgba( cc, 1, 1, 0, 0.35 );
                    cairo_rectangle( cc, b.x, b.y, b.w, b.h );

                    cairo_fill( cc );

                    cairo_set_operator( cc, CAIRO_OPERATOR_OVER );

                    cairo_surface_destroy(Xsurface);
                    cairo_destroy(cc);
#elif defined(FLTK14_SUPPORT)
                    cairo_t *cc = Fl::cairo_make_current(tle->main_window);

                    cairo_set_operator( cc, CAIRO_OPERATOR_HSL_COLOR );
                    cairo_set_source_rgba( cc, 1, 1, 0, 0.80 );
                    cairo_rectangle( cc, b.x, b.y, b.w, b.h );

                    cairo_fill( cc );

                    cairo_set_operator( cc, CAIRO_OPERATOR_OVER );
#else   // NTK
                    cairo_t *cc = Fl::cairo_cc();

                    cairo_set_operator( cc, CAIRO_OPERATOR_HSL_COLOR );
                    cairo_set_source_rgba( cc, 1, 1, 0, 0.80 );
                    cairo_rectangle( cc, b.x, b.y, b.w, b.h );

                    cairo_fill( cc );

                    cairo_set_operator( cc, CAIRO_OPERATOR_OVER );
#endif
                }
            }
        }

    }

    fl_pop_clip();
}

int
Audio_Sequence::handle_paste ( const char *text )
{
    int X = Fl::event_x();

    if ( ! strcmp( text, "Audio_Region" ) )
        return 1;

    char *file;

    if ( ! sscanf( text, "file://%m[^\r\n]\n", &file ) )
    {
        WARNING( "invalid drop \"%s\"\n", text );
        return 0;
    }

    unescape_url( file );

    printf( "pasted file \"%s\"\n", file );

    fl_cursor( FL_CURSOR_WAIT );
    Fl::check();

    char *t = strdup( file );
    if(!t)
        return 0;

    char *filebase = strdup( basename( t ) );

    free( t );

    if(!filebase)
        return 0;

    char *s = 0;

    int i = 0;

    for ( ; ; i++ )
    {
        if ( i )
        {
            free( s );
            asprintf( &s, "sources/%s-%i", filebase, i );
        }
        else
            asprintf( &s, "sources/%s", filebase );

        DMESSAGE( "Symlink %s -> %s", file, s );
        if ( symlink( file, s ) == 0 )
            break;

        if ( errno != EEXIST )
        {
            WARNING( "Failed to create symlink: %s", strerror( errno ) );
            break;
        }
    }

    Audio_File *c = Audio_File::from_file( basename( s ) );

    free( s );
    free( filebase );

    fl_cursor( FL_CURSOR_DEFAULT );
    Fl::check();

    if ( ! c || c->dummy() )
    {
        fl_alert( "Could not import file \"%s\"", file );
        free( file );

        if ( c )
        {
            delete c;
            c = NULL;
        }

        return 0;
    }

    free( file );

    Audio_Region *r =
        new Audio_Region( c, this, timeline->xoffset + timeline->x_to_ts( X - drawable_x() ) );

    r->log_create();

    redraw();

    return 1;

}

/** event handler that supports DND of audio clips */
int
Audio_Sequence::handle ( int m )
{
    switch ( m )
    {
        case FL_PASTE:
        {
            DMESSAGE("Got sequence paste");

            if ( ! Fl::event_inside( this ) )
            {
                DMESSAGE("ignoring");
                return 0;
            }

            return handle_paste(Fl::event_text());
        }
        default:
            return Sequence::handle( m );
    }
}
