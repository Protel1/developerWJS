/**********************************************************************************************************************
 *                                      This file contains the variantBlob class.                                     *
 *                                                                                                                    *
 * This class is used by ProtelDevice and ProtelHost when saving or retrieving binary data (e.g a firmware image)     *
 * to/from the database using a stored procedure. The variant contains a SafeArray which, in turn, contains the data. *
 *                                                                                                                    *
 *                                         Copyright (c) Protel Inc. 2009-2010                                        *
 **********************************************************************************************************************/
#pragma once
#include "comutil.h"

class variantBlob :
    public _variant_t
 {
protected:
    SAFEARRAY* pSafeArray;
public:

    variantBlob ( BYTE* Data, int DataLength )
    {
        /**************************************************************************************************************
         * CONSTRUCTOR                                                                                                *
         **************************************************************************************************************/
        /*
         * In case DataLength is zero, we set the variant as empty.
         */
        this->vt = VT_EMPTY;
        this->parray = NULL;

        if ( DataLength > 0 )
        {
            /*
             * There is data. We create a SafeArray of bytes of the required size.
             */
            int arrayLength = DataLength;
            BYTE *ptr;
            pSafeArray = SafeArrayCreateVector ( VT_UI1, 0, arrayLength );

            /*
             * We put the data into the SafeArray.
             */
            SafeArrayAccessData ( pSafeArray, (void **)&ptr );
            CopyMemory ( ptr, Data, DataLength );
            SafeArrayUnaccessData ( pSafeArray );

            /*
             * We put the SafeArray and its type into the variant.
             */
            this->vt = VT_ARRAY | VT_UI1;
            this->parray = pSafeArray;
        }
    }

    ~variantBlob(void)
    {
        /**************************************************************************************************************
         * DESTRUCTOR                                                                                                 *
         **************************************************************************************************************/
        if ( this->parray != NULL )                                    // there is an associated SafeArray - destroy it
        {
            SafeArrayDestroy ( pSafeArray );
        }
        this->vt = VT_EMPTY;                                                                // set the variant as empty
        this->parray = NULL;
    }
 };
