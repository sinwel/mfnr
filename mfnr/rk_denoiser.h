//
//////////////////////////////////////////////////////////////////////////
// File: rk_denoiser.h
// Desc: Denoiser
// 
// Date: Revised by yousf 20160822
//
//////////////////////////////////////////////////////////////////////////
// 
#pragma once
#ifndef _RK_DENOISER_H
#define _RK_DENOISER_H


//////////////////////////////////////////////////////////////////////////
////-------- Header files
// 
#include "rk_typedef.h"                 // Type definition
#include "rk_global.h"                  // Global definition



//////////////////////////////////////////////////////////////////////////
////-------- Macro Switch Setting
// 
#define     USE_MOTION_DETECT       1               // 1-use Motion Detect, 0-not use



//////////////////////////////////////////////////////////////////////////
////-------- Macro Definition
//
//---- TemporalDenoise Params Setting
#define     RAW_BLK_SIZE            32              // Raw Win Size of Block
#define     RAW_WIN_NUM	            2		    	// Num of Block: 32x32n Raw(DDR->DSP)
#define     RAW_BLK_BORDER          2               // Raw Block Border
#define     RAW_BLK_EXTEND_ROW      2               // Raw Block Extend Row of Block for DSPMalloc
#define     RAW_BLK_EXTEND_COL      4               // Raw Block Extend Col of Block for DSPMalloc 4-PixelAlign
#define     RAW_REF_EXTEND_ROW      9               // RawRef Hgt Win Border of Block for DSPMalloc
#define     RAW_REF_EXTEND_COL      12              // RawRef Wid Win Border of Block for DSPMalloc 4-PixelAlign


//#define     USE_MOTION_DETECT       1               // Motion Detect: 1-use Motion Detect, 0-not use
#if USE_MOTION_DETECT == 1
    #define MOTION_DETECT_TALBE_LEN	1024			// Motion Detect Table Length: 10bit
#endif

#define     WDR_GAIN                8.0f            // Gain x8 for WDR-Input 


//////////////////////////////////////////////////////////////////////////
//
///---- struct Rect
typedef struct tag_RK_RectExt
{
    // Rect Extend
    RK_S32          rowExtend;
    RK_S32          colExtend;
    RK_S32          hgtExtend;
    RK_S32          widExtend;
    RK_S32          strideExtend;
    // Rect Valid
    RK_S32          rowValid;
    RK_S32          colValid;
    RK_S32          hgtValid;
    RK_S32          widValid;
    // Rect Useful
    RK_S32          rowUseful;
    RK_S32          colUseful;
    RK_S32          hgtUseful;
    RK_S32          widUseful;

}RK_RectExt;

///---- struct NRMemory
typedef struct tag_RK_NRMemory
{
    RK_U16*         pRawBaseBlocksDspMemory;    // RawBaseBlocks DSP Memory
    RK_U16*         pRawBaseFilterDspMemory;    // RawBaseFilter DSP Memory
    RK_U16*         pRawBaseThreshDspMemory;    // RawBaseThresh DSP Memory
    RK_U16*         pRawRefBlocksDspMemory;     // RawRefBlocks DSP Memory
    RK_U16*         pRawRefFilterDspMemory;     // RawRefFilter DSP Memory
    RK_U16*         pRawDstSumDspMemory;        // RawDstSum DSP Memory
    RK_U16*         pRawDstWgtDspMemory;        // RawDstWgt DSP Memory
    
}RK_NRMemory;

//////////////////////////////////////////////////////////////////////////
////-------- Function Declaration

// Motion Detect Filter
int MotionDetectFilter(RK_U16* pSrc, RK_RectExt rect, RK_U16* pDst);

// Lookup MotionDetect Threshold
int LookupMotionDetectThresh(RK_U16* pRawBase, RK_RectExt rect, RK_U16 MotionDetectTable[], RK_U16* pRawThresh);

// RawDstSumInit
int RawDstInit(RK_RectExt rectBase, RK_U16* pRawBaseBlocks, RK_U16* pRawDstSum, RK_U8* pRawDstWgt);

// Temporal Denoise
int TemporalDenoise(RK_U16* pRawBaseFilter, RK_RectExt rectBase, RK_F32* pBaseBlocksPoint[], RK_U16* pRawBaseThresh, 
    int numBlocks, RK_U16* pRawRefFilter, RK_RectExt rectRef, RK_F32* pProjBlocksPoint[], 
    RK_U16* pRawRefBlocks, RK_U16 nRawWid, RK_U16 nRawHgt,
    RK_U16* pRawDstSum, RK_U8* pRawDstWgt);

// Temporal Denoise (Modify)
int TemporalDenoise_Modify(RK_U16* pRawBlocksData[], int numBlocks, RK_RectExt rects[], 
    int nRawFileNum, int nBasePicNum, RK_F32* pRawBlkPoints[], 
    RK_U16 MotionDetectTable[], RK_F32 fIspGain, RK_S16 nBlackLevel[],
    RK_U16* pRawDst);

// Normalization
int RawDstNormalize(RK_RectExt rectBase, RK_F32 ispGain, RK_U16* pRawDstSum, RK_U8* pRawDstWgt);

//////////////////////////////////////////////////////////////////////////

#endif // _RK_DENOISER_H



