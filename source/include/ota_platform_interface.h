/*
 * FreeRTOS OTA V2.0.0
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

#ifndef _OTA_PLATFORM_INTERFACE_
#define _OTA_PLATFORM_INTERFACE_

#include "ota_private.h"

/**
 * @brief The OTA platform interface return status.
 */
typedef enum OtaPalStatus
{
    OtaPalSuccess = 0,          /*!< OTA platform interface success. */
    OtaPalUninitialized = 0xe0, /*!< Result is not yet initialized from PAL. */
    OtaPalSignatureCheckFailed, /*!< The signature check failed for the specified file. */
    OtaPalRxFileCreateFailed,   /*!< The PAL failed to create the OTA receive file. */
    OtaPalRxFileTooLarge,       /*!< The OTA receive file is too big for the platform to support. */
    OtaPalBootInfoCreateFailed, /*!< The PAL failed to create the OTA boot info file. */
    OtaPalBadSignerCert,        /*!< The signer certificate was not readable or zero length. */
    OtaPalBadImageState,        /*!< The specified OTA image state was out of range. */
    OtaPalAbortFailed,          /*!< Error trying to abort the OTA. */
    OtaPalRejectFailed,         /*!< Error trying to reject the OTA image. */
    OtaPalCommitFailed,         /*!< The acceptance commit of the new OTA image failed. */
    OtaPalActivateFailed,       /*!< The activation of the new OTA image failed. */
    OtaPalFileAbort,            /*!< Error in low level file abort. */
    OtaPalFileClose             /*!< Error in low level file close. */
} OtaPalStatus_t;

/**
 * @brief Abort an OTA transfer.
 *
 * Aborts access to an existing open file represented by the OTA file context pFileContext. This is
 * only valid for jobs that started successfully.
 *
 * @note The input OtaFileContext_t pFileContext is checked for NULL by the OTA agent before this
 * function is called.
 * This function may be called before the file is opened, so the file pointer pFileContext->fileHandle
 * may be NULL when this function is called.
 *
 * @param[in] pFileContext OTA file context information.
 *
 * @return The OTA PAL layer error code combined with the MCU specific error code. See OTA Agent
 * error codes information in ota.h.
 *
 * The file pointer will be set to NULL after this function returns.
 * OtaPalSuccess is returned when aborting access to the open file was successful.
 * OtaPalFileAbort is returned when aborting access to the open file context was unsuccessful.
 */
typedef OtaPalStatus_t ( * OtaPalAbort_t )( OtaFileContext_t * const pFileContext );

/**
 * @brief Create a new receive file for the data chunks as they come in.
 *
 * @note Opens the file indicated in the OTA file context in the MCU file system.
 *
 * @note The previous image may be present in the designated image download partition or file, so the
 * partition or file must be completely erased or overwritten in this routine.
 *
 * @note The input OtaFileContext_t pFileContext is checked for NULL by the OTA agent before this
 * function is called.
 * The device file path is a required field in the OTA job document, so pFileContext->pFilePath is
 * checked for NULL by the OTA agent before this function is called.
 *
 * @param[in] pFileContext OTA file context information.
 *
 * @return The OTA PAL layer error code combined with the MCU specific error code. See OTA Agent
 * error codes information in ota.h.
 *
 * OtaPalSuccess is returned when file creation is successful.
 * OtaPalRxFileTooLarge is returned if the file to be created exceeds the device's non-volatile memory size constraints.
 * OtaPalBootInfoCreateFailed is returned if the bootloader information file creation fails.
 * OtaPalRxFileCreateFailed is returned for other errors creating the file in the device's non-volatile memory.
 */
typedef OtaPalStatus_t (* OtaPalCreateFileForRx_t)( OtaFileContext_t * const pFileContext );

/* @brief Authenticate and close the underlying receive file in the specified OTA context.
 *
 * @note The input OtaFileContext_t pFileContext is checked for NULL by the OTA agent before this
 * function is called. This function is called only at the end of block ingestion.
 * prvPAL_CreateFileForRx() must succeed before this function is reached, so
 * pFileContext->fileHandle(or pFileContext->pFile) is never NULL.
 * The certificate path on the device is a required job document field in the OTA Agent,
 * so pFileContext->pCertFilepath is never NULL.
 * The file signature key is required job document field in the OTA Agent, so pFileContext->pSignature will
 * never be NULL.
 *
 * If the signature verification fails, file close should still be attempted.
 *
 * @param[in] pFileContext OTA file context information.
 *
 * @return The OTA PAL layer error code combined with the MCU specific error code. See OTA Agent
 * error codes information in ota.h.
 *
 * OtaPalSuccess is returned on success.
 * OtaPalSignatureCheckFailed is returned when cryptographic signature verification fails.
 * OtaPalBadSignerCert is returned for errors in the certificate itself.
 * OtaPalFileClose is returned when closing the file fails.
 */
typedef OtaPalStatus_t ( * OtaPalCloseFile_t )( OtaFileContext_t * const pFileContext );

/**
 * @brief Write a block of data to the specified file at the given offset.
 *
 * @note The input OtaFileContext_t pFileContext is checked for NULL by the OTA agent before this
 * function is called.
 * The file pointer/handle pFileContext->pFile, is checked for NULL by the OTA agent before this
 * function is called.
 * pData is checked for NULL by the OTA agent before this function is called.
 * blockSize is validated for range by the OTA agent before this function is called.
 * offset is validated by the OTA agent before this function is called.
 *
 * @param[in] pFileContext OTA file context information.
 * @param[in] offset Byte offset to write to from the beginning of the file.
 * @param[in] pData Pointer to the byte array of data to write.
 * @param[in] blockSize The number of bytes to write.
 *
 * @return The number of bytes written on a success, or a negative error code from the platform
 * abstraction layer.
 */
typedef int16_t ( * OtaPalWriteBlock_t ) ( OtaFileContext_t * const pFileContext,
                                           uint32_t offset,
                                           uint8_t * const pData,
                                           uint32_t blockSize );

/**
 * @brief Activate the newest MCU image received via OTA.
 *
 * This function shall do whatever is necessary to activate the newest MCU
 * firmware received via OTA. It is typically just a reset of the device.
 *
 * @note This function SHOULD not return. If it does, the platform does not support
 * an automatic reset or an error occurred.
 *
 * @return The OTA PAL layer error code combined with the MCU specific error code. See OTA Agent
 * error codes information in ota.h.
 */
typedef OtaPalStatus_t ( * OtaPalActivateNewImage_t )( OtaFileContext_t * const pFileContext );

/**
 * @brief Reset the device.
 *
 * This function shall reset the MCU and cause a reboot of the system.
 *
 * @note This function SHOULD not return. If it does, the platform does not support
 * an automatic reset or an error occurred.
 *
 * @return The OTA PAL layer error code combined with the MCU specific error code. See OTA Agent
 * error codes information in ota.h.
 */

typedef OtaPalStatus_t ( * OtaPalResetDevice_t ) ( OtaFileContext_t * const pFileContext );

/**
 * @brief Attempt to set the state of the OTA update image.
 *
 * Do whatever is required by the platform to Accept/Reject the OTA update image (or bundle).
 * Refer to the PAL implementation to determine what happens on your platform.
 *
 * @param[in] pFileContext File context of type OtaFileContext_t.
 *
 * @param[in] eState The desired state of the OTA update image.
 *
 * @return The OtaPalStatus_t error code combined with the MCU specific error code. See ota.h for
 *         OTA major error codes and your specific PAL implementation for the sub error code.
 *
 * Major error codes returned are:
 *
 *   OtaPalSuccess on success.
 *   OtaPalBadImageState: if you specify an invalid OtaImageState_t. No sub error code.
 *   OtaPalAbortFailed: failed to roll back the update image as requested by OtaImageStateAborted.
 *   OtaPalRejectFailed: failed to roll back the update image as requested by OtaImageStateRejected.
 *   OtaPalCommitFailed: failed to make the update image permanent as requested by OtaImageStateAccepted.
 */
typedef OtaPalStatus_t ( * OtaPalSetPlatformImageState_t )( OtaFileContext_t * const pFileContext,
                                                            OtaImageState_t eState );

/**
 * @brief Get the state of the OTA update image.
 *
 * We read this at OTA_Init time and when the latest OTA job reports itself in self
 * test. If the update image is in the "pending commit" state, we start a self test
 * timer to assure that we can successfully connect to the OTA services and accept
 * the OTA update image within a reasonable amount of time (user configurable). If
 * we don't satisfy that requirement, we assume there is something wrong with the
 * firmware and automatically reset the device, causing it to roll back to the
 * previously known working code.
 *
 * If the update image state is not in "pending commit," the self test timer is
 * not started.
 *
 * @param[in] pFileContext File context of type OtaFileContext_t.
 *
 * @return An OtaPalImageState_t. One of the following:
 *   OtaPalImageStatePendingCommit (the new firmware image is in the self test phase)
 *   OtaPalImageStateValid         (the new firmware image is already committed)
 *   OtaPalImageStateInvalid       (the new firmware image is invalid or non-existent)
 *
 *   NOTE: OtaPalImageStateUnknown should NEVER be returned and indicates an implementation error.
 */
typedef OtaPalImageState_t ( * OtaPalGetPlatformImageState_t ) ( OtaFileContext_t * const pFileContext );

/**
 *  OTA pal Interface structure.
 */
typedef struct OtaPalInterface
{
    /* MISRA rule 21.8 prohibits the use of abort from stdlib.h. However, this is merely one of the
     * OTA platform abstraction layer interfaces, which is used to abort an OTA update. So it's a
     * false positive. */
    /* coverity[misra_c_2012_rule_21_8_violation] */
    OtaPalAbort_t abort;                                 /*!< Abort an OTA transfer. */
    OtaPalCreateFileForRx_t createFile;                  /*!< Create a new receive file. */
    OtaPalCloseFile_t closeFile;                         /*!< Authenticate and close the receive file. */
    OtaPalWriteBlock_t writeBlock;                       /*!< Write a block of data to the specified file at the given offset. */
    OtaPalActivateNewImage_t activate;                   /*!< Activate the file received over-the-air. */
    OtaPalResetDevice_t reset;                           /*!< Reset the device. */
    OtaPalSetPlatformImageState_t setPlatformImageState; /*!< Set the state of the OTA update image. */
    OtaPalGetPlatformImageState_t getPlatformImageState; /*!< Get the state of the OTA update image. */
} OtaPalInterface_t;

#endif /* ifndef _OTA_PLATFORM_INTERFACE_ */