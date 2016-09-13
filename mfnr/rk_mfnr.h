//
//////////////////////////////////////////////////////////////////////////
// File: rk_mfnr.h
// Desc: MFNR: Multi-Frame Noise Reduction
// 
// Date: Revised by yousf 20160824
//
//////////////////////////////////////////////////////////////////////////
// 
#pragma once
#ifndef _RK_MFNR_H
#define _RK_MFNR_H


//////////////////////////////////////////////////////////////////////////
////-------- Header files
// 
#include "rk_typedef.h"                 // Type definition
#include "rk_global.h"                  // Global definition
#include "dma/dma.h"                     	// DMA operation
#include "rk_register.h"                // Register
#include "rk_denoiser.h"                // Denoiser
#include "rk_bayerwdr.h"                // BayerWDR


//////////////////////////////////////////////////////////////////////////
////-------- Macro Definition

#define     BASE_PIC_NUM            0               // BasePicNum = 0,...,5
#define     MARK_EXIST_AGENT        1               // mark of ExistAgent in 4x4 Region

#define     DSP_MEM_SIZE            131072//262144          // DSP memory size: 256KB = 256*1024      =    262144 Byte
#define     DDR_MEM_SIZE            268435456       // DDR memory size: 256MB = 256*1024*1024 = 268435456 Byte

#define     USE_MODIFY_ENHANCER     1               // Enhancer Select


//////////////////////////////////////////////////////////////////////////
//
////---- struct RawInfo
typedef struct tag_RK_RawInfo
{
    RK_Char         sBayerType[8];      // RGGB, ...
    RK_F32          fRedGain;           // AWB-RedGain
    RK_F32          fBlueGain;          // AWB-BlueGain
    RK_U16          nLumIntensity;      // 
    RK_F32          fSensorGain;        // Sensor Gain
    RK_F32          fIspGain;           // ISP Gain
    RK_F32          fShutter;           // 
    RK_U16          nLuxIndex;          // 
    RK_U16          nExpIndex;          // 
    RK_S16          nBlackLevel[4];     // Black Level

    RK_Char         strRawInfo[1024];   // str RawInfo
}RK_RawInfo;


////---- struct InputParams
typedef struct tag_RK_InputParams
{
    RK_U16          nRawWid;                    // Raw width
    RK_U16          nRawHgt;                    // Raw height
    RK_U32          nRawStride;                 // Raw stride
    RK_U32          nRawSize;                   // Raw size
    RK_U16          nRawFileNum;                // Raw file num
    RK_RawInfo*     pRawInfo;                   // Raw info struct
    RK_RawType*     pRawSrcs[RK_MAX_FILE_NUM];  // RawSrcs pointers
    RK_ThumbType*   pThumbSrcs[RK_MAX_FILE_NUM];// ThumbSrcs pointers
}RK_InputParams;


////---- struct ControlParams
typedef struct tag_RK_ControlParams
{
    RK_F32      useRawClip64;           // testParams[0]	 0-Not use MAX(64,refValue), 1-Use MAX(64,refValue)    
    RK_F32      usePreGainPostBlk;      // testParams[1]	 1-Use PreGain-PostBlk   2-Use PreBlk-PostGain
    RK_F32      setGain;                // testParams[2]     n-Gain x n, n=1,2,... (n>=1, float)
    RK_F32      setGainMethod;          // testParams[3]     0-LinearGain, 1-OldNonLinearGain, 2-NewNonLinearGain, 3-LnNonLinearGain_WDR
    RK_F32      useMotionDetectTable;   // testParams[4]     0-FixedMotionDetectThreshold, n-MotionDetectTable[n], n=1,2,...,8
    RK_F32      useWdrParamPol;         // testParams[5]     0-OldWdrParamPol, 1-NewWdrParamMax
    RK_F32      useRegister;            // testParams[6]     0-NotUseRegister, 1-UseRegister
    RK_F32      useIspBlack;            // testParams[7]     0-NotUseOppoBlk, 1-UseOppoBlk
    RK_F32      setNumFrameCompose;     // testParams[8]     n-nFrameCompose, n = 1,2,3,...
    RK_F32      useOverlap;             // testParams[9]     1-NonOverlap, 2-OverlapStep1/2, 4-OverlapStep1/4, ...
    RK_F32      useSpatialDenoise;      // testParams[10]    0-NotUseSpatialDenoise, 1-NotUseSpatialDenoise  
    RK_Char     strCtrlParam[1024];     // str ControlParams

    RK_Char		useHwDMA;
}RK_ControlParams;


//////////////////////////////////////////////////////////////////////////
////-------- Class Definition
// class MFNR
class classMFNR
{
public:
    //// constructor & destructor
    //classMFNR(void){};              // constructor
    //~classMFNR(void){};             // destructor

public:
    //////////////////////////////////////////////////////////////////////////
    //// RawSrcs
    int			    mRawWid;					        // RawSrcs data width
    int			    mRawHgt;					        // RawSrcs data height
    int			    mRawFileNum;				        // RawSrcs data file number
    int             mRawStride;                         // RawSrcs data Stride (Bytes, 10bit 4ByteAlign)
    int             mRawDataSize;                       // RawSrcs data Size (Bytes, 10bit)
    RK_U16*         pRawSrcs[RK_MAX_FILE_NUM];          // RawSrcs data pointers

    //// Scaler
    int             mScaleRaw2Raw;                      // scale factor for Raw to Raw (for Preview)
    int             mScaleRaw2Luma;                     // scale factor for Raw to Luma
    int             mScaleRaw2Thumb;                    // scale factor for Raw to Thumbnail

    //// ThumbSrcs
    int			    mThumbWid;					        // ThumbSrcs data width
    int			    mThumbHgt;					        // ThumbSrcs data height
    int             mThumbStride;                       // ThumbSrcs data Stride (Bytes, 16bit 4ByteAlign)
    int             mThumbDataSize;                     // ThumbSrcs data Size (Bytes, 16bit)
    RK_U16*         pThumbSrcs[RK_MAX_FILE_NUM];        // ThumbSrcs data pointers

    
    // RawInfo
    RK_F32          mIspGain;                           // ISP Gain
    RK_S16			mBlackLevel[4];                     // Black Level



    RK_Char			mUseHwDMA; 							// HW-DMA Test
    //////////////////////////////////////////////////////////////////////////
 
    // Method-1: use malloc&free
    //DspMemManager   dspMemManager;                      // Method-1: use MemManager

    // Method-2: use Array
    RK_U8*          dspMemoryArray;       				// Method-2: use MemoryArray
    RK_U32          mDspMem_UsedCount;                  // DSP Memory Array Used Count
    RK_U32          mDspMem_ResetPos;                   // DSP Memory Reset Position

    rdma_info_t     rdmaInfo;                           // DMA Info Struct

    //////////////////////////////////////////////////////////////////////////
    //// Register: BaseFrame Feature Detect
    int             mThumbFeatWinSize;                  // Thumb Feature Win Size
    int             mThumbDivSegCol;                    // number of Seg Col
    int             mThumbDivSegRow;                    // number of Seg Row
    int             mBasePicNum;                        // Base Picture Num
    int             mMaxNumFeature;                     // Max Num of Feature
    RK_U16*         pFeaturePoints[2];                  // Feature Points: [1xNx2] * 2Byte
    RK_U16*         pFeatureValues;                     // Feature Values: [1xN] * 2Byte
    RK_U16*         pThumbDspChunks[2];                 // Thumb DSP Chunk
    RK_U16*         pThumbFilterDspChunk;               // Thumb Filter DSP Chunk
    int             mNumValidFeature;                   // num of Valid Feature

    //// Block Coarse Matching
    RK_U16*         pMatchPointsY[RK_MAX_FILE_NUM];     // Matching Points Y
    RK_U16*         pMatchPointsX[RK_MAX_FILE_NUM];     // Matching Points X
    RK_U16*         pThumbBaseBlkDspChunks[2];          // ThumbBaseBlk DSP Chunk
    RK_U16*         pThumbRefBlkDspChunks[2];           // ThumbRefBlk DSP Chunk

    //// Block Fine Matching
    RK_U8*          pFeatureIdxsInAgent;                // FeatureIdxs In Agent
    RK_U16*         pAgentPointsWeight[RK_MAX_FILE_NUM];// Agent Points Weight
    RK_U16*         pRawBaseBlkDspChunks[2];            // RawBaseBlk DSP Chunk
    RK_U16*         pLumaBaseBlkDspChunks[2];           // LumaBaseBlk DSP Chunk
    RK_U16*         pRawRefBlkDspChunks[2];             // RawRefBlk DSP Chunk
    RK_U16*         pLumaRefBlkDspChunks[2];            // LumaRefBlk DSP Chunk

    //// Compute Homography
    RK_F32*         pHomographyMatrix[RK_MAX_FILE_NUM]; // Homography: [9*RawFileNum] * 4Byte
    RK_U8*          pRowMvHist;                         // RowMV Hist
    RK_U8*          pColMvHist;                         // ColMV Hist
    RK_U8*          pMarkMatchFeature;                  // Marks of Match Feature in BaseFrame & RefFrame#k
    RK_U8*          pAgentsIn4x4Region_Marks;           // Agents in 4x4 Region [RegMark4x4] * 16
    RK_U16*         pAgentsIn4x4Region_Wgts;            // Agents in 4x4 Region [Sharp/SAD] * 16
    RK_U16*         pAgentsIn4x4Region_PtYs[RK_MAX_FILE_NUM];   // Agents in 4x4 Region [Y] * RawFileNum * 16
    RK_U16*         pAgentsIn4x4Region_PtXs[RK_MAX_FILE_NUM];   // Agents in 4x4 Region [X] * RawFileNum * 16
    RK_U16*         pRegion4Points;                     // 4 points in 4 Regions: [x0,y0,x1,y1] * 4Points * 2Byte
    RK_F32*         pMatrixA;                           // Coefficient Matrix A for A*X = B: 8*8*4Byte
    RK_F32*         pVectorB;                           // Coefficient Vector B for A*X = B: 8*1*4Byte
    RK_F32*         pVectorX;                           // Coefficient Vector X for A*X = B: 3*3*4Byte
    RK_F32*         pBasePoint;                         // Base Point: 2*4Byte
    RK_F32*         pRefPoint;                          // Ref  Point: 2*4Byte
    RK_F32*         pProjPoint;                         // Perspective Project Point: 2*4Byte

    //// Temporal Denoise
//#if USE_MODIFY_ENHANCER == 0
    RK_F32*         pBaseBlocksPoint[RAW_WIN_NUM];      // BaseBlocks Top-Left-Corners Pointer: n Points for n-block(32x32)
    RK_F32*         pProjBlocksPoint[RAW_WIN_NUM];      // ProjBlocks Top-Left-Corners Pointer: n Points for n-block(32x32)
    RK_U16*         pRawBaseBlocksDspChunks[2];         // RawBaseBlocks DSP Chunks: (32+2*2)x(32n+2*4) * 2B * 2  ExpandedBoundary=(2,4)
    RK_U16*         pRawBaseFilterDspChunk;             // RawBaseFilter DSP Chunk: (32+2*2)x(32n+2*4) * 2B * 2  ExpandedBoundary=(2,2)
    RK_U16*         pRawBaseThreshDspChunk;             // RawBaseThresh DSP Chunk: 32x32n * 1B
    RK_U16*         pRawRefBlocksDspChunks[2];          // RawRefBlocks DSP Chunks: (32+2*9)x(32n+2*12) * 2B * 2  ExpandedBoundary=(9,12)
    RK_U16*         pRawRefFilterDspChunk;              // RawRefFilter DSP Chunk:  (32+2*9)x(32n+2*12) * 2B      ExpandedBoundary=(9,12)
    RK_U16*         pRawDstSumDspChunk;                 // RawDstSum DSP Chunk: 32x32n * 2B
    RK_U8*          pRawDstWgtDspChunk;                 // RawDstWgt DSP Chunk: 32x32n * 1B
//#else
    RK_F32*         pRawBlkPoints[RK_MAX_FILE_NUM];     // RawSrcBlocks Top-Left-Corners Pointer
    RK_U16*         pRawBlkChunks[2][RK_MAX_FILE_NUM];  // RawSrcBlocks DSP Chunks
    RK_U16*         pRawDstChunk;                       // RawDstBlocks DSP Chunk
//#endif

    //// Bayer WDR
    RK_U16*         pWdrRawBlockBuf[2];                 // BlkBuf: (2+32)x(1+32n+1)*2B, n=2 -> 32*2=64
    RK_U16*         pWdrRawRowBuf;                      // RowBuf: 2xRawWid*2B
    RK_U16*         pWdrRawColBuf;                      // ColBuf: 32x1*2B
    RK_U16*         pWdrRawBlockRect[2];                // Rects: 1x4*2B
    RK_U16*         pWdrThumbWgtTable;	                // Thumb Weight Table: 9x256*2B
    RK_U32*         pWdrWeightMat;	                    // Weight Mat: 9x256*4B
#if 1
    RK_U16*         pWdrThumbWgtTable1;	                // Thumb Weight Table: 9x256*2B
    RK_U32*         pWdrWeightMat1;	                    // Weight Mat: 9x256*4B
#endif
    RK_U16*         pWdrScaleTable;                     // ScaleTabale[expouse_times] 961*2B
    RK_U16*         pWdrLeftRight;                      // 2*32x16*2B byte space, 2K store 32 line left and right, align 16, actually 9 valid..
    RK_U16*         pWdrGainMat;                        // Result: 32x32n*2B
    RK_U16*         pWdrRawResult;                      // Result: 32x32n*2B

    //// SpatialDenoise



public:
    
    ////---- RK DMA
    // transfer_mode = 0 // RDMA_DIRECTION
    int RKDMA_ReadThumb16bit2DSP(U32 srcAddr, U32 dstAddr, U16 wid, U16 hgt, U16 srcStride, U16 dstStride, U16 col);

    // transfer_mode = 1 // RDMA_10BIT_2_16BIT
    int RKDMA_ReadRaw10bit2DSP(U32 srcAddr, U32 dstAddr, U16 wid, U16 hgt, U16 srcStride, U16 dstStride, U16 col);

    // transfer_mode = 2 // RDMA_16BIT_2_10BIT
    int RKDMA_WriteRaw16bit2DDR(U32 srcAddr, U32 dstAddr, U16 wid, U16 hgt, U16 srcStride, U16 dstStride, U16 col);


    ////---- Process Module-1: Register Interface (FeatureDetect & FeatureFilter & CoarseMatching & FineMatching & ComputeHomography)
    int Register(void);
    int Register_BypassWrite(RK_F32* pHomoMats[], int nRawFileNum);
    int Register_BypassRead(int nRawFileNum, RK_F32* pHomoMats[]);


    ////---- Process Module-2: Enhancer Interface (TemporalDenoise & BayerWDR & SpatialDenoise)
    int Enhancer(RK_RawType* pRawDst);
    int Enhancer_Modify(RK_RawType* pRawDst);


    ////---- MFNR Interface Functions
    int MFNR_Init(RK_InputParams* pInParams, RK_ControlParams* pCtrlParams);    // MFNR Init
    int MFNR_Process(RK_RawType* pRawDst);                                      // MFNR Execute
    int MFNR_UnInit();			                                                // MFNR UnInit			                                    

};


//// DSP Memory: 128KB
DATA_MFNR_INT_DSP  extern	RK_U8 g_DspBuf[DSP_MEM_SIZE];

// classMFNR
DATA_MFNR_EX 	extern	classMFNR		g_mfnrProcessor;

// MFNR Interface
int RK_MFNR_Processor(RK_InputParams* pInParams, RK_ControlParams* pCtrlParams, RK_RawType* pRawDst);

//////////////////////////////////////////////////////////////////////////

#endif // _RK_MFNR_H



