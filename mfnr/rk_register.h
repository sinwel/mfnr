//
//////////////////////////////////////////////////////////////////////////
// File: rk_register.h
// Desc: Register
// 
// Date: Revised by yousf 20160824
//
//////////////////////////////////////////////////////////////////////////
// 
#pragma once
#ifndef _RK_REGISTER_H
#define _RK_REGISTER_H


//////////////////////////////////////////////////////////////////////////
////-------- Header files
// 
#include "rk_typedef.h"                 // Type definition
#include "rk_global.h"                  // Global definition

#include <vec-c.h>
#include "XM4_defines.h"

//////////////////////////////////////////////////////////////////////////
////-------- Macro Switch Setting
// 
// Compute Grad Switch Setting
#define     USE_MAX_GRAD            0               // 1-use max grad truncation, 0-not use
#define     FEATURE_TH_METHOD       0//1               // 0-USE_MIN2_TH, 1-USE_AVE_TH
// Homography Computation Switch Setting
#define     USE_MV_HIST_FILTRATE    1               // 1-use MV Hist Filtrate, 0-not use
#define     USE_EARLY_STOP_H		0               // 1-use Early Stop Compute Homography, 0-not use
#define     USE_FLOAT_ERROR         1               // 0-RK_U16, 1-RK_F32
//#define     REGISTER_DEBUG          
//#define     CEVA_CHIP_CODE

//////////////////////////////////////////////////////////////////////////
////-------- Macro Definition
//
//---- Scaler Params Setting
#define     SCALER_FACTOR_R2R       2               // Raw to Raw
#define     SCALER_FACTOR_R2L       2               // Raw to Luma
#define     SCALER_FACTOR_R2T       8               // Raw to Thumbnail

//---- Compute Grad Params Setting
//#define     USE_MAX_GRAD            0               // 1-use max grad truncation, 0-not use
#if USE_MAX_GRAD == 1
    #define MAX_GRAD                0xFFF           // max grad 0xFFF= 2^12-1
#endif
#define     USE_MIN2_TH             0               // Method-0: average
#define     USE_AVE_TH              1               // Method-1: first min + second min
//#define     FEATURE_TH_METHOD       USE_AVE_TH      // Feature Threshold Method

//---- Coarse Matching Params Setting
#define     DIV_FIXED_WIN_SIZE      32              // Win size
#define     NUM_LINE_DDR2DSP_THUMB  32              // num of line: read thumb to DSP
#define     MAX_SHARP_RATIO         0.0//0.96            // sharp < 0.96*MaxSharp -> InvalidRef
#define     MARK_BASE_FRAME         0               // base frame mark: 0
#define     MARK_VALID_REF          1               // valid ref frame mark: 1
#define     MARK_INVALID_REF        2               // invalid ref frame mark: 2
#define     COARSE_MATCH_WIN_SIZE   16              // Coarse Matching Win size in Thumb
#define     MAX_OFFSET              64//100             // max offset of each 2 frames
#define     COARSE_MATCH_RADIUS    (CEIL(MAX_OFFSET * 1.0 / SCALER_FACTOR_R2T)) // Coarse Matching Radius

//---- Divide Image
#define     NUM_DIVIDE_IMAGE        4               // Divide Image into 4x4 Region

//---- Fine Matching Params Setting
#define     FINE_MATCH_WIN_SIZE     64              // Fine Matching Win size in Raw
#define     FINE_LUMA_RADIUS        5               // search radius in Luma: 8=SCALER_FACTOR_RAW2THUMB, 2=SCALER_FACTOR_RAW2LUMA, Radius=(8/2+1)

//---- Homography Computation Params Setting
//#define     USE_MV_HIST_FILTRATE    1               // 1-use MV Hist Filtrate, 0-not use
#define		MAX_NUM_MATCH_FEATURE   512             // Max Num of Match Feature <-- 32x16 Segments at most in Thumb (Raw:8192x4096)    
#if USE_MV_HIST_FILTRATE == 1
    #define HALF_LEN_MV_HIST        (COARSE_MATCH_RADIUS * SCALER_FACTOR_R2T / SCALER_FACTOR_R2L + FINE_LUMA_RADIUS) // Half Length of MV Hist: 13*8/2+5=57
    #define LEN_MV_HIST             (HALF_LEN_MV_HIST*2+1) // Length of MV Hist: 57*2+1=115
    #define VALID_FEATURE_RATIO     0.1//0.01            // Valid Feature Ratio
#endif
#define     NUM_R4IT_CHOICE         1810            // nchoosek(16,4) - 10(in line)
#define     MARK_EXIST_AGENT        1               // mark of ExistAgent in 4x4 Region
#define     NUM_HOMOGRAPHY          100             // Number of Homography Computed
#define     ERR_TH_VALID_H          5               // Error Threshold of Valid Homography
//#define     USE_EARLY_STOP_H		1               // 1-use Early Stop Compute Homography, 0-not use
#if USE_EARLY_STOP_H == 1
    #define	ERR_TH_GOOD_H           0               // Error Threshold of Good Homography
    #define GOOD_CNT_H_RATIO		0.8				// Good Count Homography Ratio
#endif
#define     CRRCNT_TH_VALID_H       4               // Correct Count Threshold of Valid Homography
#define     MAX_PROJECT_ERROR       0x3F            // Max Project error: (64=6bit) + (16x16Division=8bit) + (RowCol=1bit) < 16bit





//////////////////////////////////////////////////////////////////////////
////-------- Function Declaration

// Feature Detect
int FeatureDetect(RK_U16* pThumbData, int nWid, int nHgt, int nStride, int rowSeg, int numFeature, RK_U16* pFeatPoints[], RK_U16* pFeatValues);

//GetWdrWeightTable
void GetWdrWeightTable( RK_U16* pThumbData,      // <<! [in]
                        int 	nWid,           // <<! [in]    
                        int 	nHgt,           // <<! [in]
                        int 	nStride,        // <<! [in]
                        int 	rowSeg,         // <<! [in]     
                        RK_U16* pThumbFilterDspChunk, // <<! [in]
                        RK_U32* pWdrWeightMat,        // <<! [in]  
                        RK_U16* pWdrThumbWgtTable,
                    #if 1//WDR_THUMB_TABLE_TR
                    	RK_U32* pWdrWeightMat1,        // <<! [in]  
                    	RK_U16* pWdrThumbWgtTable1,
                    #endif
                        int 	statisticWidth,
                        int 	statisticHeight);   // <<! [out]  

// Feature Filter
int FeatureFilter(RK_U16* pFeatPoints[], RK_U16* pFeatValues, int numFeature, int nThumbWid, int nThumbHgt, 
    int& numValidFeature);

// Feature Coarse Matching
int FeatureCoarseMatching(RK_U16* pThumbBase, RK_U16 hgt0, RK_U16 wid0, RK_U16* pThumbRef, RK_U16 hgt1, RK_U16 wid1, 
    RK_U16& row, RK_U16& col, RK_U16& cost);

// Scaler Raw to Luma
int Scaler_Raw2Luma(RK_U16* pRawData, int nRawWid, int nRawHgt, int nLumaWid, int nLumaHgt, 
    RK_U16* pLumaData);

// Feature Fine Matching
int FeatureFineMatching(RK_U16* pLumaBase, RK_U16 hgt0, RK_U16 wid0, 
    RK_U16* pLumaRef, RK_U16 hgt1, RK_U16 wid1, 
    RK_U16 col_st, RK_U16 wid_ref, 
    RK_U16& row, RK_U16& col, RK_U16& cost);


// MV Hist Filter
int MvHistFilter(RK_U16* pMatchPtsY[], RK_U16* pMatchPtsX[], int numValidFeature,
    RK_U8* pRowMvHist, RK_U8* pColMvHist, int nBasePicNum, int nRefPicNum,
    RK_U8* pMarkMatchFeature);

// Get 4 points in 4 Regions
int GetRegion4Points(RK_U8* pAgents_Marks, RK_U16* pAgents_PtYs[], RK_U16* pAgents_PtXs[],
    RK_U8* pTable, int idx, int numBase, int numRef, RK_U16* pPoints4);

// Create Coefficient MatrixA & VectorB
int CreateCoefficient(RK_U16* pPoints4, RK_F32* pMatA, RK_F32* pVecB);

//  Compute a PersPective Matrix
int ComputePerspectMatrix(RK_F32* pMatA, RK_F32* pVecB, RK_F32* pVecX);

// Perspective Project: pVecX * pBasePoint = pProjPoint
int PerspectProject(RK_F32* pVecX, RK_F32* pBasePoint, RK_F32* pProjPoint);

// Compute Homography's Error
int ComputeHomographyError(int type, RK_U8* pMarks,int numFeature, RK_U16* pPointYs[], RK_U16* pPointXs[], int nBasePicNum, int nRefPicNum, 
    RK_F32* pVectorX, RK_F32* pBasePoint, RK_F32* pProjPoint, RK_F32* pRefPoint,
    RK_U32& error);



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FeatureDetect_Vec( RK_U16 *p_s16Src,  // <<! [ in ] block data: 34 * 522
						RK_U16 &maxVal,    // <<! [ out ] max gradient value in 32 * 32
						RK_U16 &maxGrdRow, // <<! [ out ] max gradient row
						RK_U16 &maxGrdCol, // <<! [ out ] max gradient col
						RK_U32 srcStride,  // <<! [ in ] src stride: 522
						RK_U32 offsetX,    // <<! [ in ] offset in each 32 * 32  boloc: 522
						RK_U32 u32Rows,    // <<! [ in ] feature detect row s: 32
						RK_U32 u32Cols);   // <<! [ in ] feature detect col: 32;

void FeatureCoarseMatching_Vec_vswsad(  RK_U16 *p_s16Src1,  // <<! [ in ]: base block: 16 * 16
										RK_U16 *p_s16Src2,  // <<! [ in ]: ref block: ( 16 + maxMV )*( 16 + maxMV )
										RK_S32 stride1,     // <<! [ in ]: base stride: 16
										RK_S32 stride2,     // <<! [ in ]: ref stride
										RK_U32 u32Rows1,    // <<! [ in ]: base block row: 16
										RK_U32 u32Cols1,    // <<! [ in ]: base block col: 16
										RK_U32 u32Rows2,    // <<! [ in ]: base block row: 16 + maxMV
										RK_U32 u32Cols2,    // <<! [ in ]: base block col: 16 + maxMV
										RK_U16 &minSAD,     // <<! [ out ]: minSad
										RK_U16 &minRow,     // <<! [ out ]: minSad row
										RK_U16 &minCol );   // <<! [ out ]: minSad col

void FeatureFineMatching_Vec_vswsad( RK_U16 *p_s16Src1, // <<! [ in ]: base block: 32 * 32
									 RK_U16 *p_s16Src2, // <<! [ in ]: ref block: ( 32 + maxMV )*( 32 + maxMV )
									 RK_S32 stride1,    // <<! [ in ]: base stride: 32
									 RK_S32 stride2,    // <<! [ in ]: ref stride
									 RK_U32 offsetX,    // <<! [ in ]: ref block Col Start
									 RK_U32 u32Rows1,   // <<! [ in ]: base block row: 32
									 RK_U32 u32Cols1,   // <<! [ in ]: base block col: 32
									 RK_U32 u32Rows2,   // <<! [ in ]: base block row: 32 + maxMV
									 RK_U32 u32Cols2,   // <<! [ in ]: base block col: 32 + maxMV
									 RK_U16 &minSAD,    // <<! [ out ]: minSad
									 RK_U16 &minRow,    // <<! [ out ]: minSad row
									 RK_U16 &minCol );  // <<! [ out ]: minSad col

void Scaler_Raw2Luma_Vec( RK_U16 *p_s16Src,   // <<! [ in ]: src raw data
						  RK_U16 *p_s16Dst,   // <<! [ out ]: luma data
						  RK_U32 srcStride,   // <<! [ in ]: src raw stride
						  RK_U32 dstStride,   // <<! [ in ]: luma stride
						  RK_U32 u32Rows,     // <<! [ in ]:raw data row
						  RK_U32 u32Cols );   // <<! [ in ]:raw data col 

RK_S32 ComputePerspectMatrix_Vec( RK_F32 *A,      // <<! [ in ] A
							      RK_F32 *b,      // <<! [ in ] b
							      RK_F32 *OutX ); // <<! [ in ] homography

void ComputeHomographyError_Vec_float( RK_S32 type,             // <<! [ in ]: type: 0: correct num, 1: sum error
									   RK_U8 *pMasks,			// <<! [ in ]: valid match points mask   
									   RK_F32 *pH,              // <<! [ in ]: homography
									   RK_U16 *pBasePointX,     // <<! [ in ]: base pointsX 
									   RK_U16 *pBasePointY,     // <<! [ in ]: base pointsY 
									   RK_U16 *pSrcPointX,      // <<! [ in ]: match src pointsX 
									   RK_U16 *pSrcPointY,      // <<! [ in ]: match src pointsY 
									   RK_U32 u32Cols,          // <<! [ in ]: points num
									   RK_U32 &error );         // <<! [ out ]: correct num or sum error

void ComputeHomographyError_Vec_uint( RK_S32 type,              // <<! [ in ]: type: 0: correct num, 1: sum error
									  RK_U8 *pMasks,            // <<! [ in ]: valid match points mask
									  RK_F32 *pH,               // <<! [ in ]: homography
									  RK_U16 *pBasePointX,      // <<! [ in ]: base pointsX
									  RK_U16 *pBasePointY,      // <<! [ in ]: base pointsY
									  RK_U16 *pSrcPointX,       // <<! [ in ]: match src pointsX   
									  RK_U16 *pSrcPointY,       // <<! [ in ]: match src pointsY 
									  RK_U32 u32Cols,           // <<! [ in ]: points num
									  RK_U32 &error );          // <<! [ out ]: correct num or sum error

#endif // _RK_REGISTER_H




