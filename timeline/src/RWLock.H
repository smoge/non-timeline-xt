
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

#include <pthread.h>

class RWLock
{

    pthread_rwlock_t _lock;

public:

    RWLock ( )
    {
//            _lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
        pthread_rwlock_init( &_lock, NULL );
    }

    virtual ~RWLock ( )
    {
        pthread_rwlock_destroy( &_lock );
    }

    void
    rdlock ( void )
    {
        pthread_rwlock_rdlock( &_lock );
    }

    void
    wrlock ( void )
    {
        pthread_rwlock_wrlock( &_lock );
    }

    void
    unlock ( void )
    {
        pthread_rwlock_unlock( &_lock );
    }

    int
    tryrdlock ( void )
    {
        return pthread_rwlock_tryrdlock( &_lock );
    }

    int
    trywrlock ( void )
    {
        return pthread_rwlock_trywrlock( &_lock );
    }

};
