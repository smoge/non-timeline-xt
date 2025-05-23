
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

/* Routings for opening/closing/creation of projects. All the actual
   project state belongs to Timeline and other classes. */

/* Project management routines. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <errno.h>

#include "../../nonlib/Loggable.H"
#include "Project.H"

#include "Timeline.H" // for sample_rate()
#include "Engine/Engine.H" // for sample_rate()
#include "TLE.H" // all this just for load and save...

#include <FL/filename.H>

#include "const.h"
#include "../../nonlib/debug.h"
#include "../../nonlib/file.h"
#include "../../nonlib/Block_Timer.H"

#include "Transport.H"

extern Transport *transport;
extern TLE *tle;

const int PROJECT_VERSION = 2;

extern char *instance_name;
extern std::string project_directory;


const char *Project::_errstr[] =
{
    "Not a Non-DAW project",
    "Locked by another process",
    "Access denied",
    "Samplerate mismatch",
    "Incompatible project version"
};

char Project::_name[256];
char Project::_created_on[40];
char Project::_path[512];
bool Project::_is_open = false;
int Project::_lockfd = 0;



/***********/
/* Private */
/***********/

void
Project::set_name ( const char *name )
{
    strcpy( Project::_name, name );

    if ( Project::_name[ strlen( Project::_name ) - 1 ] == '/' )
        Project::_name[ strlen( Project::_name ) - 1 ] = '\0';

    char *s = rindex( Project::_name, '/' );

    s = s ? s + 1 : Project::_name;

    memmove( Project::_name, s, strlen( s ) + 1 );

    for ( s = Project::_name; *s; ++s )
        if ( *s == '_' || *s == '-' )
            *s = ' ';
}

bool
Project::write_info ( void )
{
    FILE *fp;

    if ( ! ( fp = fopen( "info", "w" ) ) )
    {
        WARNING( "could not open project info file for writing." );
        return false;
    }

    char s[40];

    if ( ! *_created_on )
    {
        time_t t = time( NULL );
        ctime_r( &t, s );
        s[ strlen( s ) - 1 ] = '\0';
    }
    else
        strcpy( s, _created_on );

    fprintf( fp, "created by\n\t%s %s\ncreated on\n\t%s\nversion\n\t%d\nsample rate\n\t%lu\n",
        APP_NAME, VERSION,
        s,
        PROJECT_VERSION,
        (unsigned long)timeline->sample_rate() );

    fclose( fp );

    return true;
}

void
Project::undo ( void )
{
    Loggable::undo();
}

bool
Project::read_info ( int *version, nframes_t *sample_rate, char **creation_date, char **created_by )
{
    FILE *fp;

    if ( ! ( fp = fopen( "info", "r" ) ) )
    {
        WARNING( "could not open project info file for reading." );
        return false;
    }

    *version = 0;
    *sample_rate = 0;
    *creation_date = 0;
    *created_by = 0;

    char *name, *value;

    while ( fscanf( fp, "%m[^\n]\n\t%m[^\n]\n", &name, &value ) == 2 )
    {
        MESSAGE( "Info: %s = %s", name, value );

        if ( ! strcmp( name, "sample rate" ) )
            *sample_rate = atoll( value );
        else if ( ! strcmp( name, "version" ) )
            *version = atoi( value );
        else if ( ! strcmp( name, "created on" ) )
            *creation_date = strdup( value );
        else if ( ! strcmp( name, "created by" ) )
            *created_by = strdup( value );

        free( name );
        free( value );
    }

    fclose( fp );

    return true;
}

/**********/
/* Public */
/**********/

/** Save out any settings and unjournaled state... */
bool
Project::save ( void )
{
    if ( ! open() )
        return true;

    tle->save_timeline_settings();

    return Loggable::save_unjournaled_state();
}

/** Close the project (reclaiming all memory) */
bool
Project::close ( void )
{
    if ( ! open() )
        return true;

    if ( ! save() )
        return false;

    Loggable::close();

    //    write_info();

    _is_open = false;

    *Project::_name = '\0';
    *Project::_created_on = '\0';

    release_lock( &_lockfd, ".lock" );

    delete engine;
    engine = NULL;

    return true;
}

/** Ensure a project is valid before opening it... */
bool
Project::validate ( const char *name )
{
    bool r = true;

    char pwd[512];

    fl_filename_absolute( pwd, sizeof( pwd ), "." );

    if ( chdir( name ) )
    {
        WARNING( "Cannot change to project dir \"%s\"", name );
        return false;
    }

    if ( ! exists( "info" ) ||
        ! exists( "history" ) ||
        ! exists( "sources" ) )
        //         ! exists( "options" ) )
    {
        WARNING( "Not a Non-DAW project: \"%s\"", name );
        r = false;
    }

    chdir( pwd );

    return r;
}

void
Project::make_engine ( void )
{
    if ( engine )
        FATAL( "Engine should be null!" );

    engine = new Engine;

    if ( ! engine->init( instance_name, JACK::Client::SLOW_SYNC | JACK::Client::TIMEBASE_MASTER  ))
        FATAL( "Could not connect to JACK!" );

    timeline->sample_rate( engine->sample_rate() );

    /* always start stopped (please imagine for me a realistic
     * scenario requiring otherwise */
    transport->stop();
}

/** Try to open project /name/. Returns 0 if successful, an error code
 * otherwise */
int
Project::open ( const char *name )
{
    if ( ! validate( name ) )
        return E_INVALID;

    close();

    chdir( name );

    project_directory = name;

    if ( ! acquire_lock( &_lockfd, ".lock" ) )
        return E_LOCKED;

    int version;
    nframes_t rate;
    char *creation_date;
    char *created_by;

    if ( ! read_info( &version, &rate, &creation_date, &created_by ) )
        return E_INVALID;

    if ( strncmp( created_by, "The Non-DAW", strlen( "The Non-DAW" ) ) &&
        strncmp( created_by, "Non-DAW", strlen( "Non-DAW" ) ) &&
        strncmp( created_by, "Non-Timeline", strlen( "Non-Timeline" ) ) &&
        strncmp( created_by, APP_TITLE, strlen( APP_TITLE ) ) &&
        strncmp( created_by, APP_NAME, strlen( APP_NAME ) ) )
        return E_INVALID;

    if ( version > PROJECT_VERSION )
        return E_VERSION;

    if ( version < 2 )
    {
        /* FIXME: Version 1->2 adds Cursor_Sequence and Cursor_Point to default project... */
    }

    /* normally, engine will be NULL after a close or on an initial open,
     but 'new' will have already created it to get the sample rate. */
    if ( ! engine )
        make_engine();

    {
        Block_Timer timer( "Replayed journal" );
        if ( ! Loggable::open( "history" ) )
            return E_INVALID;
    }

    /* /\* really a good idea? *\/ */
    /* timeline->sample_rate( rate ); */

    if ( creation_date )
    {
        strcpy( _created_on, creation_date );
        free( creation_date );
    }
    else
        *_created_on = 0;

    if ( created_by )
        free( created_by );

    *_path = '\0';

    fl_filename_absolute( _path, sizeof( _path ), "." );

    set_name( _path );

    _is_open = true;

    tle->load_timeline_settings();

    timeline->zoom_fit();

    MESSAGE( "Loaded project \"%s\"", _path );

    return 0;
}

/** Create a new project /name/ from existing template
 * /template_name/ */
bool
Project::create ( const char *name, const char *template_name )
{
    if ( exists( name ) )
    {
        WARNING( "Project already exists" );
        return false;
    }

    close();

    if ( mkdir( name, 0777 ) )
    {
        WARNING( "Cannot create project directory" );
        return false;
    }

    if ( chdir( name ) )
        FATAL( "WTF? Cannot change to new project directory" );

    if ( mkdir( "sources", 0777 ) )
    {
        WARNING( "Cannot create sources directory" );
        return false;
    }

    int ret = creat( "history", 0666 );
    if ( ret < 0 )
    {
        WARNING ( "Cannot create history file: %s", strerror( errno ));
        return false;
    }

    if ( ! engine )
        make_engine();

    /* TODO: copy template */

    write_info();

    if ( open( name ) == 0 )
    {
        /* add the bare essentials */
        timeline->beats_per_minute( 0, 120 );
        timeline->time( 0, 4, 4 );
        timeline->update_window_title();

        MESSAGE( "Created project \"%s\" from template \"%s\"", name, template_name );
        return true;
    }
    else
    {
        WARNING( "Failed to open newly created project" );
        return false;
    }
}

/** Replace the journal with a snapshot of the current state */
void
Project::compact ( void )
{
    Block_Timer timer( "Compacted journal" );
    Loggable::compact();
}
