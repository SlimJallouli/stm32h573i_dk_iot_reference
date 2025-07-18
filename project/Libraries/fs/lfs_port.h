/*
 * FreeRTOS STM32 Reference Integration
 *
 * Copyright (C) 2021 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
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
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

#include "lfs.h"
#include "lfs_util.h"

#ifdef LFS_NO_MALLOC
    const struct lfs_config * pxInitializeOSPIFlashFsStatic( TickType_t xBlockTime );
    const struct lfs_config * pxInitializeInternalFlashFsStatic( TickType_t xBlockTime );
#else
    const struct lfs_config * pxInitializeXSPIFlashFs( TickType_t xBlockTime );
    const struct lfs_config * pxInitializeOSPIFlashFs( TickType_t xBlockTime );
    const struct lfs_config * pxInitializeInternalFlashFs( TickType_t xBlockTime );
#endif

/* Provided outside of the lfs port */
lfs_t * pxGetDefaultFsCtx( void );
