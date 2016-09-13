//
/////////////////////////////////////////////////////////////////////////
// File: rk_mfnr.c
// Desc: Implementation of Register
// 
// Date: Revised by yousf 20160824
// 
//////////////////////////////////////////////////////////////////////////
////-------- Header files
//
#include "rk_register.h"     // Register
#include "rk_bayerwdr.h"     // BayerWDR

//////////////////////////////////////////////////////////////////////////
////-------- Functions Definition
//
/************************************************************************/
// Func: FeatureDetect()
// Desc: Feature Detect
//   In: pThumbDspChunk         - [in] Thumb data pointer
//       nWid                   - [in] Thumb data width
//       nHgt                   - [in] Thumb data height
//       nStride                - [in] Thumb data stride
//       rowSeg                 - [in] Seg(rowSeg,m)
//       numFeature             - [in] num Feature in input Thumb data 
//       pThumbFilterDspChunk   - [in] Thumb Filter data pointer: 9x256*2B
//       pWdrWeightMat          - [in] Weight Mat data pointer: 9x256*4B
//  Out: pFeatPoints            - [out] Feature Points: [1xNx2] * 2Byte
//       pFeatValues            - [out] Feature Values: [1xN] * 2Byte
//       pWdrThumbWgtTable      - [out] Thumb Weight Table: 9x256*2B
// 
// Date: Revised by yousf 20160824
// 
/*************************************************************************/
CODE_MFNR_EX
int FeatureDetect(RK_U16* pThumbDspChunk, int nWid, int nHgt, int nStride, int rowSeg, int numFeature, RK_U16* pFeatPoints[], RK_U16* pFeatValues)
{
#ifndef CEVA_CHIP_CODE_REGISTER
    
    //
    int     ret = 0; // return value
// #if MY_DEBUG_PRINTF == 1
//     printf("FeatureDetect()\n");
// #endif    
    //
    int Grdx, Grdy, Grad;
    int maxGrad, maxGrdRow, maxGrdCol;

    int wid = nStride/2;

    // 
    RK_U16* pTmp = NULL;
    int     nThumbFeatWinSize = DIV_FIXED_WIN_SIZE;
    for (int m=0; m < numFeature; m++)
    {
        // init
        maxGrad   = 0; // Max
        maxGrdRow = 0; // row
        maxGrdCol = 0; // col

        // 
        pTmp = pThumbDspChunk + 2*wid + 1 + m * nThumbFeatWinSize + 1; // top-1,left-1, block(1,1)
        for (int i=1; i<nThumbFeatWinSize-1; i++)
        {
            for (int j=1; j <nThumbFeatWinSize-1; j++)
            {
                // grad
//                 Grdx = *(pThumbDspChunk +   i   * wid + j-1 + m * nThumbFeatWinSize) 
//                      - *(pThumbDspChunk +   i   * wid + j+1 + m * nThumbFeatWinSize);
//                 Grdy = *(pThumbDspChunk + (i-1) * wid + j   + m * nThumbFeatWinSize) 
//                      - *(pThumbDspChunk + (i+1) * wid + j   + m * nThumbFeatWinSize);
                Grdx = *(pTmp-1) - *(pTmp+1);
                Grdy = *(pTmp-nWid) - *(pTmp+nWid);

                // next col
                pTmp++; 

                // sum grad
                Grdx = ABS_U16(Grdx);
                Grdy = ABS_U16(Grdy);
                Grad = ( Grdx + Grdy ) >> 1; // overflow ?

#if USE_MAX_GRAD == 1
                Grad = MIN(Grad, MAX_GRAD); // truncate 12bit: // max grad 0xFFF= 2^12-1
#endif

                // maxGrad
                if (Grad > maxGrad)
                {
                    maxGrad   = Grad;
                    maxGrdRow = rowSeg * nThumbFeatWinSize + i;
                    maxGrdCol = m * nThumbFeatWinSize + j;
                }

            } // for j

            // next row
            pTmp += (nWid - nThumbFeatWinSize + 2); 

        } // for i

        // Feature
        pFeatPoints[0][rowSeg * numFeature + m] = maxGrdRow;
        pFeatPoints[1][rowSeg * numFeature + m] = maxGrdCol;
        pFeatValues[rowSeg * numFeature + m]    = maxGrad;
    }

    //
    return ret;

#else
	int     ret = 0; // return value

	ushort maxGrad, maxGrdRow, maxGrdCol;

	for ( int m = 0; m < numFeature; m++ )
	{
		uint offsetX = nWid + 1 + m * DIV_FIXED_WIN_SIZE;
		FeatureDetect_Vec(pThumbDspChunk, maxGrad, maxGrdRow, maxGrdCol, nWid, offsetX, DIV_FIXED_WIN_SIZE, DIV_FIXED_WIN_SIZE);

		// Feature
		pFeatPoints[0][rowSeg * numFeature + m] = rowSeg * DIV_FIXED_WIN_SIZE + maxGrdRow;
		pFeatPoints[1][rowSeg * numFeature + m] = m * DIV_FIXED_WIN_SIZE + maxGrdCol;
		pFeatValues[rowSeg * numFeature + m]    = maxGrad;

	}

	return ret;
#endif
} // FeatureDetect()


//////////////////////////////////////////////////////////////////////////
CODE_MFNR_EX
void GetWdrWeightTable(RK_U16* pThumbData, // <<! [in]
	int 	nWid,           // <<! [in]    
	int 	nHgt,           // <<! [in]
	int 	nStride,        // <<! [in]
	int 	rowSeg,         // <<! [in]     
	RK_U16* pThumbFilterDspChunk, // <<! [in]
	RK_U32* pWdrWeightMat,        // <<! [in]  
	RK_U16* pWdrThumbWgtTable,
#if 1
	RK_U32* pWdrWeightMat1,        // <<! [in]  
	RK_U16* pWdrThumbWgtTable1,
#endif
	int 	statisticWidth,
	int 	statisticHeight)   // <<! [out]  
{	
	// add by zxy @ 2016 08 25 
#if 1//ndef CEVA_CHIP_CODE_REGISTER

	wdrPreFilterBlock( pThumbData, pThumbFilterDspChunk, nHgt, nWid );
	//hist
	CalcuHist( pThumbFilterDspChunk, pWdrThumbWgtTable, pWdrWeightMat, nHgt, nWid, statisticWidth, rowSeg );
	CalcuHistTranspose( pThumbFilterDspChunk, pWdrThumbWgtTable1, pWdrWeightMat1, nHgt, nWid, statisticWidth, rowSeg );
#else

	//filter
	wdrPreFilterBlock_Vec( pThumbData, pThumbFilterDspChunk, nHgt - 2, nWid - 2, nWid );
	//hist
	CalcuHist_Vec( pThumbFilterDspChunk, pThumbData, pWdrThumbWgtTable, pWdrWeightMat, nHgt - 2, nWid - 2, nWid, statisticWidth, rowSeg );

#endif

}


/************************************************************************/
// Func: FeatureFilter()
// Desc: Feature Filter
//   In: pFeatPoints        - [in/out] Feature Points: [1xNx2] * 2Byte
//       pFeatValues        - [in/out] Feature Values: [1xN] * 2Byte
//       numFeature         - [in] num Feature in input Thumb data 
//       nThumbWid          - [in] Thumb data width
//       nThumbHgt          - [in] Thumb data height
//  Out: numValidFeature    - [out] num of Valid Feature
// 
// Date: Revised by yousf 2016804
// 
/*************************************************************************/
CODE_MFNR_EX
int FeatureFilter(RK_U16* pFeatPoints[], RK_U16* pFeatValues, int numFeature, int nThumbWid, int nThumbHgt, int& numValidFeature)
{
#ifndef CEVA_CHIP_CODE_REGISTER
    //
    int     ret = 0; // return value
// #if MY_DEBUG_PRINTF == 1
//     printf("FeatureFilter()\n");
// #endif      
#if FEATURE_TH_METHOD == USE_MIN2_TH
    // sharpness threshold
    RK_U16      SharpTh; 
    // first min & second min
    RK_U16      FirstMinSegSharp    = 0xFFFF;   // init First Min Segment Sharpness
    RK_U16      SecondMinSegSharp   = 0xFFFF;   // init Second Min Segment Sharpness
    RK_U16      fstMinRow           = 0;        // row
    RK_U16      fstMinCol           = 0;        // col
    RK_U16      sndMinRow           = 0;        // row
    RK_U16      sndMinCol           = 0;        // col
    for (int n=0; n < numFeature; n++)
    {
        if (FirstMinSegSharp >= pFeatValues[n])
        {
            // second min
            sndMinRow         = fstMinRow;
            sndMinCol         = fstMinCol;
            SecondMinSegSharp = FirstMinSegSharp;
            // first min
            fstMinRow         = pFeatPoints[0][n];  // maxGrdRow
            fstMinCol         = pFeatPoints[1][n];  // maxGrdCol
            FirstMinSegSharp  = pFeatValues[n];     // maxGrad
        }
        else if(SecondMinSegSharp >= pFeatValues[n])
        {
            // second min 
            sndMinRow         = pFeatPoints[0][n];  // maxGrdRow
            sndMinCol         = pFeatPoints[1][n];  // maxGrdCol
            SecondMinSegSharp = pFeatValues[n];     // maxGrad 
        }
    } // for n
    // first min + second min
    SharpTh = SecondMinSegSharp + FirstMinSegSharp; // sharpness threshold

#elif FEATURE_TH_METHOD == USE_AVE_TH
    // sharpness threshold
    RK_U16      SharpTh; 
    int         sumGrd = 0;
    for (int n=0; n < numFeature; n++)
    {
        sumGrd += pFeatValues[n];
    }
    // ave
    SharpTh = sumGrd / numFeature;

#endif
	//SharpTh = 0; // 0823 test

    // Valid Features
    numValidFeature = 0;            // num of Valid Feature
    for (int n=0; n < numFeature; n++)
    {
        if (pFeatValues[n] >= SharpTh) // Segment
        {
            if (   pFeatPoints[0][n] < COARSE_MATCH_WIN_SIZE
                || pFeatPoints[1][n] < COARSE_MATCH_WIN_SIZE 
                || pFeatPoints[0][n] >= nThumbHgt - COARSE_MATCH_WIN_SIZE 
                || pFeatPoints[1][n] >= nThumbWid - COARSE_MATCH_WIN_SIZE)
            { 
                // Invalid Feature
            }
            else
            {
                // Valid Feature
                pFeatPoints[0][numValidFeature] = pFeatPoints[0][n];    // maxGrdRow
                pFeatPoints[1][numValidFeature] = pFeatPoints[1][n];    // maxGrdCol
                pFeatValues[numValidFeature]    = pFeatValues[n];       // maxGrad 
                numValidFeature++;
            }
        }        
    }
    // Invalid Feature
    pFeatPoints[0][numValidFeature] = 0;  // maxGrdRow
    pFeatPoints[1][numValidFeature] = 0;  // maxGrdCol
    pFeatValues[numValidFeature]    = 0;  // maxGrad 

    //
    return ret;

#else
	//
	int     ret = 0; // return value
	// #if MY_DEBUG_PRINTF == 1
	//     printf("FeatureFilter()\n");
	// #endif      
#if FEATURE_TH_METHOD == USE_MIN2_TH
	// sharpness threshold
	RK_U16      SharpTh; 
	// first min & second min
	RK_U16      FirstMinSegSharp    = 0xFFFF;   // init First Min Segment Sharpness
	RK_U16      SecondMinSegSharp   = 0xFFFF;   // init Second Min Segment Sharpness
	RK_U16      fstMinRow           = 0;        // row
	RK_U16      fstMinCol           = 0;        // col
	RK_U16      sndMinRow           = 0;        // row
	RK_U16      sndMinCol           = 0;        // col
	for (int n=0; n < numFeature; n++)
	{
		if (FirstMinSegSharp >= pFeatValues[n])
		{
			// second min
			sndMinRow         = fstMinRow;
			sndMinCol         = fstMinCol;
			SecondMinSegSharp = FirstMinSegSharp;
			// first min
			fstMinRow         = pFeatPoints[0][n];  // maxGrdRow
			fstMinCol         = pFeatPoints[1][n];  // maxGrdCol
			FirstMinSegSharp  = pFeatValues[n];     // maxGrad
		}
		else if(SecondMinSegSharp >= pFeatValues[n])
		{
			// second min 
			sndMinRow         = pFeatPoints[0][n];  // maxGrdRow
			sndMinCol         = pFeatPoints[1][n];  // maxGrdCol
			SecondMinSegSharp = pFeatValues[n];     // maxGrad 
		}
	} // for n
	// first min + second min
	SharpTh = SecondMinSegSharp + FirstMinSegSharp; // sharpness threshold

#elif FEATURE_TH_METHOD == USE_AVE_TH
	// sharpness threshold
	RK_U16      SharpTh; 
	int         sumGrd = 0;
	for (int n=0; n < numFeature; n++)
	{
		sumGrd += pFeatValues[n];
	}
	// ave
	SharpTh = sumGrd / numFeature;

#endif

	// Valid Features
	numValidFeature = 0;            // num of Valid Feature
	for (int n=0; n < numFeature; n++)
	{
		if (pFeatValues[n] >= SharpTh) // Segment
		{
			if (   pFeatPoints[0][n] < COARSE_MATCH_WIN_SIZE
				|| pFeatPoints[1][n] < COARSE_MATCH_WIN_SIZE 
				|| pFeatPoints[0][n] >= nThumbHgt - COARSE_MATCH_WIN_SIZE 
				|| pFeatPoints[1][n] >= nThumbWid - COARSE_MATCH_WIN_SIZE)
			{ 
				// Invalid Feature
			}
			else
			{
				// Valid Feature
				pFeatPoints[0][numValidFeature] = pFeatPoints[0][n];    // maxGrdRow
				pFeatPoints[1][numValidFeature] = pFeatPoints[1][n];    // maxGrdCol
				pFeatValues[numValidFeature]    = pFeatValues[n];       // maxGrad 
				numValidFeature++;
			}
		}        
	}
	// Invalid Feature
	pFeatPoints[0][numValidFeature] = 0;  // maxGrdRow
	pFeatPoints[1][numValidFeature] = 0;  // maxGrdCol
	pFeatValues[numValidFeature]    = 0;  // maxGrad 

	//
	return ret;

#endif
} // FeatureFilter()


/************************************************************************/
// Func: FeatureCoarseMatching()
// Desc: Feature Coarse Matching
//   In: pThumbBase         - [in] ThumbBase data pointer
//       hgt0               - [in] ThumbBase data height
//       wid0               - [in] ThumbBase data width
//       pThumbRef          - [in] ThumbRef data pointer
//       hgt1               - [in] ThumbRef data height
//       wid1               - [in] ThumbRef data width
//  Out: row                - [out] Match Result Row
//       col                - [out] Match Result Col
//       cost               - [out] Match Result Cost
// 
// Date: Revised by yousf 20160804
// 
/*************************************************************************/
CODE_MFNR_EX
int FeatureCoarseMatching(
    RK_U16* pThumbBase, RK_U16 hgt0, RK_U16 wid0, 
    RK_U16* pThumbRef, RK_U16 hgt1, RK_U16 wid1, 
    RK_U16& row, RK_U16& col, RK_U16& cost)
{
#ifndef CEVA_CHIP_CODE_REGISTER
    //
    int     ret = 0; // return value
// #if MY_DEBUG_PRINTF == 1
//     printf("FeatureCoarseMatching()\n");
// #endif  
    // init vars
    RK_U16*         pTmpBase        = NULL;             // temp pointer
    RK_U16*         pTmpRef         = NULL;             // temp pointer
    RK_U32          minSAD;
    RK_U32          curSAD;

    // init min SAD
    minSAD = 0xFFFFFFFF; // 2^32 - 1

    //
    if (hgt0 != COARSE_MATCH_WIN_SIZE || wid0 != COARSE_MATCH_WIN_SIZE)
    {
        ret = -1;
        return ret;
    }


    // Matching
	//for (int i=0; i < hgt1 - hgt0 + 1; i++)
    for (int i=0; i < hgt1 - hgt0 + 1 - 1; i++)
    {
		//for (int j=0; j < wid1 - wid0 + 1; j++)
        for (int j=0; j < wid1 - wid0 + 1 - 1; j++)
        {
            pTmpBase = pThumbBase;                  // Base data
            pTmpRef  = pThumbRef + i*wid1+ j;    // Ref data
            curSAD   = 0;
            for (int m=0; m < hgt0; m++)
            {
                for (int n=0; n < wid0; n++)
                {
                    curSAD += ABS_U16(*pTmpBase - *pTmpRef);
                    //curSAD += MIN(ABS_U16(*pTmpBase - *pTmpRef), 0xFF); // truncate 8bit: // 16x16 * 8bitSAD -> 16bit
                    pTmpBase++;
                    pTmpRef++;
                }
                pTmpRef += (wid1 - wid0);
            }
            if (curSAD < minSAD)
            {
                minSAD = curSAD;
                row    = i;
                col    = j;
            }
        }
    }

    // Matching Min SAD
    cost = minSAD & 0xFFFF;


    //
    return ret;
#else

	int     ret = 0; // return value

	 FeatureCoarseMatching_Vec_vswsad( pThumbBase, pThumbRef, wid0, wid1, hgt0, wid0, hgt1, wid1, cost, row, col );

	return ret;
#endif
} // FeatureCoarseMatching()


/************************************************************************/
// Func: Scaler_Raw2Luma()
// Desc: Scaler Raw to Luma
//   In: pRawData       - [in] Raw data pointer
//       nRawWid        - [in] Raw data width
//       nRawHgt        - [in] Raw data height
//       nThumbWid      - [in] Thumb data width
//       nThumbHgt      - [out] Thumb data height
//  Out: pThumbData     - [out] Thumb data pointer
// 
// Date: Revised by yousf 20160804
// 
/*************************************************************************/
CODE_MFNR_EX
int Scaler_Raw2Luma(RK_U16* pRawData, int nRawWid, int nRawHgt, int nLumaWid, int nLumaHgt, RK_U16* pLumaData)
{
#ifndef CEVA_CHIP_CODE_REGISTER
    //
    int     ret = 0; // return value
// #if MY_DEBUG_PRINTF == 1
//     printf("Scaler_Raw2Luma()\n");
// #endif   
    // init vars
    RK_U16*         pTmp0           = NULL;             // temp pointer for Raw data
    RK_U16*         pTmp1           = NULL;             // temp pointer for Raw data
    RK_U16*         pTmp2           = NULL;             // temp pointer for Luma data
    RK_U16          LumaValue       = 0;                // Luma Value
    int             nScaleRaw2Luma = 2;
    //
    if (nRawHgt/nScaleRaw2Luma != nLumaHgt)
    {
        ret = -1;
        return ret;
    }

    // Raw to Luma -- 1/(2x2)
    pTmp2 = pLumaData;
    for (int i=0; i < nLumaHgt; i++)
    {
        for (int j=0; j <nLumaWid; j++)
        {
            // 2x2 Rect left-top
            pTmp0 = pRawData + (i * nRawWid + j) * nScaleRaw2Luma;

            // average(2x2Raw) -> LumaValue
            LumaValue = 0;
            for (int m=0; m<nScaleRaw2Luma; m++)
            {
                // m-th line in 2x2 Rect
                pTmp1 = pTmp0 + m * nRawWid;
                for (int n=0; n<nScaleRaw2Luma; n++)
                {
                    LumaValue += *(pTmp1++);
                }
            }
            *pTmp2 = LumaValue;
            pTmp2++;
        }
    }

    //
    return ret;

#else
	int ret = 0;

	Scaler_Raw2Luma_Vec( pRawData, pLumaData, nRawWid, nLumaWid, nRawHgt, nRawWid );

	return ret;

#endif
} // Scaler_Raw2Luma()


/************************************************************************/
// Func: FeatureFineMatching()
// Desc: Feature Fine Matching
//   In: pLumaBase          - [in] LumaBase data pointer
//       hgt0               - [in] LumaBase data height
//       wid0               - [in] LumaBase data width
//       pLumaRef           - [in] LumaRef data pointer
//       hgt1               - [in] LumaRef data height
//       wid1               - [in] LumaRef data width
//       col_st             - [in] LumaRef Block Col Start
//       wid_ref            - [in] LumaRef Block Width
//  Out: row                - [out] Match Result Row
//       col                - [out] Match Result Col
//       cost               - [out] Match Result Cost
// 
// Date: Revised by yousf 20160804
// 
/*************************************************************************/
CODE_MFNR_EX
int FeatureFineMatching(
    RK_U16* pLumaBase, RK_U16 hgt0, RK_U16 wid0, 
    RK_U16* pLumaRef, RK_U16 hgt1, RK_U16 wid1, 
    RK_U16 col_st, RK_U16 wid_ref, 
    RK_U16& row, RK_U16& col, RK_U16& cost)
{
#ifndef CEVA_CHIP_CODE_REGISTER
    //
    int     ret = 0; // return value
// #if MY_DEBUG_PRINTF == 1
//     printf("FeatureFineMatching()\n");
// #endif   
    // init vars
    RK_U16*         pTmpBase        = NULL;             // temp pointer
    RK_U16*         pTmpRef         = NULL;             // temp pointer
    RK_U32          minSAD;
    RK_U32          curSAD;

    // init min SAD
    minSAD = 0xFFFFFFFF; // 2^32 - 1

    //
    if (hgt0 != FINE_MATCH_WIN_SIZE/2 || wid0 != FINE_MATCH_WIN_SIZE/2)
    {
        ret = -1;
        return ret;
    }


    // Matching
    int cnt=0;
    for (int i=0; i < hgt1 - hgt0 + 1; i++)
    {
        //for (int j=0; j < wid1 - wid0; j++)
        //for (int j=col_st; j < MIN(col_st + 2*FINE_LUMA_RADIUS+1, wid1-FINE_MATCH_WIN_SIZE/2+1); j++)
        for (int j=col_st; j < col_st + wid_ref - wid0 + 1; j++)
        {
            cnt++;
            pTmpBase = pLumaBase;               // Base data
            pTmpRef  = pLumaRef + i*wid1+ j;    // Ref data
            curSAD   = 0;
            for (int m=0; m < hgt0; m++)
            {
                for (int n=0; n < wid0; n++)
                {
                    curSAD += ABS_U16(*pTmpBase - *pTmpRef);
                    //curSAD += MIN(ABS_U16(*pTmpBase - *pTmpRef), 0x3F); // truncate 6bit: // 32x32 * 6bitSAD -> 16bit
                    pTmpBase++;
                    pTmpRef++;
                }
                pTmpRef += (wid1 - wid0);
            }
            if (curSAD < minSAD)
            {
                minSAD = curSAD;
                row    = i;
                col    = j;
            }
        }
    }

    // Matching Min SAD
    cost = minSAD & 0xFFFF;

    //
    return ret;
#else
int     ret = 0; // return value

	FeatureFineMatching_Vec_vswsad( pLumaBase, pLumaRef, wid0, wid1, col_st, hgt0, wid0, hgt1, wid1, cost, row, col );

	return ret;
#endif
} // FeatureFineMatching()


/************************************************************************/
// Func: MvHistFilter()
// Desc: MV Hist Filter
//   In: pMatchPtsY         - [in] Matching Points Y
//       pMatchPtsX         - [in] Matching Points X
//       numValidFeature    - [in] num of Valid Feature
//       pRowMvHist         - [in] RowMV Hist
//       pColMvHist         - [in] ColMV Hist
//  Out: pMarkMatchFeature  - [out] Marks of Match Feature in BaseFrame & RefFrame#k
// 
// Date: Revised by yousf 20160804
// 
/*************************************************************************/
CODE_MFNR_EX
int MvHistFilter(RK_U16* pMatchPtsY[], RK_U16* pMatchPtsX[], int numValidFeature,
    RK_U8* pRowMvHist, RK_U8* pColMvHist, int nBasePicNum, int nRefPicNum,
    RK_U8* pMarkMatchFeature)
{
#ifndef CEVA_CHIP_CODE_REGISTER
    //
    int     ret = 0; // return value
// #if MY_DEBUG_PRINTF == 1
//     printf("MvHistFilter()\n");
// #endif   
#if USE_MV_HIST_FILTRATE == 1
    //-- Filtrate: Match Feature in BaseFrame & RefFrame#k
    int     MVy, MVx;
    int     minNumValidFeat = MAX((int)(numValidFeature * VALID_FEATURE_RATIO), 1);

    // MV Hist
    memset(pRowMvHist, 0, sizeof(RK_U8) * LEN_MV_HIST);
    memset(pColMvHist, 0, sizeof(RK_U8) * LEN_MV_HIST);
    for (int n=0; n < numValidFeature; n++)
    {
        // MVy
        MVy = pMatchPtsY[nBasePicNum][n] - pMatchPtsY[nRefPicNum][n];
        MVx = pMatchPtsX[nBasePicNum][n] - pMatchPtsX[nRefPicNum][n];
        pRowMvHist[HALF_LEN_MV_HIST + MVy] += 1; // hist +1
        pColMvHist[HALF_LEN_MV_HIST + MVx] += 1; // hist +1
    }

    // MV Hist --> HistMark(0 or 1)
    for (int h=0; h < LEN_MV_HIST; h++)
    {
        // MVy
        if (pRowMvHist[h] > minNumValidFeat)
        {
            pRowMvHist[h] = 1;  // Valid MVy
        }
        else
        {
            pRowMvHist[h] = 0;  // Invalid MVy
        }
        // MVx
        if (pColMvHist[h] > minNumValidFeat)
        {
            pColMvHist[h] = 1;  // Valid MVx
        }
        else
        {
            pColMvHist[h] = 0;  // Invalid MVx
        }
    }
    // HistMark(0 or 1) --> pMarkMatchFeature
    memset(pMarkMatchFeature, 0, sizeof(RK_U8) * MAX_NUM_MATCH_FEATURE);
    if (numValidFeature > MAX_NUM_MATCH_FEATURE)
    {
        ret = -1;
#if MY_DEBUG_PRINTF == 1
        printf("Param Setting Error: MAX_NUM_MATCH_FEATURE is too small !\n");
#endif
        return ret;
    }
    for (int n=0; n < numValidFeature; n++)
    {
        // MVy
        MVy = pMatchPtsY[nBasePicNum][n] - pMatchPtsY[nRefPicNum][n];
        MVx = pMatchPtsX[nBasePicNum][n] - pMatchPtsX[nRefPicNum][n];

        // Valid Feature
        if (pRowMvHist[HALF_LEN_MV_HIST + MVy] == 1 && pColMvHist[HALF_LEN_MV_HIST + MVx] == 1)
        {
            pMarkMatchFeature[n] = 1; // Valid Feature
        }
    }
#else
    memset(pMarkMatchFeature, 1, sizeof(RK_U8) * MAX_NUM_MATCH_FEATURE);
#endif

    //
    return ret;
#else

//
	int     ret = 0; // return value
#if USE_MV_HIST_FILTRATE == 1
	//-- Filtrate: Match Feature in BaseFrame & RefFrame#k
	int     MVy, MVx;
	int     minNumValidFeat = MAX((int)(numValidFeature * VALID_FEATURE_RATIO), 1);

	// MV Hist
	memset(pRowMvHist, 0, sizeof(RK_U8) * LEN_MV_HIST);
	memset(pColMvHist, 0, sizeof(RK_U8) * LEN_MV_HIST);
	for (int n=0; n < numValidFeature; n++)
	{
		// MVy
		MVy = pMatchPtsY[nBasePicNum][n] - pMatchPtsY[nRefPicNum][n];
		MVx = pMatchPtsX[nBasePicNum][n] - pMatchPtsX[nRefPicNum][n];
		pRowMvHist[HALF_LEN_MV_HIST + MVy] += 1; // hist +1
		pColMvHist[HALF_LEN_MV_HIST + MVx] += 1; // hist +1
	}

	// MV Hist --> HistMark(0 or 1)
	for (int h=0; h < LEN_MV_HIST; h++)
	{
		// MVy
		if (pRowMvHist[h] > minNumValidFeat)
		{
			pRowMvHist[h] = 1;  // Valid MVy
		}
		else
		{
			pRowMvHist[h] = 0;  // Invalid MVy
		}
		// MVx
		if (pColMvHist[h] > minNumValidFeat)
		{
			pColMvHist[h] = 1;  // Valid MVx
		}
		else
		{
			pColMvHist[h] = 0;  // Invalid MVx
		}
	}
	// HistMark(0 or 1) --> pMarkMatchFeature
	memset(pMarkMatchFeature, 0, sizeof(RK_U8) * MAX_NUM_MATCH_FEATURE);
	if (numValidFeature > MAX_NUM_MATCH_FEATURE)
	{
		ret = -1;
#if MY_DEBUG_PRINTF == 1
		printf("Param Setting Error: MAX_NUM_MATCH_FEATURE is too small !\n");
#endif
		return ret;
	}
	for (int n=0; n < numValidFeature; n++)
	{
		// MVy
		MVy = pMatchPtsY[nBasePicNum][n] - pMatchPtsY[nRefPicNum][n];
		MVx = pMatchPtsX[nBasePicNum][n] - pMatchPtsX[nRefPicNum][n];

		// Valid Feature
		if (pRowMvHist[HALF_LEN_MV_HIST + MVy] == 1 && pColMvHist[HALF_LEN_MV_HIST + MVx] == 1)
		{
			pMarkMatchFeature[n] = 1; // Valid Feature
		}
	}
#else
	memset(pMarkMatchFeature, 1, sizeof(RK_U8) * MAX_NUM_MATCH_FEATURE);
#endif

	return ret;

#endif
} // MvHistFilter()

/************************************************************************/
// Func: GetRegion4Points()
// Desc: Get 4 points in 4 Regions
//   In: pAgents_Marks      - [in] Agents in 4x4 Region [RegMark4x4] * 16
//       pAgents_PtYs       - [in] Agents in 4x4 Region [Y] * RawFileNum * 16
//       pAgents_PtXs       - [in] Agents in 4x4 Region [X] * RawFileNum * 16
//       pTable             - Region4 Index Table
//       idx                - Region4 Index Table idx
//       numBase            - number of Base Frame
//       numRef             - number of Ref Frame
//  Out: pPoints4           - 4 points in 4 Regions
// 
// Date: Revised by yousf 20160804
// 
/*************************************************************************/
CODE_MFNR_EX
int GetRegion4Points(RK_U8* pAgents_Marks, RK_U16* pAgents_PtYs[], RK_U16* pAgents_PtXs[],
    RK_U8* pTable, int idx, int numBase, int numRef, RK_U16* pPoints4)
{
#ifndef CEVA_CHIP_CODE_REGISTER
    //
    int     ret = 0; // return value
// #if MY_DEBUG_PRINTF == 1
//     printf("GetRegion4Points()\n");
// #endif   
    // Region4 Index Table[idx]
    RK_U8* pTableItem = NULL;
    pTableItem = pTable + idx * 4; 

    // Get 4 points in 4 Regions
    for (int i=0; i < 4; i++)
    {
        if (pAgents_Marks[pTableItem[i]] == MARK_EXIST_AGENT)     // mark
        {
            // BaseFrame Point
            *(pPoints4 + i*4 + 0) = pAgents_PtYs[numBase][pTableItem[i]]; // *(pAgents + *(pTableItem+i) * wid + 2 + numBase * 2 + 0);
            *(pPoints4 + i*4 + 1) = pAgents_PtXs[numBase][pTableItem[i]]; // *(pAgents + *(pTableItem+i) * wid + 2 + numBase * 2 + 1);
            // RefFrame Matching Point
            *(pPoints4 + i*4 + 2) = pAgents_PtYs[numRef][pTableItem[i]]; // *(pAgents + *(pTableItem+i) * wid + 2 + numRef * 2 + 0);
            *(pPoints4 + i*4 + 3) = pAgents_PtXs[numRef][pTableItem[i]]; // *(pAgents + *(pTableItem+i) * wid + 2 + numRef * 2 + 1);
        }
        else
        {
            ret = -1;
            return ret;
        }
    }

    //
    return ret;
#else

int ret = 0;

	// Region4 Index Table[idx]
	RK_U8* pTableItem = NULL;
	pTableItem = pTable + idx * 4; 

	// Get 4 points in 4 Regions
	for (int i=0; i < 4; i++)
	{
		if (pAgents_Marks[pTableItem[i]] == MARK_EXIST_AGENT)     // mark
		{
			// BaseFrame Point
			*(pPoints4 + i*4 + 0) = pAgents_PtYs[numBase][pTableItem[i]]; // *(pAgents + *(pTableItem+i) * wid + 2 + numBase * 2 + 0);
			*(pPoints4 + i*4 + 1) = pAgents_PtXs[numBase][pTableItem[i]]; // *(pAgents + *(pTableItem+i) * wid + 2 + numBase * 2 + 1);
			// RefFrame Matching Point
			*(pPoints4 + i*4 + 2) = pAgents_PtYs[numRef][pTableItem[i]]; // *(pAgents + *(pTableItem+i) * wid + 2 + numRef * 2 + 0);
			*(pPoints4 + i*4 + 3) = pAgents_PtXs[numRef][pTableItem[i]]; // *(pAgents + *(pTableItem+i) * wid + 2 + numRef * 2 + 1);
		}
		else
		{
			ret = -1;
			return ret;
		}
	}

	return ret;
#endif
} // GetRegion4Points()


/************************************************************************/
// Func: classMFNR::CreateCoefficient()
// Desc: Create Coefficient MatrixA & VectorB
//   In: pPoints4       - [in] 4 points in 4 Regions
//  Out: pMatA          - [out] Coefficient MatrixA for A*X = B
//       pVecB          - [out] Coefficient VectorB for A*X = B
// 
// Date: Revised by yousf 20160804
// 
/*************************************************************************/
CODE_MFNR_EX
int CreateCoefficient(RK_U16* pPoints4, RK_F32* pMatA, RK_F32* pVecB)
{
#ifndef CEVA_CHIP_CODE_REGISTER
    //
    int     ret = 0; // return value
// #if MY_DEBUG_PRINTF == 1
//     printf("CreateCoefficient()\n");
// #endif  
    // pPoints4: Base & Ref
    /*
    RK_U16 x00, y00, x01, y01; // 0-pair Matching Points
    RK_U16 x10, y10, x11, y11; // 1-pair Matching Points
    RK_U16 x20, y20, x21, y21; // 2-pair Matching Points
    RK_U16 x30, y30, x31, y31; // 3-pair Matching Points
    */

    //              MatrixA                    VectorB
    /*
    x00 y00 1  0   0   0  -x00*x01 - y00*y01        x01    
    0   0   0  x00 y00 1  -x00*y01 - y00*y01        x01 
    ......
    */

    // Create Coefficient MatrixA & VectorB
    for (int i=0; i < 8; i++)
    {
        if (i<4)
        {
            //-- MatrixA
            *(pMatA + i * 8 + 0) = *(pPoints4 + i*4 + 0); // p[i,0]
            *(pMatA + i * 8 + 1) = *(pPoints4 + i*4 + 1); // p[i,1]
            *(pMatA + i * 8 + 2) = 1;
            *(pMatA + i * 8 + 3) = 0;
            *(pMatA + i * 8 + 4) = 0;
            *(pMatA + i * 8 + 5) = 0;
            *(pMatA + i * 8 + 6) = (RK_F32)(-1 * *(pPoints4 + i*4 + 0) * *(pPoints4 + i*4 + 2)); // p[i,0] * p[i,2]
            *(pMatA + i * 8 + 7) = (RK_F32)(-1 * *(pPoints4 + i*4 + 1) * *(pPoints4 + i*4 + 2)); // p[i,1] * p[i,2]
            //-- VectorB
            *(pVecB + i)         =  *(pPoints4 + i*4 + 2); // p[i,2] = x[i,1]
        }
        else
        {
            //-- MatrixA
            *(pMatA + i * 8 + 0) = 0;
            *(pMatA + i * 8 + 1) = 0;
            *(pMatA + i * 8 + 2) = 0;
            *(pMatA + i * 8 + 3) = *(pPoints4 + (i-4)*4 + 0); // p[i,0]
            *(pMatA + i * 8 + 4) = *(pPoints4 + (i-4)*4 + 1); // p[i,1]
            *(pMatA + i * 8 + 5) = 1;
            *(pMatA + i * 8 + 6) = (RK_F32)(-1 * *(pPoints4 + (i-4)*4 + 0) * *(pPoints4 + (i-4)*4 + 3)); // p[i,0] * p[i,3]
            *(pMatA + i * 8 + 7) = (RK_F32)(-1 * *(pPoints4 + (i-4)*4 + 1) * *(pPoints4 + (i-4)*4 + 3)); // p[i,1] * p[i,3]
            //-- VectorB
            *(pVecB + i)         =  *(pPoints4 + (i-4)*4 + 3); // p[i,3] = y[i,1]
        }
    }
    
    //
    return ret;
#else
//
    int     ret = 0; // return value

 // pPoints4: Base & Ref
    /*
    RK_U16 x00, y00, x01, y01; // 0-pair Matching Points
    RK_U16 x10, y10, x11, y11; // 1-pair Matching Points
    RK_U16 x20, y20, x21, y21; // 2-pair Matching Points
    RK_U16 x30, y30, x31, y31; // 3-pair Matching Points
    */
 //              MatrixA                    VectorB
    /*
    x00 y00 1  0   0   0  -x00*x01 - y00*y01        x01    
    0   0   0  x00 y00 1  -x00*y01 - y00*y01        x01 
    ......
    */

    // Create Coefficient MatrixA & VectorB
    for (int i=0; i < 8; i++)
    {
        if (i<4)
        {
            //-- MatrixA
            *(pMatA + i * 8 + 0) = *(pPoints4 + i*4 + 0); // p[i,0]
            *(pMatA + i * 8 + 1) = *(pPoints4 + i*4 + 1); // p[i,1]
            *(pMatA + i * 8 + 2) = 1;
            *(pMatA + i * 8 + 3) = 0;
            *(pMatA + i * 8 + 4) = 0;
            *(pMatA + i * 8 + 5) = 0;
            *(pMatA + i * 8 + 6) = (RK_F32)(-1 * *(pPoints4 + i*4 + 0) * *(pPoints4 + i*4 + 2)); // p[i,0] * p[i,2]
            *(pMatA + i * 8 + 7) = (RK_F32)(-1 * *(pPoints4 + i*4 + 1) * *(pPoints4 + i*4 + 2)); // p[i,1] * p[i,2]
            //-- VectorB
            *(pVecB + i)         =  *(pPoints4 + i*4 + 2); // p[i,2] = x[i,1]
        }
        else
        {
            //-- MatrixA
            *(pMatA + i * 8 + 0) = 0;
            *(pMatA + i * 8 + 1) = 0;
            *(pMatA + i * 8 + 2) = 0;
            *(pMatA + i * 8 + 3) = *(pPoints4 + (i-4)*4 + 0); // p[i,0]
            *(pMatA + i * 8 + 4) = *(pPoints4 + (i-4)*4 + 1); // p[i,1]
            *(pMatA + i * 8 + 5) = 1;
            *(pMatA + i * 8 + 6) = (RK_F32)(-1 * *(pPoints4 + (i-4)*4 + 0) * *(pPoints4 + (i-4)*4 + 3)); // p[i,0] * p[i,3]
            *(pMatA + i * 8 + 7) = (RK_F32)(-1 * *(pPoints4 + (i-4)*4 + 1) * *(pPoints4 + (i-4)*4 + 3)); // p[i,1] * p[i,3]
            //-- VectorB
            *(pVecB + i)         =  *(pPoints4 + (i-4)*4 + 3); // p[i,3] = y[i,1]
        }
    }
    
    //


	return ret;
#endif
} // CreateCoefficient()


/************************************************************************/
// Func: GetPerspectMatrix()
// Desc: Get a PersPective Matrix
//   In: pMatA          - [in] Coefficient MatrixA for A*X = B
//       pVecB          - [in] Coefficient VectorB for A*X = B
//  Out: pVecX          - [out] Coefficient VectorX for A*X = B
// 
// Date: Revised by yousf 20160804
// 
/*************************************************************************/
CODE_MFNR_EX
int ComputePerspectMatrix(RK_F32* pMatA, RK_F32* pVecB, RK_F32* pVecX)
{
#ifndef CEVA_CHIP_CODE_REGISTER
    //
    int     ret = 0; // return value
// #if MY_DEBUG_PRINTF == 1
//     printf("ComputePerspectMatrix()\n");
// #endif  
    // init vars
    int     i, j, k;
    float   s, t;
    int     n = 8;

    // Gaussian Elimination with Complete Pivoting    st. Ax=b -> Ux=b'
    for (k = 0; k < n; k++)
    {
        // Select Pivot Element: max_i(a[i,j])  
        j = k;  
        t = FABS(pMatA[k*n + k]);
        for (i = k + 1; i<n; i++)
        {
            if ((s = FABS(pMatA[i*n + k])) > t)
            {
                t = s; // max_i{pMatA(i,k)}
                j = i; // argmax_i{pMatA(i,k)}
            }
        }

        //  Ill-conditioned Equation
        if (t < 1.0e-30) 
        {
            ret = -1;
            return ret;  
        }

        // Exchange: pMatA(j,:) - pMatA(k,:)
        if (j != k)
        {
            for (i = k; i < n; i++)
            {
                t              = pMatA[j*n + i];
                pMatA[j*n + i] = pMatA[k*n + i];
                pMatA[k*n + i] = t;
            }
            t        = pVecB[j];
            pVecB[j] = pVecB[k];
            pVecB[k] = t;
        }

        // Recalculation pMatA(k,k:n-1): <- pMatA(k,k) = 1
        t = (RK_F32)(1.0 / pMatA[k*n + k]); 
        //for (i = k + 1; i < n; i++) 
        for (i = k ; i < n; i++) 
        {
            pMatA[k*n + i] *= t;
        }
        pVecB[k] *= t;

        // Elimination: pMatA(k+1:n-1,:)
        for (i = k + 1; i < n; i++)
        {
            t = pMatA[i*n + k];
            //for (j = k + 1; j < n; j++) 
            for (j = k; j < n; j++) 
            {
                pMatA[i*n + j] -= pMatA[k*n + j] * t;
            }
            pVecB[i] -= pVecB[k] * t;
        }
    }

    // U * X = b'  -> X 
    pVecX[n-1] = pVecB[n-1];
    for (i = n - 2; i >= 0; i--)
    {
        for (j = i + 1; j < n; j++) 
        {
            pVecB[i] -= pMatA[i*n + j] * pVecB[j];
        }
        pVecX[i] = pVecB[i];
    }

    //
    return ret;
#else

	int ret = 0;


	ComputePerspectMatrix_Vec( pMatA, pVecB, pVecX );

	return ret;
#endif
} // GetPerspectMatrix()


/************************************************************************/
// Func: PerspectProject()
// Desc: Perspective Project: pVecX * pBasePoint = pProjPoint
//   In: pVecX          - [in] Coefficient VectorX for A*X = B
//       pBasePoint     - [in] PointB(r,c) in BaseFrame
//  Out: pProjPoint     - [out] Perspective Projected PointP(r,c) in RefFrame
// 
// Date: Revised by yousf 20160808
// 
/*************************************************************************/
CODE_MFNR_EX
int PerspectProject(RK_F32* pVecX, RK_F32* pBasePoint, RK_F32* pProjPoint)
{
#ifndef CEVA_CHIP_CODE_REGISTER
    //
    int     ret = 0; // return value
// #if MY_DEBUG_PRINTF == 1
//     printf("PerspectProject()\n");
// #endif   
    // Perspective Projected Coordinate
    RK_F32          X;
    RK_F32          Y;
    RK_F32          Z;
    // Perspective Projected
    //        A(0,0) * Bx                        A(0,1) * By                   A(0,2) * 1
    X = *(pVecX + 0) * *(pBasePoint + 0) + *(pVecX + 1) * *(pBasePoint + 1) + *(pVecX + 2);
    //        A(1,0) * Bx                        A(1,1) * By                   A(1,2) * 1
    Y = *(pVecX + 3) * *(pBasePoint + 0) + *(pVecX + 4) * *(pBasePoint + 1) + *(pVecX + 5);
    //        A(2,0) * Bx                        A(2,1) * By                   A(2,2) * 1
    Z = *(pVecX + 6) * *(pBasePoint + 0) + *(pVecX + 7) * *(pBasePoint + 1) + *(pVecX + 8);

    // Normalization
#if USE_FLOAT_ERROR == 0 // 0-RK_U16, 1-RK_F32
    *(pProjPoint + 0) = (RK_F32)ROUND_I32(X / Z);
    *(pProjPoint + 1) = (RK_F32)ROUND_I32(Y / Z);
#else
    *(pProjPoint + 0) = X / Z; // RK_F32
    *(pProjPoint + 1) = Y / Z; // RK_F32
#endif

    //
    return ret;
#else
	//
	int     ret = 0; // return value
	// #if MY_DEBUG_PRINTF == 1
	//     printf("PerspectProject()\n");
	// #endif   
	// Perspective Projected Coordinate
	RK_F32          X;
	RK_F32          Y;
	RK_F32          Z;
	// Perspective Projected
	//        A(0,0) * Bx                        A(0,1) * By                   A(0,2) * 1
	X = *(pVecX + 0) * *(pBasePoint + 0) + *(pVecX + 1) * *(pBasePoint + 1) + *(pVecX + 2);
	//        A(1,0) * Bx                        A(1,1) * By                   A(1,2) * 1
	Y = *(pVecX + 3) * *(pBasePoint + 0) + *(pVecX + 4) * *(pBasePoint + 1) + *(pVecX + 5);
	//        A(2,0) * Bx                        A(2,1) * By                   A(2,2) * 1
	Z = *(pVecX + 6) * *(pBasePoint + 0) + *(pVecX + 7) * *(pBasePoint + 1) + *(pVecX + 8);

	// Normalization
#if USE_FLOAT_ERROR == 0 // 0-RK_U16, 1-RK_F32
	*(pProjPoint + 0) = ROUND_I32(X / Z);
	*(pProjPoint + 1) = ROUND_I32(Y / Z);
#else
	*(pProjPoint + 0) = X / Z; // RK_F32
	*(pProjPoint + 1) = Y / Z; // RK_F32
#endif

	//
	return ret;

#endif
} // PerspectProject()


/************************************************************************/
// Func: ComputeHomographyError()
// Desc: Compute Homography's Error
//   In: 
//  Out: 
// 
// Date: Revised by yousf 20160818
// 
/*************************************************************************/
CODE_MFNR_EX
int ComputeHomographyError( 
    int     type,        // [in] type=0(AgentsFeatures), type=1(AllFeatures)
    RK_U8*  pMarks,      // [in] Agents'/Features' Marks
    int     numFeature,  // [in] Num of Agents/Features
    RK_U16* pPointYs[],  // [in] Agents'/Features' Rows
    RK_U16* pPointXs[],  // [in] Agents'/Features' Cols
    int     nBasePicNum, // [in] Base #0
    int     nRefPicNum,  // [in] Ref #k
    RK_F32* pVectorX,    // [in] pVecX * pBasePoint = pProjectPoint
    RK_F32* pBasePoint,  // [in] pVecX * pBasePoint = pProjectPoint
    RK_F32* pProjPoint,  // [in] pVecX * pBasePoint = pProjectPoint
    RK_F32* pRefPoint,   // [in] abs(pRefPoint - pProjPoint)
    RK_U32& error)       // [out] Correct Project Count / Sum Project Errors
{
#ifndef CEVA_CHIP_CODE_REGISTER
    //
    int     ret = 0; // return value

    // type = 0
    RK_U32      corrCnt = 0;                // Correct Project Count <-- Error Threshold of Valid Homography
#if USE_EARLY_STOP_H == 1 // 1-use Early Stop Compute Homography
    int         goodCnt;                    // Correct Project Count <-- Error Threshold of Good Homography
#endif

    // type = 1
#if USE_FLOAT_ERROR == 0 // 0-RK_U16, 1-RK_F32
    RK_S32      errRow, errCol;             // Project Errors: row error & col error
    RK_U32      errSum_H = 0;               // Sum Project Errors: sum error of all features
    //RK_U32      errMin_H;                   // Min Project Errors: min error of best Homography for BaseFrame--RefFrame#k
#else
    RK_F32      errRow, errCol;             // Project Errors: row error & col error
    RK_U32      errSum_H = 0;               // Sum Project Errors: sum error of all features
    //RK_U32      errMin_H;                    // Min Project Errors: min error of best Homography for BaseFrame--RefFrame#k

#endif


    //
    for (int n=0; n < numFeature; n++)
    {
        if (pMarks[n] == 1)
        {
            // Perspective Project: pVecX * pBasePoint = pProjectPoint
            pBasePoint[0] = (RK_F32)pPointYs[nBasePicNum][n];
            pBasePoint[1] = (RK_F32)pPointXs[nBasePicNum][n];
            pRefPoint[0]  = (RK_F32)pPointYs[nRefPicNum][n];
            pRefPoint[1]  = (RK_F32)pPointXs[nRefPicNum][n];

            ret = PerspectProject(pVectorX, pBasePoint, pProjPoint);
            if (ret)
            {
#if MY_DEBUG_PRINTF == 1
                printf("Failed to PerspectProject !\n");
#endif
                //return ret;
                continue;
            }

            // Project Errors
#if USE_FLOAT_ERROR == 0 // 0-RK_U16, 1-RK_F32
            errRow = ABS_U16(pRefPoint[0] - pProjPoint[0]);
            errCol = ABS_U16(pRefPoint[1] - pProjPoint[1]);   
#else
            errRow = FABS(pRefPoint[0] - pProjPoint[0]);
            errCol = FABS(pRefPoint[1] - pProjPoint[1]); 
#endif


            //////////////////////////////////////////////////////////////////////////
            if (type==0)
            {
                // Error Threshold of Valid Homography
                if (errRow <= ERR_TH_VALID_H && errCol <= ERR_TH_VALID_H)
                {
                    corrCnt++; // Correct Project Count <-- Error Threshold of Valid Homography
                }

#if USE_EARLY_STOP_H == 1 // 1-use Early Stop Compute Homography
                // Error Threshold of Good Homography
                if (errRow <= ERR_TH_GOOD_H && errCol <= ERR_TH_GOOD_H)
                {
                    goodCnt++; // Correct Project Count <-- Error Threshold of Good Homography
                }
#endif
                
            }
            else // type = 1
            {
#if USE_FLOAT_ERROR == 0 // 0-RK_U16, 1-RK_F32
                errRow = MIN(errRow, MAX_PROJECT_ERROR);
                errCol = MIN(errCol, MAX_PROJECT_ERROR);
#else
                errRow = (RK_F32)ROUND_U32(MIN(errRow*16384.0, 0xFFFFFFFF)); // 2^14=16384
                errCol = (RK_F32)ROUND_U32(MIN(errCol*16384.0, 0xFFFFFFFF));
#endif
                // Sum Project Errors: sum error of all features
                errSum_H += (RK_U32)(errRow + errCol);

            }
            
           
        }
    } // for n

    if (type==0)
    {
        // output
        error = corrCnt;    // Correct Project Count
    }
    else
    {
        // output
        error = errSum_H;    // Sum Project Errors
    }

    //
    return ret;
#else
	//
	int     ret = 0; // return value

	ushort *pBasePointX = pPointXs[ nBasePicNum ];
	ushort *pBasePointY = pPointYs[ nBasePicNum ];
	ushort *pSrcPointX = pPointXs[ nRefPicNum ];
	ushort *pSrcPointY = pPointYs[ nRefPicNum ];

#if USE_FLOAT_ERROR == 0 

	ComputeHomographyError_Vec_uint( type, pMarks, pVectorX, pBasePointX, pBasePointY, pSrcPointX, pSrcPointY, numFeature, error );

#else

	ComputeHomographyError_Vec_float( type, pMarks, pVectorX, pBasePointX, pBasePointY, pSrcPointX, pSrcPointY, numFeature, error );

#endif

	//
	return ret;

#endif

} // ComputeHomographyError()


//////////////////////////////////////////////////////////////////////////

CODE_MFNR_EX
////////////////////////////////////////////////////////////////////////////////////////////
// add by shm @2016.08.30
//       v0_1                         v0_4
// v1_0  v1_1  v1_2             v1_3  v1_4  v1_5
// v2_0  v2_1  v2_2             v2_3  v2_4  v2_5
//       v3_1                         v3_4
////////////////////////////////////////////////////////////////////////////////////////////
void FeatureDetect_Vec( RK_U16 *p_s16Src,  // <<! [ in ] block data: 34 * 522
						RK_U16 &maxVal,    // <<! [ out ] max gradient value in 32 * 32
						RK_U16 &maxGrdRow, // <<! [ out ] max gradient row
						RK_U16 &maxGrdCol, // <<! [ out ] max gradient col
						RK_U32 srcStride,  // <<! [ in ] src stride: 522
						RK_U32 offsetX,    // <<! [ in ] offset in each 32 * 32  boloc: 522
						RK_U32 u32Rows,    // <<! [ in ] feature detect row s: 32
						RK_U32 u32Cols)    // <<! [ in ] feature detect col: 32
{   
	//coeff: boundary restriction
	RK_U16 coeff[ 32 ] = { 0, 32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768, 0 };

	ushort16 v0_1, v1_0, v1_1, v1_2, v2_0, v2_1, v2_2, v3_1, dummy;
	ushort16 v0_4, v1_3, v1_4, v1_5, v2_3, v2_4, v2_5, v3_4;
	uint16 dx0_0, dx0_1, dx1_0, dx1_1, grad0_0, grad0_1, grad1_0, grad1_1;
	ushort16 result0_0, result1_0, result0_1, result1_1;

	uint16 vmaximum0 = ( uint16 )( 0 );
	uint16 vmaximum1 = ( uint16 )( 0 );

	uint16 w0_0, w0_1, w1_0, w1_1;
	maxVal = 0;
	RK_U32 step = 2 * srcStride;

	ushort16 v_coeff0, v_coeff1;

	RK_U16 *p0_0, *p1_0, *p2_0, *p3_0;
	RK_U16 *p0_1, *p1_1, *p2_1, *p3_1;
	RK_U16 *p_src = p_s16Src + offsetX;

	//v_coeff0: 0~15, v_coeff1: 16~31
	v_coeff0 = *( ushort16* )( coeff );  v_coeff1 = *( ushort16* )( coeff + 16 );

	p0_0 = p_src; p0_1 = p0_0 + 16;
	p1_0 = p_src + srcStride - 1; p1_1 = p1_0 + 16;
	p2_0 = p1_0 + srcStride; p2_1 = p2_0 + 16;
	p3_0 = p0_0 + 3 * srcStride; p3_1 = p3_0 + 16;

	v0_1 = *( ushort16* )( p0_0 ); v0_4 = *( ushort16* )( p0_1 );
	p0_0 += step; p0_1 += step;

	for( RK_U32 row = 1; row < u32Rows - 1; row += 2 )
	{
		vldov( p1_0, v1_0, v1_1, v1_2, dummy ); 
		vldov( p1_1, v1_3, v1_4, v1_5, dummy ); 
		p1_0 += step; p1_1 += step;

		vldov( p2_0, v2_0, v2_1, v2_2, dummy ); 
		vldov( p2_1, v2_3, v2_4, v2_5, dummy ); 
		p2_0 += step; p2_1 += step;

		v3_1 = *( ushort16* )( p3_0 ); v3_4 = *( ushort16* )( p3_1 );
		p3_0 += step; p3_1 += step;

		//abs( dx )
		dx0_0  = ( uint16 )vabssub( v1_0, v1_2 ); dx0_1 =  ( uint16 )vabssub( v1_3, v1_5 );
		//abs( dx )+ abs( dy )
		grad0_0 = vabssubacc( v0_1, v2_1, dx0_0 ); grad0_1 = vabssubacc( v0_4, v2_4, dx0_1 );
		//( abs( dx )+ abs( dy ) ) >> 1
		result0_0 = vacccast( satu, grad0_0 >> 1 ); result0_1 = vacccast( satu, grad0_1 >> 1 );

		//abs( dx )
		dx1_0  = ( uint16 )vabssub( v2_0, v2_2 ); dx1_1 =  ( uint16 )vabssub( v2_3, v2_5 );
		//abs( dx )+ abs( dy )
		grad1_0 = vabssubacc( v1_1, v3_1, dx1_0 ); grad1_1 = vabssubacc( v1_4, v3_4, dx1_1 );
		//( abs( dx )+ abs( dy ) ) >> 1
		result1_0 = vacccast( satu, grad1_0 >> 1 ); result1_1 = vacccast( satu, grad1_1 >> 1 );

		//next two line
		v0_1 = v2_1; v0_4 = v2_4;

		//compare two lines' gradient and vmaximum
		w0_0 = ( uint16 )( row );  w0_1 = ( uint16 )( row );
		w1_0 = ( uint16 )( row + 1 );  w1_1 = ( uint16 )( row + 1 );
		//gradient << 15, row num are stored in the low 15bits
		w0_0 = vmac( result0_0, v_coeff0, w0_0 );  w0_1 = vmac( result0_1, v_coeff1, w0_1 ); 
		w1_0 = vmac( result1_0, v_coeff0, w1_0 );  w1_1 = vmac( result1_1, v_coeff1, w1_1 ); 

		uint8 maxlo0 = vmax( vunpack_lo( w0_0 ), vunpack_lo( w1_0 ), vunpack_lo( vmaximum0 ) );
		uint8 maxhi0 = vmax( vunpack_hi( w0_0 ), vunpack_hi( w1_0 ), vunpack_hi( vmaximum0 ) );

		uint8 maxlo1 = vmax( vunpack_lo( w0_1 ), vunpack_lo( w1_1 ), vunpack_lo( vmaximum1 ) );
		uint8 maxhi1 = vmax( vunpack_hi( w0_1 ), vunpack_hi( w1_1 ), vunpack_hi( vmaximum1 ) );

		vmaximum0 = vpack( maxlo0, maxhi0 ); vmaximum1 = vpack( maxlo1, maxhi1);
	}
	RK_U32 max1 = 0, max2 = 0;

	RK_U8 vprMax1, vprMax2;
	RK_U8 VprMask1, VprMask2;
	VprMask1 = -1;
	VprMask2 = -1;

	vintramax( vunpack_lo( vmaximum0 ), VprMask1, max1, vprMax1 );
	vintramax( vunpack_hi( vmaximum0 ), VprMask2, max2, vprMax2 );

	if ( ( max1 >> 15 ) > maxVal ) 
	{
		//maxVal are in high 15bits
		maxVal = ( RK_U16 )( max1 >> 15 );
		//the corresponding row are in low 15bits
		maxGrdRow = ( max1 & 0x7FFF );
		maxGrdCol = ffb( set, lsb, vprMax1 );
	}

	if ( ( max2 >> 15 ) > maxVal ) 
	{
		maxVal = ( RK_U16 )( max2 >> 15 );
		maxGrdRow = ( max2 & 0x7FFF );
		maxGrdCol = 8 + ffb( set, lsb, vprMax2 );
	}

	vintramax( vunpack_lo( vmaximum1 ), VprMask1, max1, vprMax1 );
	vintramax( vunpack_hi( vmaximum1 ), VprMask2, max2, vprMax2 );

	if ( ( max1 >> 15 ) > maxVal ) 
	{
		//maxVal are in high 15bits
		maxVal = ( RK_U16 )( max1 >> 15 );
		//the corresponding row are in low 15bits
		maxGrdRow = ( max1 & 0x7FFF );
		maxGrdCol = 16 + ffb( set, lsb, vprMax1 );
	}

	if ( ( max2 >> 15 ) > maxVal ) 
	{
		maxVal = ( RK_U16 )( max2 >> 15 );
		maxGrdRow = ( max2 & 0x7FFF );
		maxGrdCol = 24 + ffb( set, lsb, vprMax2 );
	}
}

CODE_MFNR_EX
////////////////////////////////////////////////////////////////////////////////////////////////
// add by shm @2016.08.30
// base data                  src data
// v0_0          |    v1_0               v2_0
// v0_1          |    v1_1               v2_1
// v0_2          |    v1_2               v2_2
// v0_3          |    v1_3               v2_3
// v0_4          |    v1_4               v2_4
// ...
// v0_15         |
////////////////////////////////////////////////////////////////////////////////////////////////
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
										RK_U16 &minCol )    // <<! [ out ]: minSad col
{
	//base data
	ushort16 v0_0, v0_1, v0_2, v0_3, v0_4, v0_5, v0_6, v0_7, v0_8, v0_9, v0_10, v0_11, v0_12, v0_13, v0_14, v0_15;

	//src data: 0 ~15
	ushort16 v1_0, v1_1, v1_2, v1_3, v1_4;
	//src data: 16 ~31
	ushort16 v2_0, v2_1, v2_2, v2_3, v2_4;

	//four lines sads in 16 positions
	uint16 result0, result1, result2, result3;
	uint16 vminimum = ( uint16 )( 0xFFFFFFFF );

	RK_U16 *p1, *p2, *p3;

	RK_U32 offsetSrc[ 6 ];
	for ( RK_U32 k = 0; k < 6; k ++ )
	{
		offsetSrc[ k ] = stride2 * k;
	}

	//load base data
	v0_0 = *( ushort16* )( p_s16Src1 + 0 ); v0_1 = *( ushort16* )( p_s16Src1 + 16 ); 
	v0_2 = *( ushort16* )( p_s16Src1 + 32 ); v0_3 = *( ushort16* )( p_s16Src1 + 48 );
	v0_4 = *( ushort16* )( p_s16Src1 + 64 ); v0_5 = *( ushort16* )( p_s16Src1 + 80 );
	v0_6 = *( ushort16* )( p_s16Src1 + 96 ); v0_7 = *( ushort16* )( p_s16Src1 + 112 );
	v0_8 = *( ushort16* )( p_s16Src1 + 128 ); v0_9 = *( ushort16* )( p_s16Src1 + 144 );
	v0_10 = *( ushort16* )( p_s16Src1 + 160 ); v0_11 = *( ushort16* )( p_s16Src1 + 176 );
	v0_12 = *( ushort16* )( p_s16Src1 + 192 ); v0_13 = *( ushort16* )( p_s16Src1 + 208 );
	v0_14 = *( ushort16* )( p_s16Src1 + 224 ); v0_15 = *( ushort16* )( p_s16Src1 + 240 );

	//one loop: 4*16 sad,search range: 16x16, loop times: 4
	RK_U32 outLoopY = ( u32Rows2 - u32Rows1 ) >> 2;
	for( RK_U32 k = 0; k < outLoopY; k ++ )
	{
		result0 = 0; result1 = 0; result2 = 0; result3 = 0;

		//4 line sad, need to read data: 16+4=20 line, one time to read 5 line data, loop times: 4
		p1 = p_s16Src2; p_s16Src2 += 4 * stride2;

		p2 = p1; p3 = p1 + 16;
		//first time to read 5 line data of src2
		v1_0 = *( ushort16* )( p2 + offsetSrc[ 0 ] ); v1_1 = *( ushort16* )( p2 + offsetSrc[ 1 ] );
		v1_2 = *( ushort16* )( p2 + offsetSrc[ 2 ] ); v1_3 = *( ushort16* )( p2 + offsetSrc[ 3 ] );
		v1_4 = *( ushort16* )( p2 + offsetSrc[ 4 ] ); p2 += offsetSrc[ 5 ];

		v2_0 = *( ushort16* )( p3 + offsetSrc[ 0 ] ); v2_1 = *( ushort16* )( p3 + offsetSrc[ 1 ] );
		v2_2 = *( ushort16* )( p3 + offsetSrc[ 2 ] ); v2_3 = *( ushort16* )( p3 + offsetSrc[ 3 ] );
		v2_4 = *( ushort16* )( p3 + offsetSrc[ 4 ] ); p3 += offsetSrc[ 5 ];

		for ( RK_U32 kx = 0; kx < 8; kx ++ )
		{			
			result0 = vswsad( accumulate, v1_0, v2_0, v0_0, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result0 );
			result1 = vswsad( accumulate, v1_1, v2_1, v0_0, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result1 );
			result2 = vswsad( accumulate, v1_2, v2_2, v0_0, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result2 );
			result3 = vswsad( accumulate, v1_3, v2_3, v0_0, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result3 );

			result0 = vswsad( accumulate, v1_1, v2_1, v0_1, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result0 );
			result1 = vswsad( accumulate, v1_2, v2_2, v0_1, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result1 );
			result2 = vswsad( accumulate, v1_3, v2_3, v0_1, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result2 );
			result3 = vswsad( accumulate, v1_4, v2_4, v0_1, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result3 );

			result0 = vswsad( accumulate, v1_2, v2_2, v0_2, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result0 );
			result1 = vswsad( accumulate, v1_3, v2_3, v0_2, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result1 );
			result2 = vswsad( accumulate, v1_4, v2_4, v0_2, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result2 );

			result0 = vswsad( accumulate, v1_3, v2_3, v0_3, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result0 );
			result1 = vswsad( accumulate, v1_4, v2_4, v0_3, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result1 );

			result0 = vswsad( accumulate, v1_4, v2_4, v0_4, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result0 );

		}

		//second time to read 5 line data of src2
		v1_0 = *( ushort16* )( p2 + offsetSrc[ 0 ] ); v1_1 = *( ushort16* )( p2 + offsetSrc[ 1 ] );
		v1_2 = *( ushort16* )( p2 + offsetSrc[ 2 ] ); v1_3 = *( ushort16* )( p2 + offsetSrc[ 3 ] );
		v1_4 = *( ushort16* )( p2 + offsetSrc[ 4 ] ); p2 += offsetSrc[ 5 ];

		v2_0 = *( ushort16* )( p3 + offsetSrc[ 0 ] ); v2_1 = *( ushort16* )( p3 + offsetSrc[ 1 ] );
		v2_2 = *( ushort16* )( p3 + offsetSrc[ 2 ] ); v2_3 = *( ushort16* )( p3 + offsetSrc[ 3 ] );
		v2_4 = *( ushort16* )( p3 + offsetSrc[ 4 ] ); p3 += offsetSrc[ 5 ];

		for ( RK_U32 kx = 0; kx < 8; kx ++ )
		{			
			result3 = vswsad( accumulate, v1_0, v2_0, v0_2, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result3 );

			result2 = vswsad( accumulate, v1_0, v2_0, v0_3, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result2 );
			result3 = vswsad( accumulate, v1_1, v2_1, v0_3, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result3 );

			result1 = vswsad( accumulate, v1_0, v2_0, v0_4, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result1 );
			result2 = vswsad( accumulate, v1_1, v2_1, v0_4, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result2 );
			result3 = vswsad( accumulate, v1_2, v2_2, v0_4, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result3 );

			result0 = vswsad( accumulate, v1_0, v2_0, v0_5, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result0 );
			result1 = vswsad( accumulate, v1_1, v2_1, v0_5, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result1 );
			result2 = vswsad( accumulate, v1_2, v2_2, v0_5, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result2 );
			result3 = vswsad( accumulate, v1_3, v2_3, v0_5, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result3 );

			result0 = vswsad( accumulate, v1_1, v2_1, v0_6, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result0 );
			result1 = vswsad( accumulate, v1_2, v2_2, v0_6, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result1 );
			result2 = vswsad( accumulate, v1_3, v2_3, v0_6, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result2 );
			result3 = vswsad( accumulate, v1_4, v2_4, v0_6, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result3 );

			result0 = vswsad( accumulate, v1_2, v2_2, v0_7, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result0 );
			result1 = vswsad( accumulate, v1_3, v2_3, v0_7, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result1 );
			result2 = vswsad( accumulate, v1_4, v2_4, v0_7, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result2 );

			result0 = vswsad( accumulate, v1_3, v2_3, v0_8, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result0 );
			result1 = vswsad( accumulate, v1_4, v2_4, v0_8, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result1 );

			result0 = vswsad( accumulate, v1_4, v2_4, v0_9, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result0 );

		}

		//third time to read 5 line data of src2
		v1_0 = *( ushort16* )( p2 + offsetSrc[ 0 ] ); v1_1 = *( ushort16* )( p2 + offsetSrc[ 1 ] );
		v1_2 = *( ushort16* )( p2 + offsetSrc[ 2 ] ); v1_3 = *( ushort16* )( p2 + offsetSrc[ 3 ] );
		v1_4 = *( ushort16* )( p2 + offsetSrc[ 4 ] ); p2 += offsetSrc[ 5 ];

		v2_0 = *( ushort16* )( p3 + offsetSrc[ 0 ] ); v2_1 = *( ushort16* )( p3 + offsetSrc[ 1 ] );
		v2_2 = *( ushort16* )( p3 + offsetSrc[ 2 ] ); v2_3 = *( ushort16* )( p3 + offsetSrc[ 3 ] );
		v2_4 = *( ushort16* )( p3 + offsetSrc[ 4 ] ); p3 += offsetSrc[ 5 ];

		for ( RK_U32 kx = 0; kx < 8; kx ++ )
		{			
			result3 = vswsad( accumulate, v1_0, v2_0, v0_7, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result3 );

			result2 = vswsad( accumulate, v1_0, v2_0, v0_8, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result2 );
			result3 = vswsad( accumulate, v1_1, v2_1, v0_8, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result3 );

			result1 = vswsad( accumulate, v1_0, v2_0, v0_9, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result1 );
			result2 = vswsad( accumulate, v1_1, v2_1, v0_9, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result2 );
			result3 = vswsad( accumulate, v1_2, v2_2, v0_9, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result3 );

			result0 = vswsad( accumulate, v1_0, v2_0, v0_10, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result0 );
			result1 = vswsad( accumulate, v1_1, v2_1, v0_10, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result1 );
			result2 = vswsad( accumulate, v1_2, v2_2, v0_10, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result2 );
			result3 = vswsad( accumulate, v1_3, v2_3, v0_10, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result3 );

			result0 = vswsad( accumulate, v1_1, v2_1, v0_11, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result0 );
			result1 = vswsad( accumulate, v1_2, v2_2, v0_11, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result1 );
			result2 = vswsad( accumulate, v1_3, v2_3, v0_11, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result2 );
			result3 = vswsad( accumulate, v1_4, v2_4, v0_11, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result3 );

			result0 = vswsad( accumulate, v1_2, v2_2, v0_12, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result0 );
			result1 = vswsad( accumulate, v1_3, v2_3, v0_12, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result1 );
			result2 = vswsad( accumulate, v1_4, v2_4, v0_12, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result2 );

			result0 = vswsad( accumulate, v1_3, v2_3, v0_13, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result0 );
			result1 = vswsad( accumulate, v1_4, v2_4, v0_13, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result1 );

			result0 = vswsad( accumulate, v1_4, v2_4, v0_14, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result0 );

		}

		//fouth time to read 5 line data of src2
		v1_0 = *( ushort16* )( p2 + offsetSrc[ 0 ] ); v1_1 = *( ushort16* )( p2 + offsetSrc[ 1 ] );
		v1_2 = *( ushort16* )( p2 + offsetSrc[ 2 ] ); v1_3 = *( ushort16* )( p2 + offsetSrc[ 3 ] );
		v1_4 = *( ushort16* )( p2 + offsetSrc[ 4 ] ); p2 += offsetSrc[ 5 ];

		v2_0 = *( ushort16* )( p3 + offsetSrc[ 0 ] ); v2_1 = *( ushort16* )( p3 + offsetSrc[ 1 ] );
		v2_2 = *( ushort16* )( p3 + offsetSrc[ 2 ] ); v2_3 = *( ushort16* )( p3 + offsetSrc[ 3 ] );
		v2_4 = *( ushort16* )( p3 + offsetSrc[ 4 ] ); p3 += offsetSrc[ 5 ];

		for ( RK_U32 kx = 0; kx < 8; kx ++ )
		{			
			result3 = vswsad( accumulate, v1_0, v2_0, v0_12, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result3 );

			result2 = vswsad( accumulate, v1_0, v2_0, v0_13, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result2 );
			result3 = vswsad( accumulate, v1_1, v2_1, v0_13, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result3 );

			result1 = vswsad( accumulate, v1_0, v2_0, v0_14, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result1 );
			result2 = vswsad( accumulate, v1_1, v2_1, v0_14, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result2 );
			result3 = vswsad( accumulate, v1_2, v2_2, v0_14, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result3 );

			result0 = vswsad( accumulate, v1_0, v2_0, v0_15, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result0 );
			result1 = vswsad( accumulate, v1_1, v2_1, v0_15, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result1 );
			result2 = vswsad( accumulate, v1_2, v2_2, v0_15, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result2 );
			result3 = vswsad( accumulate, v1_3, v2_3, v0_15, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result3 );
		}
		//data: 16 bits, sad num: 16 * 16 = 8bits, sad data: 16 + 8 = 24 bits
		result0 = ( result0 << 7 ) + k * 4 + 0;
		result1 = ( result1 << 7 ) + k * 4 + 1;
		result2 = ( result2 << 7 ) + k * 4 + 2;
		result3 = ( result3 << 7 ) + k * 4 + 3;

		//min sad
		uint8 maxlo = vmin( vunpack_lo( result0 ), vunpack_lo( result1 ), vunpack_lo( vminimum ) );
		uint8 maxhi = vmin( vunpack_hi( result0 ), vunpack_hi( result1 ), vunpack_hi( vminimum ) );
		vminimum = vpack(maxlo, maxhi);

		maxlo = vmin( vunpack_lo( result2 ), vunpack_lo( result3 ), vunpack_lo( vminimum ) );
		maxhi = vmin( vunpack_hi( result2 ), vunpack_hi( result3 ), vunpack_hi( vminimum ) );
		vminimum = vpack(maxlo, maxhi);
	}

	//min sad
	RK_U32 min1, min2;
	RK_U8 vprMin1, vprMin2;
	RK_U8 VprMask1 = -1;
	RK_U8 VprMask2 = -1;

	vintramin( vunpack_lo( vminimum ), VprMask1, min1, vprMin1 );
	vintramin( vunpack_hi( vminimum ), VprMask2, min2, vprMin2 );

	RK_U32 minSad = 0xFFFFFFFF;
	if ( ( min1 >> 7 ) < minSad ) 
	{
		minSad = min1 >> 7 ;
		minRow = min1 & 0x7F;
		minCol = ffb( set, lsb, vprMin1 );
	}

	if ( ( min2 >> 7 ) < minSad ) 
	{
		minSad = min2 >> 7;
		minRow = min2 & 0x7F;
		minCol = 8 + ffb( set, lsb, vprMin2 );
	}
	minSAD = minSad;
}

CODE_MFNR_EX
////////////////////////////////////////////////////////////////////////////////////////////////
// add by shm @2016.08.30
// base data                  src data
// v0_0       v1_0    |    v2_0               v3_0
// v0_1       v1_1    |    v2_1               v3_1
// v0_2       v1_2    |    v2_2               v3_2
// v0_3       v1_3    |    v2_3               v3_3
// v0_4       v1_4    |    v2_4               v3_4
// v0_5       v1_5    |
////////////////////////////////////////////////////////////////////////////////////////////////
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
									 RK_U16 &minCol )   // <<! [ out ]: minSad col
{
	// base data: 0 ~ 15
	ushort16 v0_0, v0_1, v0_2, v0_3, v0_4, v0_5, v0_6;
	// base data:15 ~ 31
	ushort16 v1_0, v1_1, v1_2, v1_3, v1_4, v1_5, v1_6;
	// src data: 0 ~ 15
	ushort16 v2_0, v2_1, v2_2, v2_3, v2_4;
	// src data: 16 ~ 31
	ushort16 v3_0, v3_1, v3_2, v3_3, v3_4;
	// src data: 32 ~ 47
	ushort16 v4_0, v4_1, v4_2, v4_3, v4_4;

	//three lines sads in 16 positions
	uint16 result0, result1, result2;
	uint16 vminimum = ( uint16 )( 0xFFFFFFFF );

	RK_U16 *p1, *p2, *p3, *p4;
	RK_U16 *p_src1, *p_src2;

	RK_U32 offsetSrc1[ 14 ];
	for ( RK_U32 k = 0; k < 14; k ++ )
	{
		offsetSrc1[ k ] = 16 * k;
	}

	RK_U32 offsetSrc2[ 6 ];
	for ( RK_U32 k = 0; k < 6; k ++ )
	{
		offsetSrc2[ k ] = stride2 * k;
	}

	//one loop: 3*16 sad,search range: 11x11, loop times: 4	
	RK_U32 outLoopY = ( ( u32Rows2 - u32Rows1 ) + 2 ) / 3;
	p_src2 = p_s16Src2 + offsetX;
	for( RK_U32 k = 0; k < outLoopY; k ++ )
	{
		result0 = 0; result1 = 0; result2 = 0;

		//3 line sad, need to read data: 132+3=35 line, one time to read 5 line data, loop times: 7
		p1 = p_src2; p_src2 += 3 * stride2;
		p2 = p1; p3 = p1 + 16; p4 = p1 + 32;

		//first time to read 5 line data of src1
		p_src1 = p_s16Src1; 
		v0_0 = *( ushort16* )( p_src1 + offsetSrc1[ 0 ] ); v1_0 = *( ushort16* )( p_src1 + offsetSrc1[ 1 ] ); 
		v0_1 = *( ushort16* )( p_src1 + offsetSrc1[ 2 ] ); v1_1 = *( ushort16* )( p_src1 + offsetSrc1[ 3 ] );
		v0_2 = *( ushort16* )( p_src1 + offsetSrc1[ 4 ] ); v1_2 = *( ushort16* )( p_src1 + offsetSrc1[ 5 ] );
		v0_3 = *( ushort16* )( p_src1 + offsetSrc1[ 6 ] ); v1_3 = *( ushort16* )( p_src1 + offsetSrc1[ 7 ] );
		v0_4 = *( ushort16* )( p_src1 + offsetSrc1[ 8 ] ); v1_4 = *( ushort16* )( p_src1 + offsetSrc1[ 9 ] );
		p_src1 += 3 * stride1;

		//first time to read 5 line data of src2
		v2_0 = *( ushort16* )( p2 + offsetSrc2[ 0 ] ); v2_1 = *( ushort16* )( p2 + offsetSrc2[ 1 ] );
		v2_2 = *( ushort16* )( p2 + offsetSrc2[ 2 ] ); v2_3 = *( ushort16* )( p2 + offsetSrc2[ 3 ] );
		v2_4 = *( ushort16* )( p2 + offsetSrc2[ 4 ] ); p2 += offsetSrc2[ 5 ];

		v3_0 = *( ushort16* )( p3 + offsetSrc2[ 0 ] ); v3_1 = *( ushort16* )( p3 + offsetSrc2[ 1 ] );
		v3_2 = *( ushort16* )( p3 + offsetSrc2[ 2 ] ); v3_3 = *( ushort16* )( p3 + offsetSrc2[ 3 ] );
		v3_4 = *( ushort16* )( p3 + offsetSrc2[ 4 ] ); p3 += offsetSrc2[ 5 ];

		v4_0 = *( ushort16* )( p4 + offsetSrc2[ 0 ] ); v4_1 = *( ushort16* )( p4 + offsetSrc2[ 1 ] );
		v4_2 = *( ushort16* )( p4 + offsetSrc2[ 2 ] ); v4_3 = *( ushort16* )( p4 + offsetSrc2[ 3 ] );
		v4_4 = *( ushort16* )( p4 + offsetSrc2[ 4 ] ); p4 += offsetSrc2[ 5 ];

		for ( RK_U32 kx = 0; kx < 8; kx ++ )
		{			
			result0 = vswsad( accumulate, v2_0, v3_0, v0_0, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result0 );
			result1 = vswsad( accumulate, v2_1, v3_1, v0_0, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result1 );
			result2 = vswsad( accumulate, v2_2, v3_2, v0_0, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result2 );
			result0 = vswsad( accumulate, v3_0, v4_0, v1_0, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result0 );
			result1 = vswsad( accumulate, v3_1, v4_1, v1_0, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result1 );
			result2 = vswsad( accumulate, v3_2, v4_2, v1_0, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result2 );

			result0 = vswsad( accumulate, v2_1, v3_1, v0_1, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result0 );
			result1 = vswsad( accumulate, v2_2, v3_2, v0_1, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result1 );
			result2 = vswsad( accumulate, v2_3, v3_3, v0_1, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result2 );
			result0 = vswsad( accumulate, v3_1, v4_1, v1_1, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result0 );
			result1 = vswsad( accumulate, v3_2, v4_2, v1_1, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result1 );
			result2 = vswsad( accumulate, v3_3, v4_3, v1_1, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result2 );

			result0 = vswsad( accumulate, v2_2, v3_2, v0_2, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result0 );
			result1 = vswsad( accumulate, v2_3, v3_3, v0_2, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result1 );
			result2 = vswsad( accumulate, v2_4, v3_4, v0_2, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result2 );
			result0 = vswsad( accumulate, v3_2, v4_2, v1_2, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result0 );
			result1 = vswsad( accumulate, v3_3, v4_3, v1_2, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result1 );
			result2 = vswsad( accumulate, v3_4, v4_4, v1_2, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result2 );

			result0 = vswsad( accumulate, v2_3, v3_3, v0_3, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result0 );
			result1 = vswsad( accumulate, v2_4, v3_4, v0_3, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result1 );
			result0 = vswsad( accumulate, v3_3, v4_3, v1_3, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result0 );
			result1 = vswsad( accumulate, v3_4, v4_4, v1_3, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result1 );

			result0 = vswsad( accumulate, v2_4, v3_4, v0_4, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result0 );
			result0 = vswsad( accumulate, v3_4, v4_4, v1_4, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result0 );

		}
		//second time to sixth time
		for( RK_U32 n = 0; n < 5; n ++ )
		{
			//second time to read 7 line data of src1
			v0_0 = *( ushort16* )( p_src1 + offsetSrc1[ 0 ] ); v1_0 = *( ushort16* )( p_src1 + offsetSrc1[ 1 ] ); 
			v0_1 = *( ushort16* )( p_src1 + offsetSrc1[ 2 ] ); v1_1 = *( ushort16* )( p_src1 + offsetSrc1[ 3 ] );
			v0_2 = *( ushort16* )( p_src1 + offsetSrc1[ 4 ] ); v1_2 = *( ushort16* )( p_src1 + offsetSrc1[ 5 ] );
			v0_3 = *( ushort16* )( p_src1 + offsetSrc1[ 6 ] ); v1_3 = *( ushort16* )( p_src1 + offsetSrc1[ 7 ] );
			v0_4 = *( ushort16* )( p_src1 + offsetSrc1[ 8 ] ); v1_4 = *( ushort16* )( p_src1 + offsetSrc1[ 9 ] );
			v0_5 = *( ushort16* )( p_src1 + offsetSrc1[ 10 ] ); v1_5 = *( ushort16* )( p_src1 + offsetSrc1[ 11 ] );
			v0_6 = *( ushort16* )( p_src1 + offsetSrc1[ 12 ] ); v1_6 = *( ushort16* )( p_src1 + offsetSrc1[ 13 ] );
			p_src1 += 5 * stride1;

			//second time to read 5 line data of src2
			v2_0 = *( ushort16* )( p2 + offsetSrc2[ 0 ] ); v2_1 = *( ushort16* )( p2 + offsetSrc2[ 1 ] );
			v2_2 = *( ushort16* )( p2 + offsetSrc2[ 2 ] ); v2_3 = *( ushort16* )( p2 + offsetSrc2[ 3 ] );
			v2_4 = *( ushort16* )( p2 + offsetSrc2[ 4 ] ); p2 += offsetSrc2[ 5 ];

			v3_0 = *( ushort16* )( p3 + offsetSrc2[ 0 ] ); v3_1 = *( ushort16* )( p3 + offsetSrc2[ 1 ] );
			v3_2 = *( ushort16* )( p3 + offsetSrc2[ 2 ] ); v3_3 = *( ushort16* )( p3 + offsetSrc2[ 3 ] );
			v3_4 = *( ushort16* )( p3 + offsetSrc2[ 4 ] ); p3 += offsetSrc2[ 5 ];

			v4_0 = *( ushort16* )( p4 + offsetSrc2[ 0 ] ); v4_1 = *( ushort16* )( p4 + offsetSrc2[ 1 ] );
			v4_2 = *( ushort16* )( p4 + offsetSrc2[ 2 ] ); v4_3 = *( ushort16* )( p4 + offsetSrc2[ 3 ] );
			v4_4 = *( ushort16* )( p4 + offsetSrc2[ 4 ] ); p4 += offsetSrc2[ 5 ];

			for ( RK_U32 kx = 0; kx < 8; kx ++ )
			{		
				result2 = vswsad( accumulate, v2_0, v3_0, v0_0, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result2 );
				result2 = vswsad( accumulate, v3_0, v4_0, v1_0, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result2 );

				result1 = vswsad( accumulate, v2_0, v3_0, v0_1, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result1 );
				result2 = vswsad( accumulate, v2_1, v3_1, v0_1, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result2 );
				result1 = vswsad( accumulate, v3_0, v4_0, v1_1, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result1 );
				result2 = vswsad( accumulate, v3_1, v4_1, v1_1, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result2 );

				result0 = vswsad( accumulate, v2_0, v3_0, v0_2, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result0 );
				result1 = vswsad( accumulate, v2_1, v3_1, v0_2, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result1 );
				result2 = vswsad( accumulate, v2_2, v3_2, v0_2, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result2 );
				result0 = vswsad( accumulate, v3_0, v4_0, v1_2, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result0 );
				result1 = vswsad( accumulate, v3_1, v4_1, v1_2, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result1 );
				result2 = vswsad( accumulate, v3_2, v4_2, v1_2, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result2 );

				result0 = vswsad( accumulate, v2_1, v3_1, v0_3, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result0 );
				result1 = vswsad( accumulate, v2_2, v3_2, v0_3, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result1 );
				result2 = vswsad( accumulate, v2_3, v3_3, v0_3, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result2 );
				result0 = vswsad( accumulate, v3_1, v4_1, v1_3, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result0 );
				result1 = vswsad( accumulate, v3_2, v4_2, v1_3, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result1 );
				result2 = vswsad( accumulate, v3_3, v4_3, v1_3, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result2 );

				result0 = vswsad( accumulate, v2_2, v3_2, v0_4, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result0 );
				result1 = vswsad( accumulate, v2_3, v3_3, v0_4, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result1 );
				result2 = vswsad( accumulate, v2_4, v3_4, v0_4, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result2 );
				result0 = vswsad( accumulate, v3_2, v4_2, v1_4, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result0 );
				result1 = vswsad( accumulate, v3_3, v4_3, v1_4, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result1 );
				result2 = vswsad( accumulate, v3_4, v4_4, v1_4, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result2 );

				result0 = vswsad( accumulate, v2_3, v3_3, v0_5, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result0 );
				result1 = vswsad( accumulate, v2_4, v3_4, v0_5, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result1 );
				result0 = vswsad( accumulate, v3_3, v4_3, v1_5, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result0 );
				result1 = vswsad( accumulate, v3_4, v4_4, v1_5, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result1 );

				result0 = vswsad( accumulate, v2_4, v3_4, v0_6, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result0 );
				result0 = vswsad( accumulate, v3_4, v4_4, v1_6, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result0 );

			}
		}

		//seventh time to read 4 line data of src1
		v0_0 = *( ushort16* )( p_src1 + offsetSrc1[ 0 ] ); v1_0 = *( ushort16* )( p_src1 + offsetSrc1[ 1 ] ); 
		v0_1 = *( ushort16* )( p_src1 + offsetSrc1[ 2 ] ); v1_1 = *( ushort16* )( p_src1 + offsetSrc1[ 3 ] );
		v0_2 = *( ushort16* )( p_src1 + offsetSrc1[ 4 ] ); v1_2 = *( ushort16* )( p_src1 + offsetSrc1[ 5 ] );
		v0_3 = *( ushort16* )( p_src1 + offsetSrc1[ 6 ] ); v1_3 = *( ushort16* )( p_src1 + offsetSrc1[ 7 ] );

		//first time to read 5 line data of src2
		v2_0 = *( ushort16* )( p2 + offsetSrc2[ 0 ] ); v2_1 = *( ushort16* )( p2 + offsetSrc2[ 1 ] );
		v2_2 = *( ushort16* )( p2 + offsetSrc2[ 2 ] ); v2_3 = *( ushort16* )( p2 + offsetSrc2[ 3 ] );
		v2_4 = *( ushort16* )( p2 + offsetSrc2[ 4 ] ); p2 += offsetSrc2[ 5 ];

		v3_0 = *( ushort16* )( p3 + offsetSrc2[ 0 ] ); v3_1 = *( ushort16* )( p3 + offsetSrc2[ 1 ] );
		v3_2 = *( ushort16* )( p3 + offsetSrc2[ 2 ] ); v3_3 = *( ushort16* )( p3 + offsetSrc2[ 3 ] );
		v3_4 = *( ushort16* )( p3 + offsetSrc2[ 4 ] ); p3 += offsetSrc2[ 5 ];

		v4_0 = *( ushort16* )( p4 + offsetSrc2[ 0 ] ); v4_1 = *( ushort16* )( p4 + offsetSrc2[ 1 ] );
		v4_2 = *( ushort16* )( p4 + offsetSrc2[ 2 ] ); v4_3 = *( ushort16* )( p4 + offsetSrc2[ 3 ] );
		v4_4 = *( ushort16* )( p4 + offsetSrc2[ 4 ] ); p4 += offsetSrc2[ 5 ];
		for ( RK_U32 kx = 0; kx < 8; kx ++ )
		{		
			result2 = vswsad( accumulate, v2_0, v3_0, v0_0, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result2 );
			result2 = vswsad( accumulate, v3_0, v4_0, v1_0, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result2 );

			result1 = vswsad( accumulate, v2_0, v3_0, v0_1, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result1 );
			result2 = vswsad( accumulate, v2_1, v3_1, v0_1, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result2 );
			result1 = vswsad( accumulate, v3_0, v4_0, v1_1, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result1 );
			result2 = vswsad( accumulate, v3_1, v4_1, v1_1, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result2 );

			result0 = vswsad( accumulate, v2_0, v3_0, v0_2, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result0 );
			result1 = vswsad( accumulate, v2_1, v3_1, v0_2, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result1 );
			result2 = vswsad( accumulate, v2_2, v3_2, v0_2, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result2 );
			result0 = vswsad( accumulate, v3_0, v4_0, v1_2, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result0 );
			result1 = vswsad( accumulate, v3_1, v4_1, v1_2, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result1 );
			result2 = vswsad( accumulate, v3_2, v4_2, v1_2, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result2 );

			result0 = vswsad( accumulate, v2_1, v3_1, v0_3, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result0 );
			result1 = vswsad( accumulate, v2_2, v3_2, v0_3, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result1 );
			result2 = vswsad( accumulate, v2_3, v3_3, v0_3, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result2 );
			result0 = vswsad( accumulate, v3_1, v4_1, v1_3, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result0 );
			result1 = vswsad( accumulate, v3_2, v4_2, v1_3, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result1 );
			result2 = vswsad( accumulate, v3_3, v4_3, v1_3, SW_CONFIG( 0, 1, kx * 2, kx * 2, 0, 0 ), result2 );
		}
		//data: 10 bits, sad num: 32 * 32 = 10bits, sad data: 10 + 10 = 20 bits
		result0 = ( result0 << 8 ) + k * 3 + 0;
		result1 = ( result1 << 8 ) + k * 3 + 1;
		result2 = ( result2 << 8 ) + k * 3 + 2;

		uint8 maxlo = vmin( vunpack_lo( result0 ), vunpack_lo( result1 ), vunpack_lo( vminimum ) );
		uint8 maxhi = vmin( vunpack_hi( result0 ), vunpack_hi( result1 ), vunpack_hi( vminimum ) );
		vminimum = vpack(maxlo, maxhi);

		maxlo = vmin( vunpack_lo( result2 ), vunpack_lo( vminimum ) );
		maxhi = vmin( vunpack_hi( result2 ), vunpack_hi( vminimum ) );
		vminimum = vpack(maxlo, maxhi);
	}

	//min sad
	RK_U32 min1, min2;
	RK_U8 vprMin1, vprMin2;
	RK_U8 VprMask1 = -1;
	RK_U8 VprMask2 = 7;

	vintramin( vunpack_lo( vminimum ), VprMask1, min1, vprMin1 );
	vintramin( vunpack_hi( vminimum ), VprMask2, min2, vprMin2 );

	RK_U32 minSad = 0xFFFFFFFF;
	if ( ( min1 >> 8 ) < minSad ) 
	{
		minSad = min1 >> 8 ;
		minRow = min1 & 0xFF;
		minCol = ffb( set, lsb, vprMin1 ) + offsetX;
	}

	if ( ( min2 >> 8 ) < minSad ) 
	{
		minSad = min2 >> 8;
		minRow = min2 & 0xFF;
		minCol = 8 + ffb( set, lsb, vprMin2 ) + offsetX;
	}
	minSAD = minSad;
}

CODE_MFNR_EX
////////////////////////////////////////////////////////////////////////////////////////////////
// add by shm @2016.08.30
// raw data                  
// vr0   vgr0        vr1   vgr1    | result0    result1
// vgb0   vb0        vgb1   vb1    |
//
////////////////////////////////////////////////////////////////////////////////////////////////
void Scaler_Raw2Luma_Vec( RK_U16 *p_s16Src,   // <<! [ in ]: src raw data
						  RK_U16 *p_s16Dst,   // <<! [ out ]: luma data
						  RK_U32 srcStride,   // <<! [ in ]: src raw stride
						  RK_U32 dstStride,   // <<! [ in ]: luma stride
						  RK_U32 u32Rows,     // <<! [ in ]:raw data row
						  RK_U32 u32Cols )    // <<! [ in ]:raw data col  
{
	//raw data
	ushort16 vr0, vgr0, vgb0, vb0, vr1, vgr1, vgb1, vb1;
	//luma data
	ushort16 result0, result1;

	ushort16 *p_out0, *p_out1;
	RK_U16 *p0, *p1, *p_s16DstTmp;

	RK_U16 vprMask = -1;
	RK_U16 vprMaskLastCol = ( 1 << ( ( u32Cols & 31 ) >> 1 ) ) - 1;

	RK_U32 step1 = 3 * srcStride;
	RK_U32 step2 = 2 * dstStride;

	p_s16DstTmp = p_s16Dst;
	for( RK_U32 col = 0; col < u32Cols; col += 32 )
	{
		p0 = p_s16Src; p1 = p_s16Src + 2 * srcStride; p_s16Src += 32;
		p_out0 = ( ushort16* )( p_s16DstTmp ); p_out1 = ( ushort16* )( p_s16DstTmp + dstStride ); 
		p_s16DstTmp += 16;

		if( col +32 > u32Cols )
		{
			vprMask = vprMaskLastCol;
		}

		for( RK_U32 row = 0; row < u32Rows; row += 4 )
		{
			vldchk( p0, vr0, vgr0 ); p0 += srcStride; 
			vldchk( p1, vr1, vgr1 ); p1 += srcStride; 
			vldchk( p0, vgb0, vb0 ); p0 += step1; 
			vldchk( p1, vgb1, vb1 ); p1 += step1;

			result0 = vr0 + vgr0 + vb0 + vgb0;
			result1 = vr1 + vgr1 + vb1 + vgb1;

			vst( result0, p_out0, vprMask ); p_out0 = ( ushort16* )( ( RK_U16* )p_out0 + step2 );
			vst( result1, p_out1, vprMask ); p_out1 = ( ushort16* )( ( RK_U16* )p_out1 + step2 );
		}
	}

}

CODE_MFNR_EX
////////////////////////////////////////////////////////////////////////////////////////////////
// add by shm @2016.08.30
// A * x = b                 
//
////////////////////////////////////////////////////////////////////////////////////////////////
RK_S32 ComputePerspectMatrix_Vec( RK_F32 *A,     // <<! [ in ] A
								  RK_F32 *b,     // <<! [ in ] b
							      RK_F32 *OutX ) // <<! [ in ] homography
{
	RK_S32 i, j, k;
	RK_S32 *pFbin, FData;

	RK_F32 MaxData;
	RK_F32 DataTmp;

	RK_F32 DataTmp0;

	RK_F32  *pData0;
	RK_F32  *pData1;
	float8 *pVec[9];
	float8 *pVecTmp;

	float8 VecTmpT;
	float8 VecTmpS;

	for( RK_S32 i = 0; i < 8; i ++ )
	{
		pVec[ i ] = ( float8 * )( A + i * 8 );
	}

	pVec[ 8 ] = ( float8 * )b;

	for (k = 0; k < 8; k++)
	{
		j = k;
		pData0 = (RK_F32 *)pVec[k];
		// MaxData = fabs(pData0[k]);
		pFbin = (RK_S32 *)&pData0[k];
		FData = *pFbin & 0x7fffffff;
		MaxData = *(RK_F32 *)&FData;
		//MaxData = ( RK_F32 )FData;
		DataTmp0 = pData0[k];

		for (i = k + 1; i < 8; i++)
		{
			pData1 = (RK_F32 *)pVec[i];
			// DataTmp = fabs(pData1[k]);
			pFbin = (RK_S32 *)&pData1[k];
			FData = *pFbin & 0x7fffffff;
			DataTmp = *(RK_F32 *)&FData;
			//DataTmp = ( RK_F32 )FData;

			if (DataTmp > MaxData)
			{
				DataTmp0 = pData1[k];
				MaxData = DataTmp;
				j = i;
			}
		}
		VecTmpS = (float8)DataTmp0;
		if (MaxData < 1.0e-30)
		{
			return 1;        // The equation is ill conditioned , or has infinitely many solutions
		}

		if (j != k)          // exchange the j line and k line
		{
			pVecTmp = pVec[j];
			pVec[j] = pVec[k];
			pVec[k] = pVecTmp;

			DataTmp = b[j];
			b[j] = b[k];
			b[k] = DataTmp;
		}
		VecTmpT = vfpinv(VecTmpS);             // turn the diagonal element into 1.0
		*pVec[k] = vfpmpy(*pVec[k], VecTmpT);  //
		b[k] *= *(RK_F32 *)&VecTmpT;

		for (i = k + 1; i < 8; i++)            // elimination the k col element in k+1 to n-1 row
		{
			pData0 = (RK_F32 *)pVec[i];
			DataTmp = pData0[k];
			VecTmpS = (float8)pData0[k];

			VecTmpT = vfpmpy(*pVec[k], VecTmpS); //
			*pVec[i] = vfpsub(*pVec[i], VecTmpT);
			b[i] -= b[k] * DataTmp;
		}
	} // end of k

	pData0 = (RK_F32 *)&VecTmpT;
	// i = 6
	VecTmpT = vfpmpy(*pVec[6], *pVec[8]); //
	b[6] -= pData0[7];

	// i = 5
	VecTmpT = vfpmpy(*pVec[5], *pVec[8]); //
	b[5] -= pData0[6];
	b[5] -= pData0[7];

	// i = 4
	VecTmpT = vfpmpy(*pVec[4], *pVec[8]); //
	b[4] -= pData0[5];
	b[4] -= pData0[6];
	b[4] -= pData0[7];

	// i = 3
	VecTmpT = vfpmpy(*pVec[3], *pVec[8]); //
	b[3] -= pData0[4];
	b[3] -= pData0[5];
	b[3] -= pData0[6];
	b[3] -= pData0[7];

	// i = 2
	VecTmpT = vfpmpy(*pVec[2], *pVec[8]); //
	b[2] -= pData0[3];
	b[2] -= pData0[4];
	b[2] -= pData0[5];
	b[2] -= pData0[6];
	b[2] -= pData0[7];

	// i = 1
	VecTmpT = vfpmpy(*pVec[1], *pVec[8]); //
	b[1] -= pData0[2];
	b[1] -= pData0[3];
	b[1] -= pData0[4];
	b[1] -= pData0[5];
	b[1] -= pData0[6];
	b[1] -= pData0[7];

	// i = 0
	VecTmpT = vfpmpy(*pVec[0], *pVec[8]); //
	b[0] -= pData0[1];
	b[0] -= pData0[2];
	b[0] -= pData0[3];
	b[0] -= pData0[4];
	b[0] -= pData0[5];
	b[0] -= pData0[6];
	b[0] -= pData0[7];

	for ( RK_S32 i = 0; i <= 7; i ++ )
	{
		OutX[ i ] = b[ i ];
	}
	OutX[ 8 ] = 1;

	return 0;
}

CODE_MFNR_EX
////////////////////////////////////////////////////////////////////////////////////////////////
// add by shm @2016.08.30                
//
////////////////////////////////////////////////////////////////////////////////////////////////
void ComputeHomographyError_Vec_float( RK_S32 type,             // <<! [ in ]: type: 0: correct num, 1: sum error
									   RK_U8 *pMasks,			// <<! [ in ]: valid match points mask   
									   RK_F32 *pH,              // <<! [ in ]: homography
									   RK_U16 *pBasePointX,     // <<! [ in ]: base pointsX 
									   RK_U16 *pBasePointY,     // <<! [ in ]: base pointsY 
									   RK_U16 *pSrcPointX,      // <<! [ in ]: match src pointsX 
									   RK_U16 *pSrcPointY,      // <<! [ in ]: match src pointsY 
									   RK_U32 u32Cols,          // <<! [ in ]: points num
									   RK_U32 &error )          // <<! [ out ]: correct num or sum error
{
	RK_U8 vprMask1 = -1;
	RK_U8 vprMask2 = -1;

	RK_U8 vprMaskLastCol1 = -1;
	RK_U8 vprMaskLastCol2 = -1;

	if ( u32Cols & 15 )
	{
		RK_U32 bits = u32Cols & 15;

		if (bits < 8) 
		{
			vprMaskLastCol1 = (1 << bits) - 1;
			vprMaskLastCol2 = 0;
		}
		else
			vprMaskLastCol2 = (1 << (bits - 8)) - 1;
	}

	//scale the project points, scale the project error
	RK_S32 shiftBit = 14;
	RK_U32 scale = 1 << shiftBit;

	//homography
	RK_F32 a0 = pH[ 0 ];
	RK_F32 a1 = pH[ 1 ];
	RK_F32 a2 = pH[ 2 ];
	RK_F32 a3 = pH[ 3 ];
	RK_F32 a4 = pH[ 4 ];
	RK_F32 a5 = pH[ 5 ];
	RK_F32 a6 = pH[ 6 ];
	RK_F32 a7 = pH[ 7 ];
	RK_F32 a8 = pH[ 8 ];

	float8 v_baseX_lo, v_baseX_hi, v_baseY_lo, v_baseY_hi;
	float8 numeratorX_lo, numeratorX_hi, numeratorY_lo, numeratorY_hi, denominator_lo, denominator_hi;
	float8 v_projectX_lo, v_projectX_hi, v_projectY_lo, v_projectY_hi;
	uint8 v_projectX_lo_uint, v_projectX_hi_uint, v_projectY_lo_uint, v_projectY_hi_uint;

	uint16 v_srcX, v_srcY;

	uint8 projectErrorX_lo, projectErrorX_hi, projectErrorY_lo, projectErrorY_hi;

	error = 0;

	RK_U16 *p_BaseX, *p_BaseY;
	RK_U16 *p_SrcX, *p_SrcY;

	p_BaseX = pBasePointX; p_BaseY = pBasePointY; 
	p_SrcX = pSrcPointX; p_SrcY = pSrcPointY;

	RK_U32 outLoop = ( u32Cols + 15 ) >> 4;
	for ( RK_U32 k = 0; k < outLoop; k ++ )
	{
		if( k == outLoop - 1  )
		{
			vprMask1 = vprMaskLastCol1;
			vprMask2 = vprMaskLastCol2;
		}
		//16 base points
		uint16 tmp = ( uint16 ) *( ushort16* )( p_BaseX ); p_BaseX += 16;
		uint16 tmp2 = ( uint16 ) *( ushort16* )( p_BaseY ); p_BaseY += 16;

		v_baseX_lo = ( float8 )( vunpack_lo( tmp ) );
		v_baseX_hi = ( float8 )( vunpack_hi( tmp ) );

		v_baseY_lo = ( float8 )( vunpack_lo( tmp2 ) );
		v_baseY_hi = ( float8 )( vunpack_hi( tmp2 ) );

		//y
		numeratorY_lo = a0 * v_baseY_lo + a1 * v_baseX_lo + a2;
		numeratorY_hi = a0 * v_baseY_hi + a1 * v_baseX_hi + a2;

		//x
		numeratorX_lo = a3 * v_baseY_lo + a4 * v_baseX_lo + a5;
		numeratorX_hi = a3 * v_baseY_hi + a4 * v_baseX_hi + a5;

		//denominator
		denominator_lo = a6 * v_baseY_lo + a7 * v_baseX_lo + a8;
		denominator_hi = a6 * v_baseY_hi + a7 * v_baseX_hi + a8;

		denominator_lo = vfpinv( denominator_lo );
		denominator_hi = vfpinv( denominator_hi );

		//projectX¡¢projectY
		v_projectX_lo = vfpmpy( numeratorX_lo, denominator_lo );
		v_projectX_hi = vfpmpy( numeratorX_hi, denominator_hi );

		v_projectY_lo = vfpmpy( numeratorY_lo, denominator_lo );
		v_projectY_hi = vfpmpy( numeratorY_hi, denominator_hi );

		v_projectX_lo_uint = ( uint8 )( scale * v_projectX_lo );
		v_projectX_hi_uint = ( uint8 )( scale * v_projectX_hi );

		v_projectY_lo_uint = ( uint8 )( scale * v_projectY_lo );
		v_projectY_hi_uint = ( uint8 )( scale * v_projectY_hi );

		//16 src points
		v_srcX = ( uint16 )*( ushort16* )( p_SrcX ); p_SrcX += 16;
		v_srcY = ( uint16 )*( ushort16* )( p_SrcY ); p_SrcY += 16;

		//scale src points
		v_srcX = v_srcX << shiftBit;
		v_srcY = v_srcY << shiftBit;

		//scaled project error
		projectErrorX_lo = vabssub( vunpack_lo( v_srcX ), v_projectX_lo_uint );
		projectErrorX_hi = vabssub( vunpack_hi( v_srcX ), v_projectX_hi_uint );

		projectErrorY_lo = vabssub( vunpack_lo( v_srcY ), v_projectY_lo_uint );
		projectErrorY_hi = vabssub( vunpack_hi( v_srcY ), v_projectY_hi_uint );

		if( type == 0 )
		{
			//valid match points
			uint16 mask_char = ( uint16 ) *( uchar16* )( pMasks ); pMasks += 16;

			//invalid match points,set invalid match points' to 2 * ERR_TH_VALID_H
			uint16 mask_char_not = 1 - mask_char;
			mask_char_not = mask_char_not * 2 * ERR_TH_VALID_H * scale;

			uint8 mask_char_lo = ( uint8 )vadd( vunpack_lo( mask_char ), vunpack_lo( mask_char_not ) );
			uint8 mask_char_hi = ( uint8 )vadd( vunpack_hi( mask_char ), vunpack_hi( mask_char_not ) );

			projectErrorX_lo = ( uint8 )vmpy( lsb, projectErrorX_lo, mask_char_lo );
			projectErrorX_hi = ( uint8 )vmpy( lsb, projectErrorX_hi, mask_char_hi );

			projectErrorY_lo = ( uint8 )vmpy( lsb, projectErrorY_lo, mask_char_lo );
			projectErrorY_hi = ( uint8 )vmpy( lsb, projectErrorY_hi, mask_char_hi );

			//error less than ERR_TH_VALID_H
			RK_U8 num1_X = vabscmp( le, ( int8 )projectErrorX_lo,( int8 )( ERR_TH_VALID_H * scale ) );
			RK_U8 num1_Y = vabscmp( le, ( int8 )projectErrorY_lo,( int8 )( ERR_TH_VALID_H * scale ) );
			RK_U8 num2_X = vabscmp( le, ( int8 )projectErrorX_hi,( int8 )( ERR_TH_VALID_H * scale ) );
			RK_U8 num2_Y = vabscmp( le, ( int8 )projectErrorY_hi,( int8 )( ERR_TH_VALID_H * scale ) );

			RK_U8 num1 = num1_X & num1_Y;
			RK_U8 num2 = num2_X & num2_Y;

			error = cntbits( num1 ) + cntbits( num2 );

		}
		else
		{
			//valid match points
			uint16 mask_char = ( uint16 ) *( uchar16* )( pMasks ); pMasks += 16;

			uint8 mask_char_lo = vunpack_lo( mask_char );
			uint8 mask_char_hi = vunpack_hi( mask_char );

			//invalid match points,set invalid match points' to 0
			projectErrorX_lo = ( uint8 )vmpy( lsb, mask_char_lo, projectErrorX_lo );
			projectErrorX_hi = ( uint8 )vmpy( lsb, mask_char_hi, projectErrorX_hi );

			projectErrorY_lo = ( uint8 )vmpy( lsb, mask_char_lo, projectErrorY_lo );
			projectErrorY_hi = ( uint8 )vmpy( lsb, mask_char_hi, projectErrorY_hi );

			RK_S32 curSumError1 = vintrasum( projectErrorX_lo, vprMask1 ) + vintrasum( projectErrorY_lo, vprMask1 );
			RK_S32 curSumError2 = vintrasum( projectErrorX_hi, vprMask2 ) + vintrasum( projectErrorY_hi, vprMask2 );

			//overflow: points num(8 bits) + scale( 14 bits ) + th < 32bits, th < 32 - 22 = 9bits
			curSumError1 = MIN( curSumError1 + curSumError2, 0x7FFFFF );

			error += curSumError1;

		}
	}	
}

CODE_MFNR_EX
////////////////////////////////////////////////////////////////////////////////////////////////
// add by shm @2016.08.30                
//
////////////////////////////////////////////////////////////////////////////////////////////////
void ComputeHomographyError_Vec_uint( RK_S32 type,              // <<! [ in ]: type: 0: correct num, 1: sum error
									  RK_U8 *pMasks,            // <<! [ in ]: valid match points mask
									  RK_F32 *pH,               // <<! [ in ]: homography
									  RK_U16 *pBasePointX,      // <<! [ in ]: base pointsX
									  RK_U16 *pBasePointY,      // <<! [ in ]: base pointsY
									  RK_U16 *pSrcPointX,       // <<! [ in ]: match src pointsX   
									  RK_U16 *pSrcPointY,       // <<! [ in ]: match src pointsY 
									  RK_U32 u32Cols,           // <<! [ in ]: points num
									  RK_U32 &error )           // <<! [ out ]: correct num or sum error
{
	RK_U8 vprMask1 = -1;
	RK_U8 vprMask2 = -1;

	RK_U8 vprMaskLastCol1 = -1;
	RK_U8 vprMaskLastCol2 = -1;

	if ( u32Cols & 15 )
	{
		RK_U32 bits = u32Cols & 15;

		if (bits < 8) 
		{
			vprMaskLastCol1 = (1 << bits) - 1;
			vprMaskLastCol2 = 0;
		}
		else
			vprMaskLastCol2 = (1 << (bits - 8)) - 1;
	}

	//homography
	RK_F32 a0 = pH[ 0 ];
	RK_F32 a1 = pH[ 1 ];
	RK_F32 a2 = pH[ 2 ];
	RK_F32 a3 = pH[ 3 ];
	RK_F32 a4 = pH[ 4 ];
	RK_F32 a5 = pH[ 5 ];
	RK_F32 a6 = pH[ 6 ];
	RK_F32 a7 = pH[ 7 ];
	RK_F32 a8 = pH[ 8 ];

	float8 v_baseX_lo, v_baseX_hi, v_baseY_lo, v_baseY_hi;
	float8 numeratorX_lo, numeratorX_hi, numeratorY_lo, numeratorY_hi, denominator_lo, denominator_hi;
	uint8 v_projectX_lo_uint, v_projectX_hi_uint, v_projectY_lo_uint, v_projectY_hi_uint;

	uint16 v_srcX, v_srcY;

	uint8 projectErrorX_lo, projectErrorX_hi, projectErrorY_lo, projectErrorY_hi;

	error = 0;

	RK_U16 *p_BaseX, *p_BaseY;
	RK_U16 *p_SrcX, *p_SrcY;

	p_BaseX = pBasePointX; p_BaseY = pBasePointY; 
	p_SrcX = pSrcPointX; p_SrcY = pSrcPointY;

	RK_U32 outLoop = ( u32Cols + 15 ) >> 4;
	for ( RK_U32 k = 0; k < outLoop; k ++ )
	{
		if( k == outLoop - 1  )
		{
			vprMask1 = vprMaskLastCol1;
			vprMask2 = vprMaskLastCol2;
		}
		//16 base points
		uint16 tmp = ( uint16 ) *( ushort16* )( p_BaseX ); p_BaseX += 16;
		uint16 tmp2 = ( uint16 ) *( ushort16* )( p_BaseY ); p_BaseY += 16;

		v_baseX_lo = ( float8 )( vunpack_lo( tmp ) );
		v_baseX_hi = ( float8 )( vunpack_hi( tmp ) );

		v_baseY_lo = ( float8 )( vunpack_lo( tmp2 ) );
		v_baseY_hi = ( float8 )( vunpack_hi( tmp2 ) );

		//y
		numeratorY_lo = a0 * v_baseY_lo + a1 * v_baseX_lo + a2;
		numeratorY_hi = a0 * v_baseY_hi + a1 * v_baseX_hi + a2;

		//x
		numeratorX_lo = a3 * v_baseY_lo + a4 * v_baseX_lo + a5;
		numeratorX_hi = a3 * v_baseY_hi + a4 * v_baseX_hi + a5;

		//denominator
		denominator_lo = a6 * v_baseY_lo + a7 * v_baseX_lo + a8;
		denominator_hi = a6 * v_baseY_hi + a7 * v_baseX_hi + a8;

		denominator_lo = vfpinv( denominator_lo );
		denominator_hi = vfpinv( denominator_hi );

		//projectX¡¢projectY, rounding
		v_projectX_lo_uint = ( uint8 )( 0.5 + vfpmpy( numeratorX_lo, denominator_lo ) );
		v_projectX_hi_uint = ( uint8 )( 0.5 + vfpmpy( numeratorX_hi, denominator_hi ) );

		v_projectY_lo_uint = ( uint8 )( 0.5 + vfpmpy( numeratorY_lo, denominator_lo ) );
		v_projectY_hi_uint = ( uint8 )( 0.5 + vfpmpy( numeratorY_hi, denominator_hi ) );

		//16 src points
		v_srcX = ( uint16 )*( ushort16* )( p_SrcX ); p_SrcX += 16;
		v_srcY = ( uint16 )*( ushort16* )( p_SrcY ); p_SrcY += 16;

		//project error
		projectErrorX_lo = vabssub( vunpack_lo( v_srcX ), v_projectX_lo_uint );
		projectErrorX_hi = vabssub( vunpack_hi( v_srcX ), v_projectX_hi_uint );

		projectErrorY_lo = vabssub( vunpack_lo( v_srcY ), v_projectY_lo_uint );
		projectErrorY_hi = vabssub( vunpack_hi( v_srcY ), v_projectY_hi_uint );


		if( type == 0 )
		{
			//valid match points
			uint16 mask_char = ( uint16 ) *( uchar16* )( pMasks ); pMasks += 16;

			//invalid match points,set invalid match points' to 2 * ERR_TH_VALID_H
			uint16 mask_char_not = 1 - mask_char;
			mask_char_not = mask_char_not * 2 * ERR_TH_VALID_H;

			uint8 mask_char_lo = ( uint8 )vadd( vunpack_lo( mask_char ), vunpack_lo( mask_char_not ) );
			uint8 mask_char_hi = ( uint8 )vadd( vunpack_hi( mask_char ), vunpack_hi( mask_char_not ) );

			projectErrorX_lo = ( uint8 )vmpy( lsb, projectErrorX_lo, mask_char_lo );
			projectErrorX_hi = ( uint8 )vmpy( lsb, projectErrorX_hi, mask_char_hi );

			projectErrorY_lo = ( uint8 )vmpy( lsb, projectErrorY_lo, mask_char_lo );
			projectErrorY_hi = ( uint8 )vmpy( lsb, projectErrorY_hi, mask_char_hi );

			//error less than ERR_TH_VALID_H
			RK_U8 num1_X = vabscmp( le, ( int8 )projectErrorX_lo,( int8 )( ERR_TH_VALID_H ) );
			RK_U8 num1_Y = vabscmp( le, ( int8 )projectErrorY_lo,( int8 )( ERR_TH_VALID_H ) );
			RK_U8 num2_X = vabscmp( le, ( int8 )projectErrorX_hi,( int8 )( ERR_TH_VALID_H ) );
			RK_U8 num2_Y = vabscmp( le, ( int8 )projectErrorY_hi,( int8 )( ERR_TH_VALID_H ) );

			RK_U8 num1 = num1_X & num1_Y;
			RK_U8 num2 = num2_X & num2_Y;

			error = cntbits( num1 ) + cntbits( num2 );

		}
		else
		{
			//valid match points
			uint16 mask_char = ( uint16 ) *( uchar16* )( pMasks ); pMasks += 16;

			uint8 mask_char_lo = vunpack_lo( mask_char );
			uint8 mask_char_hi = vunpack_hi( mask_char );

			//invalid match points,set invalid match points' to 0
			projectErrorX_lo = ( uint8 )vmpy( lsb, mask_char_lo, projectErrorX_lo );
			projectErrorX_hi = ( uint8 )vmpy( lsb, mask_char_hi, projectErrorX_hi );

			projectErrorY_lo = ( uint8 )vmpy( lsb, mask_char_lo, projectErrorY_lo );
			projectErrorY_hi = ( uint8 )vmpy( lsb, mask_char_hi, projectErrorY_hi );

			RK_S32 curSumError1 = vintrasum( projectErrorX_lo, vprMask1 ) + vintrasum( projectErrorY_lo, vprMask1 );
			RK_S32 curSumError2 = vintrasum( projectErrorX_hi, vprMask2 ) + vintrasum( projectErrorY_hi, vprMask2 );

			//overflow, Max Project error: (64=6bit) + (16x16Division=8bit) + (RowCol=1bit) < 16bit
			curSumError1 = MIN( curSumError1 + curSumError2, 0x3F );

			error += curSumError1;
		}

	}
}


