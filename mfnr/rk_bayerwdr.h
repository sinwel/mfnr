//
//////////////////////////////////////////////////////////////////////////
// File: rk_bayerwdr.h
// Desc: BayerWDR
// 
// Date: Revised by yousf 20160824
//
//////////////////////////////////////////////////////////////////////////
// 
/***************************************************************************
**  
**  rk_wdr.h
**  
**
**  NOTE: NULL
**   
**  Author: zxy
**  Contact: zxy@rock-chips.com
**  2016/08/12 9:12:44 version 1.0
**  
**	version 1.0 	have not MERGE to branch.
**   
**
** Copyright 2016, rockchip.
**
***************************************************************************/

#pragma once
#ifndef _RK_BAYER_WDR_H
#define _RK_BAYER_WDR_H


//////////////////////////////////////////////////////////////////////////
////-------- Header files
// 
#include "rk_typedef.h"                 // Type definition
#include "rk_global.h"                  // Global definition


#include <assert.h>
#include <vec-c.h>
#include "XM4_defines.h" 
#ifdef __XM4__
#include "profiler.h"
#include <asm-dsp.h>
#endif

#define SHIFT_BIT		 	8 // 4 , 5 , 6
#define SHIFT_BIT_SCALE 	(SHIFT_BIT - 3)
#define MAX_BIT_VALUE  		(1<<SHIFT_BIT) 
#define MAX_BIT_V_MINUS1 	((1<<SHIFT_BIT) - 1)
#define SPLIT_SIZE 			MAX_BIT_VALUE


#define     WDR_SIMU_DEBUG                      0 // debug the wdr only.
#define     WDR_WEIGHT_TRANSPOSE                1 // SET wdr from thumb be 256lines x 16 ( 9 valid data).

#define     WDR_THUMB_TABLE_TR                  0



#if 1
	#define     WDR_VECC                           1
#else
	#define     WDR_VECC                           0
#endif
#ifdef  WIN32
#define WDR_C_MODEL		    1
#else
#define WDR_C_MODEL		    0
#endif



#define max_(a,b) ((a) > (b) ? (a) : (b))
#define min_(a,b) ((a) < (b) ? (a) : (b))

#define WDR_WEIGHT_STRIDE   256  
#if 0//def WIN32
#define PRINT_C_GROUP(namestr,var,start_pos,num,fp,...) \
    {\
        if (fp) \
        fprintf(fp,"[%8s]: ",namestr);\
        else \
        fprintf(stderr,"[%8s]: ",namestr); \
        for (int elem = start_pos; elem < (start_pos+num); elem++) \
        { \
            if (fp) \
                fprintf(fp,"0x%04x ",var[elem]);\
            else \
                fprintf(stderr,"0x%04x ",var[elem]); \
        } \
        if (fp) \
        fprintf(fp,"\n");\
        else \
        fprintf(stderr,"\n"); \
    }

#define PRINT_CEVA_VRF(namestr,vReg,fp,...) \
    {\
        if (fp) \
        fprintf(fp,"[%8s]: ",namestr);\
        else \
        fprintf(stderr,"[%8s]: ",namestr); \
        for (int elem = 0; elem < vReg.num_of_elements; elem++) \
        { \
            if (fp) \
                fprintf(fp,"0x%04x ",vReg[elem]);\
            else \
                fprintf(stderr,"0x%04x ",vReg[elem]); \
        } \
        if (fp) \
        fprintf(fp,"\n");\
        else \
        fprintf(stderr,"\n"); \
    }
#else
    #define PRINT_C_GROUP(namestr,var,start_pos,num,fp,...)
    #define PRINT_CEVA_VRF(namestr,vReg,fp,...)  

#endif

extern RK_U16      g_BaseThumbBuf[409600];                         // Thumb data pointers

void writeFile(RK_U16 *data, int Num, char* FileName);


void wdr_cevaxm4_vecc(unsigned short *pixel_in, 
					unsigned short *pixel_out, 
					int w, 
					int h, 
					float max_scale, 
					RK_U32* pGainMat, 
					RK_F32 testParams_5);


void cul_wdr_cure2(unsigned short *table, RK_U16 exp_times);
unsigned short vsubsat_c(unsigned short a, unsigned short b);
int check_vector_reg(short16 data1, short16 data2);
int check_vector_unsigned_reg(ushort16 data1, ushort16 data2);

int check_result(RK_U16* data1, RK_U16* data2,int Wid ,int  Hgt, int stride1, int stride2);
int check_short16_vecc_result(RK_U16* data1, short16 data2, int  num);
int check_ushort16_vecc_result(RK_U16* data1, ushort16 data2, int  num);
void wdr_simu_cevaxm4();
void set_char32(uchar32 &data, int offset);
void set_short16(short16 &data, int offset);
void wdr_luma_distributionFromThumb(RK_U16		*pweight_mat_vecc,
												int 		w, 
												int 		h);

void wdrPreFilterBlock( ushort *pBaseRawData, ushort *p_dst, int BlockHeight, int BlockWidth );
void CalcuHist( ushort *p_src, ushort *pcount_mat, uint *pweight_mat, int BlockHeight, int BlockWidth, int statisticWidth, int row );
void HistFilter( ushort *pcount_mat, uint *pweight_mat, int statisticHeight, int statisticWidth );
void normalizeWeight( ushort *pcount_mat, uint *pweight_mat, int statisticHeight, int statisticWidth );

void CalcuHistTranspose( ushort *p_src, ushort *pcount_mat, uint *pweight_mat, int BlockHeight, int BlockWidth, int statisticWidth, int row );
void HistFilterTranspose( ushort *pcount_mat, uint *pweight_mat, int statisticHeight, int statisticWidth );
void normalizeWeightTranspose( ushort *pcount_mat, uint *pweight_mat, int statisticHeight, int statisticWidth );



//wdr vec
void wdrPreFilterBlock_Vec( RK_U16 *p_src,       // <<! [ in ]: src data( boundaries have been copyied )
							RK_U16 *p_dst,       // <<! [ out ]: filtered data
							RK_S32 u32Rows,      // <<! [ in ]: row of src data
							RK_S32 u32Cols,      // <<! [ in ]: col of src data
							RK_S32 srcStride );  // <<! [ in ]: stride of src data
							
void CalcuHist_Vec( RK_U16 *p_src,    // <<![ in ]: data used to calculate hist
					RK_U16 *tmp,      // <<![ in ]: tmp pointer used to store hist( only correct when run in vs )
					RK_U16 *pcount,   // <<![ out ]: 32 * 32 data hist in 0 ~8
					RK_U32 *pweight,  // <<![ out ]: 32 * 32 data accumulate in 0 ~8
					RK_S32 u32Rows,   // <<![ in ]: row of data   
					RK_S32 u32Cols,   // <<![ in ]: col of data  
					RK_S32 srcStride, // <<![ in ]: stride of data 
					RK_S32 sw,        // <<![ in ]: num of 32 in width, sw = ceil( width / 32 ) + 1   
					RK_S32 row );     // <<![ in ]: num of 32 * 522  

void countFilter_Vec( RK_U16 *p_src,      // <<! [ in/out ]: pcount need to filter
					  RK_S32 sh,          // <<! [ in ]: sh of pcount: ( ( mRawHgt + 128 ) >> 8 ) + 1 	
					  RK_S32 sw,          // <<! [ in ]: sh of pcount: ( ( mRawWid + 128 ) >> 8 ) + 1
					  RK_S32 srcStride ); // <<! [ in ]: stride of pcount: 256

void weightFilter_Vec( RK_U32 *p_src,      // <<! [ in/out ]: pweight
					   RK_S32 sh,          // <<! [ in ]: sh of pcount: ( ( mRawHgt + 128 ) >> 8 ) + 1
					   RK_S32 sw,          // <<! [ in ]: sw of pcount: ( ( mRawWgt + 128 ) >> 8 ) + 1
					   RK_S32 srcStride ); // <<! [in ]: src stride: 256

void normalizeWeight_Vec( RK_U16 *pcount,      // <<! [ in/out ]: pcount
						  RK_U32 *pweight,     // <<! [ in ]: pweight
						  RK_S32 sh,           // <<! [ in ]: sh of pcount: ( ( mRawHgt + 128 ) >> 8 ) + 1
						  RK_S32 sw,           // <<! [ in ]: sw of pcount: ( ( mRawWgt + 128 ) >> 8 ) + 1
						  RK_S32 srcStride );  // <<! [in ]: src stride: 256

//////////////////////////////////////////////////////////////////////////
////-------- Function Declaration

// Copy Block Data
int CopyBlockData(RK_U16* pSrc, RK_U16* pDst, int nWid, int nHgt, int nSrcStride, int nDstStride);

// WDR Process Block
//void wdr_process_block(RK_U16* pixel_in, RK_U16* pixel_out, int block_w, int block_h, int bBoolFristCTUline, RK_U16* pGainMat);
void wdr_process_block(
    int     x_base,         // [in] x of block in Raw
    int     y_base,         // [in] y of block in Raw
    int     cols,           // [in] min(64, valid)
    int     rows,           // [in] min(32, valid)
    int     statisticWidth,	// [in] (raw_width+128)/256+1 ceil((4164+128)/256)
    int     stride,	        // [in] buffer stride         66
    int     blockWidth,		// [in] picture stride        64
    RK_U16* pPixel_padding, // [in] input buf             34x66*2B
    RK_U16* weightdata,	    // [in] thumb weight table,   256*16*2B = 8K,  
    RK_U16* scale_table,    // [in] tabale[expouse_times] 961*2B
    RK_U16* pGainMat,       // [out] Gain Matrix          32x64*2B
    RK_U16* pPixel_out,     // [out] WDR result           32x64*2B
    RK_U16* pLeftRight      // [in] 2 * 32x16*2B byte space   2K store 32 line left and right, align 16, actually 9 valid..
						) ;
//////////////////////////////////////////////////////////////////////////

#endif // _RK_BAYER_WDR_H



