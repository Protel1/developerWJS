/**********************************************************************************************************************
 *                                      This file contains the CProtelList class.                                     *
 *                                                                                                                    *
 * This maintains a double-linked list of pointers to ProtelHost instances (each instance represents a remote device  *
 * that the server may be communicating with).                                                                        *
 *                                                                                                                    *
 * Application.h constructs the ProtelList, adds a ProtelSerial (inheriting ProtelHost) to it for each modem in the   *
 * system, then sets up SocketListener, passing a pointer to the ProtelList to it. When a socket connection is        *
 * established, SocketListener adds a ProtelHost for it to the ProtelList. SocketListener removes its ProtelHosts     *
 * from the list as the connections are closed but the modem ProtelSerials remain in the list until the Application   *
 * ends.                                                                                                              *
 *                                                                                                                    *
 * NOTE: there is at least a potential risk that the Next or Previous method is used to obtain a pointer to a         *
 * ProtelHost which is then deleted by another thread using the Remove or RemoveAll methods. !!!!Is this a problem?   *
 *                                                                                                                    *
 *                                         Copyright (c) Protel Inc. 2009-2010                                        *
 **********************************************************************************************************************/

#pragma once
#include "ProtelHost.h"

class CProtelList
 {
public:
    CProtelList(void)
    {
        /**************************************************************************************************************
         * CONSTRUCTOR.                                                                                               *
         **************************************************************************************************************/
        InitializeCriticalSection ( &criticalSection );
        m_sProtelList = NULL;                                              // first entry in list (there isn't one yet)
        m_sProtelListCurrent = NULL;                                                           // current entry in list
        //m_hMutex = CreateMutex( NULL, TRUE, NULL );
    }


    virtual ~CProtelList(void)
    {
        /**************************************************************************************************************
         * DESTRUCTOR.                                                                                                *
         **************************************************************************************************************/
        //CloseHandle ( m_hMutex );
        DeleteCriticalSection ( &criticalSection );
    }

    void Add ( CProtelHost* client )
    {
        /**************************************************************************************************************
         * This adds client to the list after the current position and sets the current list position to the new      *
         * entry.                                                                                                     *
         **************************************************************************************************************/
        //LockMutex();
        EnterCriticalSection ( &criticalSection );

        /*
         * We create and initalise the new list entry, setting it to point to client.
         */
        SProtelList* sProtelList = new SProtelList;
        sProtelList->next = NULL;
        sProtelList->previous = NULL;
        sProtelList->protelHost = client;

        if ( m_sProtelList == NULL )
        {
            /*
             * This is the first entry added to the list.
             */
            m_sProtelList = sProtelList;
        }

        if ( m_sProtelListCurrent != NULL )
        {
            /*
             * There is already an entry at the current list position. We set its forward link to point to the new
             * entry and the new entry's backward link to point to the current list entry.
             */
            sProtelList->next = m_sProtelListCurrent->next;                                     // This was missing!!!!
            if ( sProtelList->next != NULL )
            {
                sProtelList->next->previous = sProtelList;                                           // So was this!!!!
            }
            m_sProtelListCurrent->next = sProtelList;
            sProtelList->previous = m_sProtelListCurrent;
        }

        m_sProtelListCurrent = sProtelList;                               // new entry is now the current list position
        //ReleaseMutex ( m_hMutex );
        LeaveCriticalSection ( &criticalSection );
    }

    void Remove ( CProtelHost* client )
    {
        /**************************************************************************************************************
         * This deletes client after removing it from the list. (If client isn't in the list, it does nothing.)       *
         **************************************************************************************************************/
        //LockMutex();
        EnterCriticalSection ( &criticalSection );

        SProtelList* sProtelList = m_sProtelList;                                                  // beginning of list
        while ( sProtelList != NULL )                                                           // find entry to remove
        {
            if ( sProtelList->protelHost == client )                                                      // this is it
            {
                if ( sProtelList->previous == NULL )                                           // it is the first entry
                {
                    m_sProtelList = sProtelList->next;                                            // adjust first entry
                }
                else                                                                        // it isn't the first entry
                {
                    sProtelList->previous->next = sProtelList->next;              // adjust previous entry forward link
                }
                if ( sProtelList->next != NULL )                                             // it isn't the last entry
                {
                    sProtelList->next->previous = sProtelList->previous;             // adjust next entry backward link
                }
                // This was missing!!!!
                if ( m_sProtelListCurrent == sProtelList )                                   // it is the current entry
                {
                    m_sProtelListCurrent = ( sProtelList->previous != NULL )
                        ? sProtelList->previous : sProtelList->next;                            // adjust current entry
                }
                delete sProtelList->protelHost;                                                    // delete the client
                delete sProtelList;                                                               // and the list entry
                break;                                                                                   // we are done
            }
            sProtelList = sProtelList->next;                                                     // try next list entry
        }
        //ReleaseMutex ( m_hMutex );
        LeaveCriticalSection ( &criticalSection );
    }

    void RemoveAll ( void )
    {
        /**************************************************************************************************************
         * This removes all clients from the list and deletes them.                                                   *
         **************************************************************************************************************/
        //LockMutex();
        EnterCriticalSection ( &criticalSection );

        SProtelList* protelList = m_sProtelList;                                                   // beginning of list
        while ( protelList != NULL )                                                          // there is another entry
        {
            SProtelList* protelCurrent = protelList;
            protelList = protelList->next;                                                                // next entry
            delete protelCurrent->protelHost;                                                      // delete the client
            delete protelCurrent;                                                                 // and the list entry
        }
        m_sProtelList = NULL;                                                                      // list is now empty
        m_sProtelListCurrent = NULL;
        //ReleaseMutex ( m_hMutex );
        LeaveCriticalSection ( &criticalSection );
    }

    CProtelHost* MoveFirst ( void )
    {
        /**************************************************************************************************************
         * This moves the current list position to the first entry and returns its ProtelHost (NULL if there isn't    *
         * one).                                                                                                      *
         **************************************************************************************************************/
        m_sProtelListCurrent = m_sProtelList;                                                      // beginning of list
        if ( m_sProtelListCurrent != NULL )                                                         // list isn't empty
        {
            return m_sProtelListCurrent->protelHost;
        }
        return NULL;                                                                                   // list is empty
    }

    CProtelHost* Next ( void )
    {
        /**************************************************************************************************************
         * This moves the current list position to the next entry and returns its ProtelHost. If there isn't another  *
         * entry, it leaves the postion unchanged and returns NULL.                                                   *
         **************************************************************************************************************/

        if ( m_sProtelList && m_sProtelListCurrent->next != NULL ) // there is another entry Empty list check added!!!!
        {
            m_sProtelListCurrent = m_sProtelListCurrent->next;
            return m_sProtelListCurrent->protelHost;
        }
        return NULL;
    }

    CProtelHost* Previous ( void )
    {
        /**************************************************************************************************************
         * This moves the current list position to the previous entry and returns its ProtelHost. If there isn't a    *
         * previous entry, it leaves the postion unchanged and returns NULL.                                          *
         **************************************************************************************************************/

        if ( m_sProtelList && m_sProtelListCurrent->previous != NULL ) // there is a previous entry Empty list check added!!!!
        {
            m_sProtelListCurrent = m_sProtelListCurrent->previous;
            return m_sProtelListCurrent->protelHost;
        }
        return NULL;
    }


private:
    struct SProtelList                                                                                  // a list entry
    {
        CProtelHost* protelHost;                                                               // the listed ProtelHost
        SProtelList* previous;                                                                         // backward link
        SProtelList* next;                                                                              // forward link
    };
    SProtelList* m_sProtelList;                                                        // points to first entry in list
    SProtelList* m_sProtelListCurrent;                                               // points to current entry in list
    CRITICAL_SECTION criticalSection;
    HANDLE m_hMutex;
    void LockMutex ( void )                                                                       // not currently used
    {
        BOOL bWait = SetEvent ( m_hMutex );
        while ( bWait == FALSE )
        {
            WaitForSingleObject( m_hMutex, INFINITE );
            bWait = SetEvent ( m_hMutex );
        }
    }
 };
