//
//////////////////////////////////////////////////////////////////////////
// File: rk_global.h
// Desc: Global definition
// 
// Date: Revised by yousf 20160824
//
//////////////////////////////////////////////////////////////////////////
// 
#pragma once
#ifndef _RK_GLOBAL_H
#define _RK_GLOBAL_H


//////////////////////////////////////////////////////////////////////////
////-------- Header files
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "section.h"


#include "rk_typedef.h"                 // Type definition


#define 	DEBUG_DMA_SW_HW 	0 // 0-Use CEVA_CHIP_CODE   1-Use flag_UseHwDMA
// HW-DMA
//#define 	CEVA_CHIP_CODE
#define 	CEVA_CHIP_CODE_REGISTER
#define 	CEVA_CHIP_CODE_DENOISER
#define 	CEVA_CHIP_CODE_BAYERWDR


//////////////////////////////////////////////////////////////////////////
////-------- MFNR Module Debug Switch Setting
//enum BypassSwitch{ DISABLE_BYPASS = 0, ENABLE_BYPASS = 1 };
#define     DISABLE_BYPASS              0x0
#define     ENABLE_BYPASS               0x1
#define     BYPASS_Register             0   // 0-DisableBypass, 1-EnableBypass,  FeatureDetect,CoarseMatching,FineMatching
#define     BYPASS_Enhancer             0   // 0-DisableBypass, 1-EnableBypass,  TemporalDenoise,BayerWDR,SpatialDenoise
// #define     BYPASS_TemporalDenoise      0   // 0-DisableBypass, 1-EnableBypass,  TemporalDenoise
// #define     BYPASS_BayerWDR             0   // 0-DisableBypass, 1-EnableBypass,  BayerWDR
// #define     BYPASS_SpatialDenoise       0   // 0-DisableBypass, 1-EnableBypass,  SpatialDenoise

////-------- CEVA XM4 Debug Switch Setting
#define     CEVA_XM4_DEBUG              1   // 1/0 for CEVA_XM4 IDE Debug-View
////-------- Debug Params Setting
#define     MY_DEBUG_PRINTF             0   // for printf()  1 or 0

//////////////////////////////////////////////////////////////////////////
////-------- Input Params Setting
// Raw Info
#define     RK_MAX_FILE_NUM	        10              // max input images
#define     RAW_BIT_COUNT           10              // Raw Bit Count (10forDDR or 16forPC)
#define     THUMB_BIT_COUNT         16              // Thumb Bit Count (default=16)
// Align
//#define     ALIGN_4BYTE_WIDTH(Wid, BitCt)	(((int)(Wid) * (BitCt) + 31) / 32 * 4) // 4ByteAlign
#define     ALIGN_4BYTE_WIDTH(Wid, BitCt)	((((int)(Wid) * (BitCt) + 31) >> 5) << 2) // 4ByteAlign
//#define     ALIGN_4PIXEL_START(X)           ((long)(X) / 4 * 4)                     // 4PixelAlign
#define     ALIGN_4PIXEL_START(X)           ((RK_S32)(X) - ((RK_S32)(X)%4!=0)*2)  //((X)%4) ? (((X)/4)-4) : (X)
//#define     ALIGN_4PIXEL_WIDTH(W)           ((RK_U16)((double)(W) / 4 + 0.5)  * 4)    // 4PixelAlign
#define     ALIGN_4PIXEL_WIDTH(W)           ((((RK_U16)(W) + 3) >> 2) << 2)    // 4PixelAlign

#define     ALIGN_SET(var,wAlign)           (((var+wAlign-1)/wAlign)*wAlign) 


//////////////////////////////////////////////////////////////////////////
////-------- Functions Definition
//---- MAX() & MIN()
#define		MAX(a, b)			    ( (a) > (b) ? (a) : (b) )
#define		MIN(a, b)			    ( (a) < (b) ? (a) : (b) )

//---- ABS()
#define	    ABS_U16(a)			    (RK_U16)( (a) > 0 ? (a) : (-(a)) )

//---- ROUND()
#define	    ROUND_U16(a)		    (RK_U16)( (double) (a) + 0.5 )
#define	    ROUND_U32(a)		    (RK_U32)( (double) (a) + 0.5 )
#define	    ROUND_I32(a)		    (RK_S32)( (a) > 0 ? ((double) (a) + 0.5) : ((double) (a) - 0.5) )

//---- CEIL() & FLOOR()
#define	    CEIL(a)                 (int)( (double)(a) > (int)(a) ? (int)((a)+1) : (int)(a) )  
#define	    FLOOR(a)                (int)( (a) ) 

//---- FABS()
#define	    FABS(a)                 ( (a) >= 0 ? (a) : (-(a)) )


//////////////////////////////////////////////////////////////////////////

#endif // _RK_UTILS_H



