
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

// #include "../../nonlib/Loggable.H"
#include "Sequence_Region.H"


class Annotation_Region : public Sequence_Region
{

    /* not permitted */
    Annotation_Region & operator = ( const Annotation_Region &rhs );

protected:

    virtual void get ( Log_Entry &e ) const override;
    virtual void set ( Log_Entry &e ) override;

    Annotation_Region ( )
    {
    }

    Annotation_Region ( const Annotation_Region &rhs );

public:

    /* for loggable */
    LOG_CREATE_FUNC( Annotation_Region );
    SEQUENCE_WIDGET_CLONE_FUNC( Annotation_Region );

    Annotation_Region ( Sequence *track, nframes_t when, const char *name );
    virtual ~Annotation_Region ( );

    void draw_box ( void ) override;
    void draw ( void ) override;
    int handle ( int m ) override;

};
