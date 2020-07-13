/*
 * FreeRTOS PKCS #11 PAL for STM32L4 Discovery kit IoT node V1.0.3
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
 */


/**
 * @file iot_pkcs11_pal.c
 * @brief FreeRTOS device specific helper functions for
 * PKCS#11 implementation based on mbedTLS.  This
 * file deviates from the FreeRTOS style standard for some function names and
 * data types in order to maintain compliance with the PKCS#11 standard.
 */

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "FreeRTOSIPConfig.h"
#include "task.h"
#include "iot_pkcs11.h"
#include "iot_pkcs11_config.h"

/* C runtime includes. */
#include <stdio.h>
#include <string.h>

/* flash driver includes. */
#include "flash.h"

/* mbedTLS includes. */
#include "mbedtls/pk.h"
#include "mbedtls/base64.h"
#include "mbedtls/platform.h"

#define pkcs11OBJECT_MAX_SIZE                   2048
#define pkcs11OBJECT_PRESENT_MAGIC              ( 0xABCD0000uL )
#define pkcs11OBJECT_LENGTH_MASK                ( 0x0000FFFFuL )
#define pkcs11OBJECT_PRESENT_MASK               ( 0xFFFF0000uL )

enum eObjectHandles
{
    eInvalidHandle = 0, /* From PKCS #11 spec: 0 is never a valid object handle.*/
    eAwsDevicePrivateKey = 1,
    eAwsDevicePublicKey,
    eAwsDeviceCertificate,
    eAwsCodeSigningKey
};

/**
 * @brief Structure for certificates/key storage.
 */
typedef struct
{
    CK_CHAR cDeviceCertificate[ pkcs11OBJECT_MAX_SIZE ];
    CK_CHAR cDeviceKey[ pkcs11OBJECT_MAX_SIZE ];
    CK_CHAR cCodeSignKey[ pkcs11OBJECT_MAX_SIZE ];
    uint32_t ulDeviceCertificateMark;
    uint32_t ulDeviceKeyMark;
    uint32_t ulCodeSignKeyMark;
} P11KeyConfig_t;


/**
 * @brief Certificates/key storage in flash.
 */
P11KeyConfig_t P11KeyConfig __attribute__( ( section( "UNINIT_FIXED_LOC" ) ) );



/*-----------------------------------------------------------*/

/**
 * @brief Saves an object in non-volatile storage.
 *
 * Port-specific file write for cryptographic information.
 *
 * @param[in] pxLabel       The label of the object to be stored.
 * @param[in] pucData       The object data to be saved
 * @param[in] pulDataSize   Size (in bytes) of object data.
 *
 * @return The object handle if successful.
 * eInvalidHandle = 0 if unsuccessful.
 */
CK_OBJECT_HANDLE PKCS11_PAL_SaveObject( CK_ATTRIBUTE_PTR pxLabel,
                                        uint8_t * pucData,
                                        uint32_t ulDataSize )

{
    CK_OBJECT_HANDLE xHandle = eInvalidHandle;
    CK_RV xBytesWritten = 0;
    uint32_t ulFlashMark = ( pkcs11OBJECT_PRESENT_MAGIC | ( ulDataSize ) );

    if( ulDataSize <= pkcs11OBJECT_MAX_SIZE )
    {
        /*
         * write client certificate.
         */
        if( strcmp( pxLabel->pValue,
                    pkcs11configLABEL_DEVICE_CERTIFICATE_FOR_TLS ) == 0 )
        {
            xBytesWritten = FLASH_update( ( uint32_t ) P11KeyConfig.cDeviceCertificate,
                                          pucData,
                                          ( ulDataSize ) );

            if( xBytesWritten == ( ulDataSize ) )
            {
                xHandle = eAwsDeviceCertificate;

                /*change flash written mark'*/
                FLASH_update( ( uint32_t ) &P11KeyConfig.ulDeviceCertificateMark,
                              &ulFlashMark,
                              sizeof( uint32_t ) );
            }
        }

        /*
         * write client key.
         */

        else if( strcmp( pxLabel->pValue,
                         pkcs11configLABEL_DEVICE_PRIVATE_KEY_FOR_TLS ) == 0 )
        {
            xBytesWritten = FLASH_update( ( uint32_t ) P11KeyConfig.cDeviceKey,
                                          pucData,
                                          ulDataSize );

            if( xBytesWritten == ( ulDataSize ) )
            {
                xHandle = eAwsDevicePrivateKey;
                /*change flash written mark'*/
                FLASH_update( ( uint32_t ) &P11KeyConfig.ulDeviceKeyMark,
                              &ulFlashMark,
                              sizeof( uint32_t ) );
            }
        }

        else if( strcmp( pxLabel->pValue,
                         pkcs11configLABEL_CODE_VERIFICATION_KEY ) == 0 )
        {
            xBytesWritten = FLASH_update( ( uint32_t ) P11KeyConfig.cCodeSignKey,
                                          pucData,
                                          ulDataSize );

            if( xBytesWritten == ( ulDataSize ) )
            {
                xHandle = eAwsCodeSigningKey;

                /*change flash written mark'*/
                FLASH_update( ( uint32_t ) &P11KeyConfig.ulCodeSignKeyMark,
                              &ulFlashMark,
                              sizeof( uint32_t ) );
            }
        }
    }

    return xHandle;
}



/*-----------------------------------------------------------*/

/**
 * @brief Translates a PKCS #11 label into an object handle.
 *
 * Port-specific object handle retrieval.
 *
 *
 * @param[in] pLabel         Pointer to the label of the object
 *                           who's handle should be found.
 * @param[in] usLength       The length of the label, in bytes.
 *
 * @return The object handle if operation was successful.
 * Returns eInvalidHandle if unsuccessful.
 */

CK_OBJECT_HANDLE PKCS11_PAL_FindObject( uint8_t * pLabel,
                                        uint8_t usLength )
{
    CK_OBJECT_HANDLE xHandle = eInvalidHandle;

    if( ( 0 == memcmp( pLabel, pkcs11configLABEL_DEVICE_CERTIFICATE_FOR_TLS, usLength ) ) &&
        ( ( P11KeyConfig.ulDeviceCertificateMark & pkcs11OBJECT_PRESENT_MASK ) == pkcs11OBJECT_PRESENT_MAGIC ) )
    {
        xHandle = eAwsDeviceCertificate;
    }
    else if( ( 0 == memcmp( pLabel, pkcs11configLABEL_DEVICE_PRIVATE_KEY_FOR_TLS, usLength ) ) &&
             ( ( P11KeyConfig.ulDeviceKeyMark & pkcs11OBJECT_PRESENT_MASK ) == pkcs11OBJECT_PRESENT_MAGIC ) )
    {
        xHandle = eAwsDevicePrivateKey;
    }
    else if( ( 0 == memcmp( pLabel, pkcs11configLABEL_CODE_VERIFICATION_KEY, usLength ) ) &&
             ( ( P11KeyConfig.ulCodeSignKeyMark & pkcs11OBJECT_PRESENT_MASK ) == pkcs11OBJECT_PRESENT_MAGIC ) )
    {
        xHandle = eAwsCodeSigningKey;
    }

    return xHandle;
}

/**
 * @brief Gets the value of an object in storage, by handle.
 *
 * Port-specific file access for cryptographic information.
 *
 * This call dynamically allocates the buffer which object value
 * data is copied into.  PKCS11_PAL_GetObjectValueCleanup()
 * should be called after each use to free the dynamically allocated
 * buffer.
 *
 * @sa PKCS11_PAL_GetObjectValueCleanup
 *
 * @param[in] pcFileName    The name of the file to be read.
 * @param[out] ppucData     Pointer to buffer for file data.
 * @param[out] pulDataSize  Size (in bytes) of data located in file.
 * @param[out] pIsPrivate   Boolean indicating if value is private (CK_TRUE)
 *                          or exportable (CK_FALSE)
 *
 * @return CKR_OK if operation was successful.  CKR_KEY_HANDLE_INVALID if
 * no such object handle was found, CKR_DEVICE_MEMORY if memory for
 * buffer could not be allocated, CKR_FUNCTION_FAILED for device driver
 * error.
 */

CK_RV PKCS11_PAL_GetObjectValue( CK_OBJECT_HANDLE xHandle,
                                 uint8_t ** ppucData,
                                 uint32_t * pulDataSize,
                                 CK_BBOOL * pIsPrivate )

{
    CK_RV ulReturn = CKR_OBJECT_HANDLE_INVALID;

    /*
     * Read client certificate.
     */

    if( xHandle == eAwsDeviceCertificate )
    {
        /*
         * return reference and size only if certificates are present in flash
         */
        if( ( P11KeyConfig.ulDeviceCertificateMark & pkcs11OBJECT_PRESENT_MASK ) == pkcs11OBJECT_PRESENT_MAGIC )
        {
            *ppucData = P11KeyConfig.cDeviceCertificate;
            *pulDataSize = ( uint32_t ) P11KeyConfig.ulDeviceCertificateMark & pkcs11OBJECT_LENGTH_MASK;
            *pIsPrivate = CK_FALSE;
            ulReturn = CKR_OK;
        }
    }

    /*
     * Read client key.
     */

    else if( xHandle == eAwsDevicePrivateKey )
    {
        /*
         * return reference and size only if certificates are present in flash
         */
        if( ( P11KeyConfig.ulDeviceKeyMark & pkcs11OBJECT_PRESENT_MASK ) == pkcs11OBJECT_PRESENT_MAGIC )
        {
            *ppucData = P11KeyConfig.cDeviceKey;
            *pulDataSize = ( uint32_t ) ( P11KeyConfig.ulDeviceKeyMark & pkcs11OBJECT_LENGTH_MASK );
            *pIsPrivate = CK_TRUE;
            ulReturn = CKR_OK;
        }
    }

    else if( xHandle == eAwsDevicePublicKey )
    {
        /*
         * return reference and size only if certificates are present in flash
         */
        if( ( P11KeyConfig.ulDeviceKeyMark & pkcs11OBJECT_PRESENT_MASK ) == pkcs11OBJECT_PRESENT_MAGIC )
        {
            *ppucData = P11KeyConfig.cDeviceKey;
            *pulDataSize = ( uint32_t ) P11KeyConfig.ulDeviceKeyMark & pkcs11OBJECT_LENGTH_MASK;
            *pIsPrivate = CK_FALSE;
            ulReturn = CKR_OK;
        }
    }

    else if( xHandle == eAwsCodeSigningKey )
    {
        if( ( P11KeyConfig.ulCodeSignKeyMark & pkcs11OBJECT_PRESENT_MASK ) == pkcs11OBJECT_PRESENT_MAGIC )
        {
            *ppucData = P11KeyConfig.cCodeSignKey;
            *pulDataSize = ( uint32_t ) P11KeyConfig.ulCodeSignKeyMark & pkcs11OBJECT_LENGTH_MASK;
            *pIsPrivate = CK_FALSE;
            ulReturn = CKR_OK;
        }
    }

    return ulReturn;
}

/*-----------------------------------------------------------*/

/**
 * @brief Cleanup after PKCS11_GetObjectValue().
 *
 * @param[in] pucData       The buffer to free.
 *                          (*ppucData from PKCS11_PAL_GetObjectValue())
 * @param[in] ulDataSize    The length of the buffer to free.
 *                          (*pulDataSize from PKCS11_PAL_GetObjectValue())
 */
void PKCS11_PAL_GetObjectValueCleanup( uint8_t * pucData,
                                       uint32_t ulDataSize )
{
    /* Unused parameters. */
    ( void ) pucData;
    ( void ) ulDataSize;

    /* Since no buffer was allocated on heap, there is no cleanup
     * to be done. */
}
/*-----------------------------------------------------------*/
