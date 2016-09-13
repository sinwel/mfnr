/***************************************************************************
**  
**  wdr_process_block.cpp
**  
** 	Do wdr for block, once store 2*4*16 short data out.
** 	need to set the width and height with corrsponding Stride.
**
**  NOTE: NULL
**   
**  Author: zxy
**  Contact: zxy@rock-chips.com
**  2016/08/18 9:12:44 version 1.0
**  
**	version 1.0 	have not MERGE to branch.
**   
**
**	version 1.0 	do filter and interpolation with block(32x64).
**   				data interface is 34x66 data
**
** Copyright 2016, rockchip.
**
***************************************************************************/
//
/////////////////////////////////////////////////////////////////////////
// File: rk_bayerwdr.c
// Desc: Implementation of BayerWDR
// 
// Date: Revised by yousf 20160824
// 
//////////////////////////////////////////////////////////////////////////
////-------- Header files
//
#include "rk_bayerwdr.h"     // BayerWDR
#define 	COL_4		1


DATA_MFNR_EX unsigned short cure_table[24][961] =
{
#include "wdr_cure_tab.dat"
};//*/

DATA_WDR_INT_B3 const unsigned short tab_16banks[481*32] =
{
	#include "isp3.0_tab_16banks.dat"
};


#if CEVA_XM4_DEBUG == 1

	//Block0
	DATA_WDR_INT_B0 RK_S16	u16dstB0[ 5 * 32 ];//   PRAGMA_DSECT_NO_LOAD("data_int_b0");
	//Block1
	DATA_WDR_INT_B1 RK_S32	u16dstB1[ 5 * 32 ];//   PRAGMA_DSECT_NO_LOAD("data_int_b1");

#endif

//////////////////////////////////////////////////////////////////////////
////-------- Functions Definition
//
/************************************************************************/
// Func: CopyBlockData()
// Desc: Copy Block Data
//   In: pSrc               - [in] Src data pointer
//       nWid               - [in] Src data width
//       nHgt               - [in] Src data height
//       nSrcStride         - [in] Src data stride
//       nDstStride         - [in] Dst data stride
//  Out: pDst               - [in/out] Dst data pointer
// 
// Date: Revised by yousf 20160812
// 
/*************************************************************************/
CODE_MFNR_EX
int CopyBlockData(RK_U16* pSrc, RK_U16* pDst, int nWid, int nHgt, int nSrcStride, int nDstStride)
{
/*#ifndef CEVA_CHIP_CODE_BAYERWDR
    //
    int     ret = 0; // return value

    RK_U8*     p_src = (RK_U8*)pSrc; // temp pointer
    RK_U8*     p_dst = (RK_U8*)pDst; // temp pointer
    for (int i=0; i < nHgt; i++)
    {
        memcpy(p_dst, p_src, nWid*sizeof(RK_U16));
        p_src += nSrcStride;
        p_dst += nDstStride;
    }

    //
    return ret;

#else//*/
    //
    int     ret = 0; // return value
//
    RK_U8*     p_src = (RK_U8*)pSrc; // temp pointer
    RK_U8*     p_dst = (RK_U8*)pDst; // temp pointer
    for (int i=0; i < nHgt; i++)
    {
        memcpy(p_dst, p_src, nWid*sizeof(RK_U16));
        p_src += nSrcStride;
        p_dst += nDstStride;
    }
//*/
    /*for (int i=0; i < nHgt; i++)
	{
		memcpy(pDst+i*nSrcStride/2, pSrc+i*nDstStride/2, nWid*sizeof(RK_U16));
	}//*/

    //
    return ret;


//#endif
} // CopyBlockData()


/************************************************************************/
// Func: wdr_process_block()
// Desc: WDR Process Block
//   In: 
//  Out: 
// 
// Date: Revised by yousf 20160824
// 
/*************************************************************************/
CODE_MFNR_EX
void wdr_process_block(
    int     x_base,         // [in] x of block in Raw
    int     y_base,         // [in] y of block in Raw
    int     cols,           // [in] min(64, valid)
    int     rows,           // [in] min(32, valid)
    int     statisticWidth,	// [in] (raw_width+128)/256+1 ceil((4164+128)/256)
    int     stride,	        // [in] buffer stride         66
    int     blockWidth,		// [in] picture stride        64
    RK_U16* pPixel_padding, // [in] input buf             34x66*2B
    RK_U16* weightdata,	    // [in] thumb weight table,   9x256*2B
    RK_U16* scale_table,    // [in] tabale[expouse_times] 961*2B
    RK_U16* pGainMat,       // [out] Gain Matrix          32x64*2B
    RK_U16* pPixel_out,     // [out] WDR result           32x64*2B
    RK_U16* pLeftRight      // [in] 2 * 32x16*2B byte space   2K store 32 line left and right, align 16, actually 9 valid..
						)         
{
	#ifdef __XM4__
		PROFILER_START(rows, cols);
	#endif	

#if 0//ndef CEVA_CHIP_CODE_BAYERWDR
    // NoProcess...
    //CopyBlockData(pPixel_padding + 2*66 + 1, pixel_out, RAW_BLK_SIZE*RAW_WIN_NUM, RAW_BLK_SIZE, (RAW_BLK_SIZE*RAW_WIN_NUM+2)*2, RAW_BLK_SIZE*RAW_WIN_NUM*2);
    CopyBlockData(pPixel_padding + 2*66 + 1, pPixel_out, 64, 32, 66*2, 64*2);
    //*
    for (int r=0; r < 32; r++)
    {
        for (int c=0; c < 64; c++)
        {
            *(pPixel_out + r * 64 + c) = *(pPixel_out + r * 64 + c)  / 8 ;//(*(pPixel_out + r * 64 + c) - 64) / 8 + 64;
        }
    }
	//*/
#else
		int 				col,row,i,ret = 0,blacklevel=256;
	#if WDR_VECC
		RK_U16 				*pTmpOut_vecc = pPixel_out;
		RK_U16 				DataBlk[32*64];
		RK_U16				*pTmpOut 	= DataBlk;	
		const unsigned short *pScaleTab = tab_16banks;
	#else
		RK_U16				*pTmpOut 	= pPixel_out;
	#endif

	#if WDR_C_MODEL
		RK_U16 		left[32*16],right[32*16];
		RK_U16 		light16[64];
		RK_U16 		lindex16[64],lindex2_16[64];
		RK_U16 		weight1[64];
		RK_U16 		weight2[64];
		RK_U16 		weight_phase0[64],weight[64];
		RK_U16 		light16_bak[64];
		RK_U16 		lindex16_bak[64],lindex2_16_bak[64];
		RK_U16 		weight1_bak[64];
		RK_U16 		weight2_bak[64];
		RK_U16 		weight_phase0_bak[64],weight_bak[64];
		RK_U16 		*pLine1 = pPixel_padding + 1;                      
		RK_U16 		*pLine2 = pPixel_padding + 1 + stride;             
		RK_U16 		*pLine3 = pPixel_padding + 1 + 2*stride;           
		unsigned short bi0Y,bi1Y,bi0X,bi1X;	                         
		RK_U16 		*pTmpIn 	= pPixel_padding + stride + 1 ;      
		int					xCoarse 		= x_base >> 8;                
		int					yCoarse 		= y_base >> 8;	      
		int 				fir_idx			= yCoarse*statisticWidth + xCoarse;       
		int 				fir_idxPlus1	= yCoarse*statisticWidth + xCoarse+1;   
		int 				snd_idx			= yCoarse*statisticWidth + xCoarse+statisticWidth;    
		int 				snd_idxPlus1	= yCoarse*statisticWidth + xCoarse+statisticWidth+1;

	#endif
	#if WDR_VECC
		short16 			fir_offset,snd_offset;
		short				offset_w;
		ushort16 			v0,v1,v2,v3,v4,v5,v6,v7;//,v8,v9,v10,v11,v12,v13,v14,v15;
		ushort16			vLine0_0,vLine0_1,vLine0_2,vLine0_3,vLine0_4;
		ushort16 			vLine1_0,vLine1_1,vLine1_2,vLine1_3,vLine1_4;
		ushort16 			vLine2_0,vLine2_1,vLine2_2,vLine2_3,vLine2_4;
		ushort16 			vLine3_0,vLine3_1,vLine3_2,vLine3_3,vLine3_4;
		short16 			vHoriL0_Col0,vHoriL1_Col0,vHoriL2_Col0,vHoriL3_Col0,vVeri0_Col0,vVeri1_Col0;
		short16 			vHoriL0_Col1,vHoriL1_Col1,vHoriL2_Col1,vHoriL3_Col1,vVeri0_Col1,vVeri1_Col1;
		short16 			vHoriL0_Col2,vHoriL1_Col2,vHoriL2_Col2,vHoriL3_Col2,vVeri0_Col2,vVeri1_Col2;
		short16 			vHoriL0_Col3,vHoriL1_Col3,vHoriL2_Col3,vHoriL3_Col3,vVeri0_Col3,vVeri1_Col3;
		short16 			vLine0_Idx0,vLine0_Idx1,vLine0_Idx2,vLine0_Idx3;
		short16 			vLine1_Idx0,vLine1_Idx1,vLine1_Idx2,vLine1_Idx3;
		char32				s32vL0Idx0,s32vL0Idx1,s32vL0Idx2,s32vL0Idx3;
		char32				s32vL1Idx0,s32vL1Idx1,s32vL1Idx2,s32vL1Idx3;



		
		short16 			vWeight0_0,vWeight0_1,vWeight0_2,vWeight0_3;
		short16 			vWeight1_0,vWeight1_1,vWeight1_2,vWeight1_3;
		short16				vMax0_0,vMax0_1,vMax0_2,vMax0_3,vMin0_0,vMin0_1,vMin0_2,vMin0_3;
		short16				vMax1_0,vMax1_1,vMax1_2,vMax1_3,vMin1_0,vMin1_1,vMin1_2,vMin1_3;


		short16 			vLeft0_0,vLeft0_1,vLeft0_2,vLeft0_3,vRigh0_0,vRigh0_1,vRigh0_2,vRigh0_3;
		short16 			vLeft1_0,vLeft1_1,vLeft1_2,vLeft1_3,vRigh1_0,vRigh1_1,vRigh1_2,vRigh1_3;

		short16 			vLeft0_0Plus1,vLeft0_1Plus1,vLeft0_2Plus1,vLeft0_3Plus1;
		short16				vRigh0_0Plus1,vRigh0_1Plus1,vRigh0_2Plus1,vRigh0_3Plus1;
		short16 			vLeft1_0Plus1,vLeft1_1Plus1,vLeft1_2Plus1,vLeft1_3Plus1;
		short16				vRigh1_0Plus1,vRigh1_1Plus1,vRigh1_2Plus1,vRigh1_3Plus1;


		uchar32 			vuIdx0_0,vuIdx0_1,vuIdx0_2,vuIdx0_3,vuIdx1_0,vuIdx1_1,vuIdx1_2,vuIdx1_3;
		ushort16 			w1L0_0,w1L0_1,w1L0_2,w1L0_3, w2L0_0,w2L0_1,w2L0_2,w2L0_3,w0L0,w1L0,w2L0,w3L0;
		ushort16 			w1L1_0,w1L1_1,w1L1_2,w1L1_3, w2L1_0,w2L1_1,w2L1_2,w2L1_3,w0L1,w1L1,w2L1,w3L1;
		short16 			vDataIn0,vDataIn1,vDataIn2,vDataIn3,vDataIn4,vDataIn5,vDataIn6,vDataIn7;

		short16 			vDataOut0,vDataOut1,vDataOut2,vDataOut3,vDataOut4,vDataOut5,vDataOut6,vDataOut7;
		ushort16			wL0_0_s,wL0_1_s,wL0_2_s,wL0_3_s,wL1_0_s,wL1_1_s,wL1_2_s,wL1_3_s;
		ushort16			vFir,vFir1,vSnd,vSnd1;
		unsigned short		vprRightMask = ((x_base & 255) == 0) ? 0xfffe : 0xffff;
		unsigned short 		uBiY0[2],uBiY1[2];
		unsigned short		vprOutMask0,vprOutMask1,vprOutMask2,vprOutMask3;
		unsigned short 		*p_in_u16 = pPixel_padding;
		unsigned short 		*p_in_u16L0_0,*p_in_u16L0_1,*p_in_u16L0_2,*p_in_u16L0_3,*p_in_u16L0_4;
		unsigned short 		*p_in_u16L1_0,*p_in_u16L1_1,*p_in_u16L1_2,*p_in_u16L1_3,*p_in_u16L1_4;
		unsigned char 		s16Kernel_vecc_3[4] = {1,2,1,0};
		uchar32 			v_coeff_3 = *(uchar32*)s16Kernel_vecc_3;
		unsigned char 		vConfig_C[16] = { 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16};
		uchar32 			vConfig = *(uchar32*)vConfig_C;
		uchar32				biBase,bifactor0 = (uchar32)0,bifactor1 = (uchar32)0,bifactor2 = (uchar32)0,bifactor3 = (uchar32)0;	

		uint16   			blackMpy256 = (uint16)(blacklevel*256);
		unsigned short		left_offset[32],righ_offset[32];

														  
		short ptrChunks[16];                                  
		for ( i = 0 ; i < 16 ; i++ )                          
		{                                                     
			ptrChunks[i] = 256 * i;                           
		}	                                                    
		short16 			ptrChunk = *(short16*)ptrChunks;        

		for ( i = 0 ; i < 32 ; i++ )
		{
			left_offset[i]	= 16*i;
			righ_offset[i]	= 16*i + 32*16;
		}
		
		uBiY1[0] 		= y_base & 255;
		uBiY1[1] 		= (y_base+1) & 255;
		//uBiY1[2] 		= (y_base+2) & 255;
		//uBiY1[3] 		= (y_base+3) & 255;

		uBiY0[0] 		= 256 - uBiY1[0];
		uBiY0[1] 		= 256 - uBiY1[1];
		//uBiY0[2] 		= 256 - uBiY1[2];
		//uBiY0[3] 		= 256 - uBiY1[3];

		if  (cols>48)
		{
			vprOutMask0 = 0xffff;
			vprOutMask1 = 0xffff;
			vprOutMask2 = 0xffff;
			vprOutMask3 = (1<<(cols-48)) - 1;
		}
		else if (cols>32)
		{
			vprOutMask0 = 0xffff;
			vprOutMask1 = 0xffff;
			vprOutMask2 = (1<<(cols-32)) - 1;
			vprOutMask3 = 0;

		}
		else if(cols>16)
		{
			vprOutMask0 = 0xffff;
			vprOutMask1 = (1<<(cols-16)) - 1;;
			vprOutMask2 = 0;
			vprOutMask3 = 0;

		}
		else
		{
			vprOutMask0 = (1<<cols) - 1;
			vprOutMask1 = 0;
			vprOutMask2 = 0;
			vprOutMask3 = 0;
		}


		set_char32(biBase,0);	
		bifactor0 = (uchar32)vselect(vsub(biBase,(uchar32)x_base), 			vadd(biBase,(uchar32)x_base), 0x0000ffff);
		bifactor1 = (uchar32)vselect(vsub(biBase,(uchar32)(x_base + 16)), 	vadd(biBase,(uchar32)(x_base + 16)), 0x0000ffff);
		bifactor2 = (uchar32)vselect(vsub(biBase,(uchar32)(x_base + 32)), 	vadd(biBase,(uchar32)(x_base + 32)), 0x0000ffff);
		bifactor3 = (uchar32)vselect(vsub(biBase,(uchar32)(x_base + 48)), 	vadd(biBase,(uchar32)(x_base + 48)), 0x0000ffff);

	#endif
		// Calcu the y interpolation , Left and Right
		// Do Stage1.
	#if WDR_C_MODEL	

		#if WDR_WEIGHT_TRANSPOSE
		if(1) // (x_base & 255) == 0), calcu for every block.
		{
			for  (int row_ = 0 ; row_ < rows ; row_++ )
			{
				bi1Y 		= (y_base+row_) & 255;
				bi0Y 		= 256 - bi1Y;
		
				for (i = 0; i < 9; i++)
				{
					left [row_*16 + i] = (weightdata[i + fir_idx*16] * bi0Y    	   + weightdata[i + snd_idx*16]  * bi1Y) >> 8;
					right[row_*16 + i] = (weightdata[i + fir_idxPlus1*16] * bi0Y   + weightdata[i + snd_idxPlus1*16] * bi1Y) >> 8;
				}
			}
		}
		#else
		if(1) // (x_base & 255) == 0), calcu for every block.
		{
			for  (int row_ = 0 ; row_ < rows ; row_++ )
			{
				bi1Y 		= (y_base+row_) & 255;
				bi0Y 		= 256 - bi1Y;
		
				for (i = 0; i < 9; i++)
				{
					left [row_*16 + i] = (weightdata[i*256 + fir_idx] * bi0Y    	+ weightdata[i*256 + snd_idx]  * bi1Y) >> 8;
					right[row_*16 + i] = (weightdata[i*256 + fir_idxPlus1] * bi0Y   + weightdata[i*256 + snd_idxPlus1] * bi1Y) >> 8;
				}
			}
		}
		#endif
		
	#endif
	#if WDR_VECC
		if ((x_base & 255) == 0)
		{
 			// 256x256 Large Block have the same weight matrix.
			offset_w = (y_base >> 8)*statisticWidth + (x_base>> 8);// actully x+col,but col is low<256,so bypass.	

		#if WDR_WEIGHT_TRANSPOSE
			vFir 	= *(ushort16*)(weightdata + offset_w*16);
			vFir1 	= *(ushort16*)(weightdata + offset_w*16 + 16);
			vSnd 	= *(ushort16*)(weightdata + offset_w*16 + statisticWidth*16);
			vSnd1 	= *(ushort16*)(weightdata + offset_w*16 + (statisticWidth+ 1)*16 );
			//short o_ff[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
			//fir_offset = *(short16*)o_ff;
			//vFir = vpld(rel,weightdata, fir_offset);// v0,vLine0_0 is first,first_plus1

		#else
			// WDR_VECC code
			fir_offset 		 = vadd(ptrChunk,	(short16)offset_w);
			snd_offset 		 = vadd(fir_offset,	(short16)statisticWidth);		
			vpld((unsigned short*)weightdata, fir_offset, vFir, vFir1);// v0,vLine0_0 is first,first_plus1
			vpld((unsigned short*)weightdata, snd_offset, vSnd, vSnd1);// v1,vLine0_0 is second,second_plus1
		#endif
			// store the 4 vReg to Mem, so do need calcu every block.
           vst(vFir,	(ushort16*)(pLeftRight+0),            	 0xffff);
           vst(vFir1,	(ushort16*)(pLeftRight+16),           	 0xffff);
           vst(vSnd,	(ushort16*)(pLeftRight+2*16),            0xffff);
           vst(vSnd1,	(ushort16*)(pLeftRight+3*16),            0xffff);
		}
		else
		{
			vFir 	= *(ushort16*)(pLeftRight+0)    ; 
			vFir1	= *(ushort16*)(pLeftRight+16)   ; 
			vSnd 	= *(ushort16*)(pLeftRight+2*16) ; 
			vSnd1	= *(ushort16*)(pLeftRight+3*16) ; 
		}
		p_in_u16L0_0 = p_in_u16 + 2*stride;  
		p_in_u16L0_1 = p_in_u16L0_0 + 16; 
		p_in_u16L0_2 = p_in_u16L0_0 + 32; 
		p_in_u16L0_3 = p_in_u16L0_0 + 48; 
		p_in_u16L0_4 = p_in_u16L0_0 + 64; 

		p_in_u16L1_0 = p_in_u16 + 3*stride;  
		p_in_u16L1_1 = p_in_u16L1_0 + 16; 
		p_in_u16L1_2 = p_in_u16L1_0 + 32; 
		p_in_u16L1_3 = p_in_u16L1_0 + 48; 
		p_in_u16L1_4 = p_in_u16L1_0 + 64; 

		
		vLine0_0 = *(ushort16*)p_in_u16;  
		vLine0_1 = *(ushort16*)(p_in_u16+16);     
		vLine0_2 = *(ushort16*)(p_in_u16+32);   
		#if COL_4
		vLine0_3 = *(ushort16*)(p_in_u16+48);     
		vLine0_4 = *(ushort16*)(p_in_u16+64);
		#endif

		p_in_u16 += stride;    
		vLine1_0 = *(ushort16*)p_in_u16;  
		vLine1_1 = *(ushort16*)(p_in_u16+16);     
		vLine1_2 = *(ushort16*)(p_in_u16+32);  
		#if COL_4
		vLine1_3 = *(ushort16*)(p_in_u16+48);     
		vLine1_4 = *(ushort16*)(p_in_u16+64);
		#endif

		vLine2_0 = *(ushort16*)p_in_u16L0_0; 
		vLine2_1 = *(ushort16*)p_in_u16L0_1;    
		vLine2_2 = *(ushort16*)p_in_u16L0_2;  
		#if COL_4
		vLine2_3 = *(ushort16*)p_in_u16L0_3;    
		vLine2_4 = *(ushort16*)p_in_u16L0_4;  
		#endif

		p_in_u16L0_0 += 2*stride;   
		p_in_u16L0_1 += 2*stride;   
		p_in_u16L0_2 += 2*stride; 
		#if COL_4
		p_in_u16L0_3 += 2*stride; 
		p_in_u16L0_4 += 2*stride; 
		#endif


		vLine3_0 = *(ushort16*)p_in_u16L1_0; 
		vLine3_1 = *(ushort16*)p_in_u16L1_1;    
		vLine3_2 = *(ushort16*)p_in_u16L1_2;  
		#if COL_4
		vLine3_3 = *(ushort16*)p_in_u16L1_3;    
		vLine3_4 = *(ushort16*)p_in_u16L1_4;  
		#endif

		p_in_u16L1_0 += 2*stride;   
		p_in_u16L1_1 += 2*stride;   
		p_in_u16L1_2 += 2*stride; 
		#if COL_4
		p_in_u16L1_3 += 2*stride; 
		p_in_u16L1_4 += 2*stride; 
		#endif

	
		vHoriL0_Col0 = vswmac5( psl, vLine0_0,  vLine0_1,  v_coeff_3, SW_CONFIG(0,0,0,0,0,0), (uint16)0);// 10 + 3(gain) + 2		PRINT_CEVA_VRF("vHori0",vHori0,stderr);
		vHoriL1_Col0 = vswmac5( psl, vLine1_0,  vLine1_1,  v_coeff_3, SW_CONFIG(0,0,0,0,0,0), (uint16)0);
		
		vHoriL0_Col1 = vswmac5( psl, vLine0_1,  vLine0_2,  v_coeff_3, SW_CONFIG(0,0,0,0,0,0), (uint16)0);// 10 + 3(gain) + 2		PRINT_CEVA_VRF("vHori0",vHori0,stderr);
		vHoriL1_Col1 = vswmac5( psl, vLine1_1,  vLine1_2,  v_coeff_3, SW_CONFIG(0,0,0,0,0,0), (uint16)0);

		#if COL_4
		vHoriL0_Col2 = vswmac5( psl, vLine0_2,  vLine0_3,  v_coeff_3, SW_CONFIG(0,0,0,0,0,0), (uint16)0);// 10 + 3(gain) + 2		PRINT_CEVA_VRF("vHori0",vHori0,stderr);
		vHoriL1_Col2 = vswmac5( psl, vLine1_2,  vLine1_3,  v_coeff_3, SW_CONFIG(0,0,0,0,0,0), (uint16)0);
		#endif

		#if COL_4
		vHoriL0_Col3 = vswmac5( psl, vLine0_3,  vLine0_4,  v_coeff_3, SW_CONFIG(0,0,0,0,0,0), (uint16)0);// 10 + 3(gain) + 2		PRINT_CEVA_VRF("vHori0",vHori0,stderr);
		vHoriL1_Col3 = vswmac5( psl, vLine1_3,  vLine1_4,  v_coeff_3, SW_CONFIG(0,0,0,0,0,0), (uint16)0);
		#endif

	
	#endif	



		// Do Stage2.
		for  ( row = 0 ; row < rows ; row+=2 )
		{
		#if WDR_C_MODEL	
			int sumlight;
			for (int j = 0 ; j < 2; j++ )
			{
				if(j==0)
				{
					for (col = 0; col < cols; col++)
					{
						
						//--------------------------
						// 3x3 filter,0.3cycle
						//--------------------------
						sumlight =	pLine1[col- 1] + 2 * pLine1[col] + pLine1[col + 1] + 
									2 * pLine2[col  - 1] + 4 * pLine2[ col ] + 2 * pLine2[col + 1] +
									pLine3[col- 1] + 2 * pLine3[col] + pLine3[col + 1];
						
						light16[col]  = sumlight >> 7; // 10 valid.

						assert(light16[col]<1024);
						
						lindex16[col] = light16[col] >> 7;// HIGH 3bit for lineIdx, low 7 bit for interpolation factor. SPLIT INSTRUCTION
						
						assert(lindex16[col]<8);

					#if 1//!FAST_BY_SKIP_LIGHT_INTERPOLATION
						weight1[col] = (left[row*16+lindex16[col]]     * (128 - (light16[col] & 127))   
									+ right[row*16+lindex16[col]]     * (light16[col] & 127)) / 128;
					
						weight2[col] = (left[row*16+lindex16[col] + 1] * (128 - (light16[col] & 127)) 
									+ right[row*16+lindex16[col] + 1] * (light16[col] & 127)) / 128;

						bi1X = (x_base+col) & 255;
						bi0X = 256 - bi1X;

						weight_phase0[col]  = (weight1[col]*bi0X + weight2[col]*bi1X) / 256;

					#else
						weight1[col] = (left[row*16+lindex16[col]] + right[row*16+lindex16[col]]  ) ;
					
						weight2[col] = (left[row*16+lindex16[col] + 1] + right[row*16+lindex16[col] + 1] ) ;

						bi1X = (x_base+col) & 255;
						bi0X = 256 - bi1X;

						weight_phase0[col]  = (weight1[col]*bi0X + weight2[col]*bi1X) / 512;
					#endif


						//weight = vclip_c(weight, max_((light-512),blacklevel*4), (light+512));
						//weight = weight-blacklevel*4;
						//lindex2_16[col] = max_( min_(  max_((weight_phase0[col]>>4)-64, 0), max_((light16[col]+32-64),0))  , max_(light16[col]-32-64,0));
						lindex2_16[col] = max_( min_(  max_((weight_phase0[col]>>4), 0), max_((light16[col]+32),0))  , max_(light16[col]-32,0));
						lindex2_16[col] = max_(lindex2_16[col]-64,0);
						assert(lindex2_16[col] < 961);
		
						//--------------------------
						// LUT
						//--------------------------
						weight[col] = (scale_table[lindex2_16[col]] * (16 - (weight_phase0[col] & 15)) +
									   scale_table[lindex2_16[col] + 1] * (weight_phase0[col] & 15) + 8) >> 4;

						//--------------------------
						// STORE OUT 
						//--------------------------
						//*(pGainMat + row*blockWidth + col) = (RK_U32)weight[col]; // for zlf-SpaceDenoise
						*(pTmpOut+col) = min_(max_(*(pTmpIn+col)- 2*blacklevel ,0)*weight[col] / 1024 + blacklevel/4 , 1023);
					}
				}
				else
				{
					for (col = 0; col < cols; col++)
					{
						
						//--------------------------
						// 3x3 filter,0.3cycle
						//--------------------------
						sumlight =	pLine1[col- 1] + 2 * pLine1[col] + pLine1[col + 1] + 
									2 * pLine2[col  - 1] + 4 * pLine2[ col ] + 2 * pLine2[col + 1] +
									pLine3[col- 1] + 2 * pLine3[col] + pLine3[col + 1];
						
						light16_bak[col]  = sumlight >> 7; // 10 valid.
						
						assert(light16_bak[col]<1024);
	
						lindex16_bak[col] = light16_bak[col] >> 7;// HIGH 3bit for lineIdx, low 7 bit for interpolation factor. SPLIT INSTRUCTION
						
						assert(lindex16_bak[col]<8);
					#if 1//!FAST_BY_SKIP_LIGHT_INTERPOLATION
						weight1_bak[col] = (left[(row+1)*16 + lindex16_bak[col]]     * (128 - (light16_bak[col] & 127))   
									+ right[(row+1)*16 + lindex16_bak[col]]     * (light16_bak[col] & 127)) / 128;
					
						weight2_bak[col] = (left[(row+1)*16 + lindex16_bak[col] + 1] * (128 - (light16_bak[col] & 127)) 
									+ right[(row+1)*16 + lindex16_bak[col] + 1] * (light16_bak[col] & 127)) / 128;

						bi1X = (x_base+col) & 255;
						bi0X = 256 - bi1X;

						weight_phase0_bak[col]  = (weight1_bak[col]*bi0X + weight2_bak[col]*bi1X) / 256;

					#else

						weight1_bak[col] = (left[(row+1)*16 + lindex16_bak[col]]     
									+ right[(row+1)*16 + lindex16_bak[col]]    );
					
						weight2_bak[col] = (left[(row+1)*16 + lindex16_bak[col] + 1] 
									+ right[(row+1)*16 + lindex16_bak[col] + 1]  );

						bi1X = (x_base+col) & 255;
						bi0X = 256 - bi1X;

						weight_phase0_bak[col]  = (weight1_bak[col]*bi0X + weight2_bak[col]*bi1X) / 512;
					#endif


						//weight = vclip_c(weight, max_((light-512),blacklevel*4), (light+512));
						//weight = weight-blacklevel*4;
						// split weight_phase0, high 10 bit for compare to get lineIdx, low 4 bit for bilinear.
						//lindex2_16_bak[col] = max_( min_(  max_((weight_phase0_bak[col]>>4)-64, 0), max_((light16_bak[col]+32-64),0))  , max_(light16_bak[col]-32-64,0));
						lindex2_16_bak[col] = max_( min_(  max_((weight_phase0_bak[col]>>4), 0), max_((light16_bak[col]+32),0))  , max_(light16_bak[col]-32,0));
						lindex2_16_bak[col] = max_(lindex2_16_bak[col]-64,0);

						assert(lindex2_16_bak[col] < 961);
							//--------------------------
						// LUT
						//--------------------------
						weight_bak[col] = (scale_table[lindex2_16_bak[col]] * (16 - (weight_phase0_bak[col] & 15)) +
									   scale_table[lindex2_16_bak[col] + 1] * (weight_phase0_bak[col] & 15) + 8) >> 4;

						//--------------------------
						// STORE OUT 
						//--------------------------
						//*(pGainMat + row*blockWidth + col) = (RK_U32)weight[col]; // for zlf-SpaceDenoise
						*(pTmpOut+col) =  min_(max_(*(pTmpIn+col)- 2*blacklevel ,0)*weight_bak[col] / 1024 + blacklevel/4,1023);
					}
				}
				pLine1 += stride;
				pLine2 += stride;
				pLine3 += stride;
				pTmpOut  += blockWidth;
				pTmpIn	 += stride;// share the buff with padding.
			}

		#endif


		// calcu the uBiYx
		uBiY1[0] 		= (y_base+row) & 255;
		uBiY1[1] 		= (y_base+row+1) & 255;
		uBiY0[0] 		= 256 - uBiY1[0];
		uBiY0[1] 		= 256 - uBiY1[1];

		// calcu the left and right @ y-axis for every row.
		v0 = vmac3(psl, vFir,  uBiY0[0], vSnd,  uBiY1[0], (uint16) 0, (unsigned char)8); 
		v1 = vmac3(psl, vFir1, uBiY0[0], vSnd1, uBiY1[0], (uint16) 0, (unsigned char)8);
		v2 = vmac3(psl, vFir,  uBiY0[1], vSnd,  uBiY1[1], (uint16) 0, (unsigned char)8); 
		v3 = vmac3(psl, vFir1, uBiY0[1], vSnd1, uBiY1[1], (uint16) 0, (unsigned char)8);
	

		#if WDR_VECC	
			//--------------------------
			// 3x3 filter,0.3cycle , 26 command
			//--------------------------
			// 0,1,2 line

			vDataIn0	= vperm(vLine1_0,vLine1_1,vConfig);
			vDataIn1	= vperm(vLine1_1,vLine1_2,vConfig);
			#if COL_4
			vDataIn2	= vperm(vLine1_2,vLine1_3,vConfig);
			vDataIn3	= vperm(vLine1_3,vLine1_4,vConfig);		
			#endif

			vDataIn4	= vperm(vLine2_0,vLine2_1,vConfig);
			vDataIn5	= vperm(vLine2_1,vLine2_2,vConfig);
			#if COL_4
			vDataIn6	= vperm(vLine2_2,vLine2_3,vConfig);
			vDataIn7	= vperm(vLine2_3,vLine2_4,vConfig);		
			#endif

			vHoriL2_Col0= vswmac5( psl, vLine2_0, vLine2_1, v_coeff_3, SW_CONFIG(0,0,0,0,0,0), (uint16)0);
			vHoriL2_Col1= vswmac5( psl, vLine2_1, vLine2_2, v_coeff_3, SW_CONFIG(0,0,0,0,0,0), (uint16)0);

			#if COL_4
			vHoriL2_Col2= vswmac5( psl, vLine2_2, vLine2_3, v_coeff_3, SW_CONFIG(0,0,0,0,0,0), (uint16)0);
			vHoriL2_Col3= vswmac5( psl, vLine2_3, vLine2_4, v_coeff_3, SW_CONFIG(0,0,0,0,0,0), (uint16)0);
			#endif

			vHoriL3_Col0= vswmac5( psl, vLine3_0, vLine3_1, v_coeff_3, SW_CONFIG(0,0,0,0,0,0), (uint16)0);
			vHoriL3_Col1= vswmac5( psl, vLine3_1, vLine3_2, v_coeff_3, SW_CONFIG(0,0,0,0,0,0), (uint16)0);

			#if COL_4
			vHoriL3_Col2= vswmac5( psl, vLine3_2, vLine3_3, v_coeff_3, SW_CONFIG(0,0,0,0,0,0), (uint16)0);
			vHoriL3_Col3= vswmac5( psl, vLine3_3, vLine3_4, v_coeff_3, SW_CONFIG(0,0,0,0,0,0), (uint16)0);
			#endif


			vVeri0_Col0 = vmac3(psl, vHoriL1_Col0, (unsigned char)2, vHoriL2_Col0, (unsigned char)1,  (uint16)vHoriL0_Col0, (unsigned char)7); 
			vVeri0_Col1	= vmac3(psl, vHoriL1_Col1, (unsigned char)2, vHoriL2_Col1, (unsigned char)1,  (uint16)vHoriL0_Col1, (unsigned char)7); 

			#if COL_4
			vVeri0_Col2 = vmac3(psl, vHoriL1_Col2, (unsigned char)2, vHoriL2_Col2, (unsigned char)1,  (uint16)vHoriL0_Col2, (unsigned char)7); 
			vVeri0_Col3	= vmac3(psl, vHoriL1_Col3, (unsigned char)2, vHoriL2_Col3, (unsigned char)1,  (uint16)vHoriL0_Col3, (unsigned char)7); 
			#endif
			
			// 1,2,3 line 
			vVeri1_Col0 = vmac3(psl, vHoriL2_Col0, (unsigned char)2, vHoriL3_Col0, (unsigned char)1,  (uint16)vHoriL1_Col0, (unsigned char)7); 
			vVeri1_Col1	= vmac3(psl, vHoriL2_Col1, (unsigned char)2, vHoriL3_Col1, (unsigned char)1,  (uint16)vHoriL1_Col1, (unsigned char)7); 

			#if COL_4
			vVeri1_Col2 = vmac3(psl, vHoriL2_Col2, (unsigned char)2, vHoriL3_Col2, (unsigned char)1,  (uint16)vHoriL1_Col2, (unsigned char)7); 
			vVeri1_Col3	= vmac3(psl, vHoriL2_Col3, (unsigned char)2, vHoriL3_Col3, (unsigned char)1,  (uint16)vHoriL1_Col3, (unsigned char)7); 
			#endif

			vHoriL0_Col0 = vHoriL2_Col0;// 0 <- 2
			vHoriL1_Col0 = vHoriL3_Col0;// 1 <- 3
			vHoriL0_Col1 = vHoriL2_Col1;// 0 <- 2
			vHoriL1_Col1 = vHoriL3_Col1;// 1 <- 3

			#if COL_4
			vHoriL0_Col2 = vHoriL2_Col2;// 0 <- 2
			vHoriL1_Col2 = vHoriL3_Col2;// 1 <- 3
			vHoriL0_Col3 = vHoriL2_Col3;// 0 <- 2
			vHoriL1_Col3 = vHoriL3_Col3;// 1 <- 3
			#endif
			
			// split the light to index and factor [line 0,1]
			vsplit(vVeri0_Col0, (unsigned char)7, vLine0_Idx0, vWeight0_0);
			vsplit(vVeri0_Col1, (unsigned char)7, vLine0_Idx1, vWeight0_1);
			#if COL_4
			vsplit(vVeri0_Col2, (unsigned char)7, vLine0_Idx2, vWeight0_2);
			vsplit(vVeri0_Col3, (unsigned char)7, vLine0_Idx3, vWeight0_3);
			#endif
			vsplit(vVeri1_Col0, (unsigned char)7, vLine1_Idx0, vWeight1_0);
			vsplit(vVeri1_Col1, (unsigned char)7, vLine1_Idx1, vWeight1_1);
			#if COL_4
			vsplit(vVeri1_Col2, (unsigned char)7, vLine1_Idx2, vWeight1_2);
			vsplit(vVeri1_Col3, (unsigned char)7, vLine1_Idx3, vWeight1_3);
			#endif


			// << -------------------------------------------------------
 			
 			#if (WDR_C_MODEL && WDR_VECC)
			if(64 == cols)
			{
				ret += check_short16_vecc_result(light16,     vVeri0_Col0, 16);
				ret += check_short16_vecc_result(light16+16,  vVeri0_Col1, 16);
				ret += check_short16_vecc_result(light16+32,  vVeri0_Col2, 16);
				ret += check_short16_vecc_result(light16+48,  vVeri0_Col3, 16);

				ret += check_short16_vecc_result(lindex16,     vLine0_Idx0, 16);
				ret += check_short16_vecc_result(lindex16+16,  vLine0_Idx1, 16);
				ret += check_short16_vecc_result(lindex16+32,  vLine0_Idx2, 16);
				ret += check_short16_vecc_result(lindex16+48,  vLine0_Idx3, 16);
				assert(ret == 0);
			}
			#endif
			//--------------------------
			// bilinear @ light 36 command
			//--------------------------

			vuIdx0_0 = (uchar32)vcast(vsub((short)128,vWeight0_0),vWeight0_0);// lindex[0  -15]
			vuIdx0_1 = (uchar32)vcast(vsub((short)128,vWeight0_1),vWeight0_1);// lindex[16- 31]
			#if COL_4
			vuIdx0_2 = (uchar32)vcast(vsub((short)128,vWeight0_2),vWeight0_2);// lindex[0  -15]
			vuIdx0_3 = (uchar32)vcast(vsub((short)128,vWeight0_3),vWeight0_3);// lindex[16- 31]
			#endif
			
			vuIdx1_0 = (uchar32)vcast(vsub((short)128,vWeight1_0),vWeight1_0);// lindex[0  -15]      
			vuIdx1_1 = (uchar32)vcast(vsub((short)128,vWeight1_1),vWeight1_1);// lindex[16- 31]      
			#if COL_4
			vuIdx1_2 = (uchar32)vcast(vsub((short)128,vWeight1_2),vWeight1_2);// lindex[0  -15]      
			vuIdx1_3 = (uchar32)vcast(vsub((short)128,vWeight1_3),vWeight1_3);// lindex[16- 31]      
			#endif

		#if 0
			vpld((short*)pLeftRight,vadd(vLine0_Idx0,left_offset[row]),	vLeft0_0 , vLeft0_0Plus1);      
			vpld((short*)pLeftRight,vadd(vLine0_Idx0,righ_offset[row]),vRigh0_0 , vRigh0_0Plus1);      
			vpld((short*)pLeftRight,vadd(vLine0_Idx1,left_offset[row]),	vLeft0_1 , vLeft0_1Plus1);      
			vpld((short*)pLeftRight,vadd(vLine0_Idx1,righ_offset[row]),vRigh0_1 , vRigh0_1Plus1);      

			vpld((short*)pLeftRight,vadd(vLine1_Idx0,left_offset[row+1]),vLeft1_0, vLeft1_0Plus1 );      
			vpld((short*)pLeftRight,vadd(vLine1_Idx0,righ_offset[row+1]),vRigh1_0,vRigh1_0Plus1 );      
			vpld((short*)pLeftRight,vadd(vLine1_Idx1,left_offset[row+1]),vLeft1_1, vLeft1_1Plus1 );      
			vpld((short*)pLeftRight,vadd(vLine1_Idx1,righ_offset[row+1]),vRigh1_1,vRigh1_1Plus1 );      

			#if COL_4
			vpld((short*)pLeftRight,vadd(vLine0_Idx2,left_offset[row]),	vLeft0_2 , vLeft0_2Plus1);      
			vpld((short*)pLeftRight,vadd(vLine0_Idx2,righ_offset[row]),vRigh0_2 , vRigh0_2Plus1);      
			vpld((short*)pLeftRight,vadd(vLine0_Idx3,left_offset[row]),	vLeft0_3 , vLeft0_3Plus1);      
			vpld((short*)pLeftRight,vadd(vLine0_Idx3,righ_offset[row]),vRigh0_3 , vRigh0_3Plus1);      

			vpld((short*)pLeftRight,vadd(vLine1_Idx2,left_offset[row+1]),vLeft1_2, vLeft1_2Plus1 );      
			vpld((short*)pLeftRight,vadd(vLine1_Idx2,righ_offset[row+1]),vRigh1_2,vRigh1_2Plus1 );      
			vpld((short*)pLeftRight,vadd(vLine1_Idx3,left_offset[row+1]),vLeft1_3, vLeft1_3Plus1 );      
			vpld((short*)pLeftRight,vadd(vLine1_Idx3,righ_offset[row+1]),vRigh1_3,vRigh1_3Plus1 );      
			#endif
		#else
			s32vL0Idx0 		= vcast(sat, vLine0_Idx0, vLine0_Idx0);
			s32vL0Idx1 		= vcast(sat, vLine0_Idx1, vLine0_Idx1);
			s32vL0Idx2 		= vcast(sat, vLine0_Idx2, vLine0_Idx2);
			s32vL0Idx3 		= vcast(sat, vLine0_Idx3, vLine0_Idx3);
			
			s32vL1Idx0 		= vcast(sat, vLine1_Idx0, vLine1_Idx0);
			s32vL1Idx1 		= vcast(sat, vLine1_Idx1, vLine1_Idx1);
			s32vL1Idx2 		= vcast(sat, vLine1_Idx2, vLine1_Idx2);
			s32vL1Idx3 		= vcast(sat, vLine1_Idx3, vLine1_Idx3);

			vLeft0_0 		= vlut(v0,v0,s32vL0Idx0,vLeft0_0) ;
			vLeft0_0Plus1 	= vlut(v0,v0,vadd(s32vL0Idx0,(char)1),vLeft0_0Plus1) ;
			vLeft0_1 		= vlut(v0,v0,s32vL0Idx1,vLeft0_1) ;
			vLeft0_1Plus1 	= vlut(v0,v0,vadd(s32vL0Idx1,(char)1),vLeft0_1Plus1) ;
			vLeft0_2 		= vlut(v0,v0,s32vL0Idx2,vLeft0_2) ;
			vLeft0_2Plus1 	= vlut(v0,v0,vadd(s32vL0Idx2,(char)1),vLeft0_2Plus1) ;
			vLeft0_3 		= vlut(v0,v0,s32vL0Idx3,vLeft0_3) ;
			vLeft0_3Plus1 	= vlut(v0,v0,vadd(s32vL0Idx3,(char)1),vLeft0_3Plus1) ;

			vRigh0_0 		= vlut(v1,v1,s32vL0Idx0,vRigh0_0) ;
			vRigh0_0Plus1 	= vlut(v1,v1,vadd(s32vL0Idx0,(char)1),vRigh0_0Plus1) ;
			vRigh0_1 		= vlut(v1,v1,s32vL0Idx1,vRigh0_1) ;
			vRigh0_1Plus1 	= vlut(v1,v1,vadd(s32vL0Idx1,(char)1),vRigh0_1Plus1) ;
			vRigh0_2 		= vlut(v1,v1,s32vL0Idx2,vRigh0_2) ;
			vRigh0_2Plus1 	= vlut(v1,v1,vadd(s32vL0Idx2,(char)1),vRigh0_2Plus1) ;
			vRigh0_3 		= vlut(v1,v1,s32vL0Idx3,vRigh0_3) ;
			vRigh0_3Plus1 	= vlut(v1,v1,vadd(s32vL0Idx3,(char)1),vRigh0_3Plus1) ;

			vLeft1_0 		= vlut(v2,v2,s32vL1Idx0,vLeft1_0) ;
			vLeft1_0Plus1 	= vlut(v2,v2,vadd(s32vL1Idx0,(char)1),vLeft1_0Plus1) ;
			vLeft1_1 		= vlut(v2,v2,s32vL1Idx1,vLeft1_1) ;           
			vLeft1_1Plus1 	= vlut(v2,v2,vadd(s32vL1Idx1,(char)1),vLeft1_1Plus1) ;
			vLeft1_2 		= vlut(v2,v2,s32vL1Idx2,vLeft1_2) ;             
			vLeft1_2Plus1 	= vlut(v2,v2,vadd(s32vL1Idx2,(char)1),vLeft1_2Plus1) ;
			vLeft1_3 		= vlut(v2,v2,s32vL1Idx3,vLeft1_3) ;             
			vLeft1_3Plus1 	= vlut(v2,v2,vadd(s32vL1Idx3,(char)1),vLeft1_3Plus1) ;

			vRigh1_0 		= vlut(v3,v3,s32vL1Idx0,vRigh1_0) ;
			vRigh1_0Plus1 	= vlut(v3,v3,vadd(s32vL1Idx0,(char)1),vRigh1_0Plus1) ;
			vRigh1_1 		= vlut(v3,v3,s32vL1Idx1,vRigh1_1) ;
			vRigh1_1Plus1 	= vlut(v3,v3,vadd(s32vL1Idx1,(char)1),vRigh1_1Plus1) ;
			vRigh1_2 		= vlut(v3,v3,s32vL1Idx2,vRigh1_2) ;
			vRigh1_2Plus1 	= vlut(v3,v3,vadd(s32vL1Idx2,(char)1),vRigh1_2Plus1) ;
			vRigh1_3 		= vlut(v3,v3,s32vL1Idx3,vRigh1_3) ;
			vRigh1_3Plus1 	= vlut(v3,v3,vadd(s32vL1Idx3,(char)1),vRigh1_3Plus1) ;




		#endif
			w1L0_0 = (ushort16)vmac3(splitsrc, psl, vLeft0_0, vRigh0_0, vuIdx0_0, (uint16) 0, (unsigned char)7); 
			w1L0_1 = (ushort16)vmac3(splitsrc, psl, vLeft0_1, vRigh0_1, vuIdx0_1, (uint16) 0, (unsigned char)7);
			#if COL_4
			w1L0_2 = (ushort16)vmac3(splitsrc, psl, vLeft0_2, vRigh0_2, vuIdx0_2, (uint16) 0, (unsigned char)7); 
			w1L0_3 = (ushort16)vmac3(splitsrc, psl, vLeft0_3, vRigh0_3, vuIdx0_3, (uint16) 0, (unsigned char)7);
			#endif
			
			w1L1_0 = (ushort16)vmac3(splitsrc, psl, vLeft1_0, vRigh1_0, vuIdx1_0, (uint16) 0, (unsigned char)7);                                                                                                                                                                                                                                                                                                               
			w1L1_1 = (ushort16)vmac3(splitsrc, psl, vLeft1_1, vRigh1_1, vuIdx1_1, (uint16) 0, (unsigned char)7);                                                                                                                                                                                                                                                                                                               
			#if COL_4
			w1L1_2 = (ushort16)vmac3(splitsrc, psl, vLeft1_2, vRigh1_2, vuIdx1_2, (uint16) 0, (unsigned char)7);                                                                                                                                                                                                                                                                                                               
			w1L1_3 = (ushort16)vmac3(splitsrc, psl, vLeft1_3, vRigh1_3, vuIdx1_3, (uint16) 0, (unsigned char)7);                                                                                                                                                                                                                                                                                                               
			#endif

		
			#if (WDR_C_MODEL && WDR_VECC)
			if(64 == cols)
			{
				ret += check_ushort16_vecc_result(weight1,     w1L0_0, 16);
				ret += check_ushort16_vecc_result(weight1+16,  w1L0_1, 16);

				assert(ret==0);
			}
			#endif


			w2L0_0 = (ushort16)vmac3(splitsrc, psl, vLeft0_0Plus1, vRigh0_0Plus1, vuIdx0_0, (uint16) 0, (unsigned char)7); 
			w2L0_1 = (ushort16)vmac3(splitsrc, psl, vLeft0_1Plus1, vRigh0_1Plus1, vuIdx0_1, (uint16) 0, (unsigned char)7);
			#if COL_4
			w2L0_2 = (ushort16)vmac3(splitsrc, psl, vLeft0_2Plus1, vRigh0_2Plus1, vuIdx0_2, (uint16) 0, (unsigned char)7); 
			w2L0_3 = (ushort16)vmac3(splitsrc, psl, vLeft0_3Plus1, vRigh0_3Plus1, vuIdx0_3, (uint16) 0, (unsigned char)7);
			#endif
			
			w2L1_0 = (ushort16)vmac3(splitsrc, psl, vLeft1_0Plus1, vRigh1_0Plus1, vuIdx1_0, (uint16) 0, (unsigned char)7); 
			w2L1_1 = (ushort16)vmac3(splitsrc, psl, vLeft1_1Plus1, vRigh1_1Plus1, vuIdx1_1, (uint16) 0, (unsigned char)7);
			#if COL_4
			w2L1_2 = (ushort16)vmac3(splitsrc, psl, vLeft1_2Plus1, vRigh1_2Plus1, vuIdx1_2, (uint16) 0, (unsigned char)7); 
			w2L1_3 = (ushort16)vmac3(splitsrc, psl, vLeft1_3Plus1, vRigh1_3Plus1, vuIdx1_3, (uint16) 0, (unsigned char)7);
			#endif

			
			vLine2_0 = *(ushort16*)p_in_u16L0_0; 
			vLine2_1 = *(ushort16*)p_in_u16L0_1;    
			vLine2_2 = *(ushort16*)p_in_u16L0_2;  
			#if COL_4
			vLine2_3 = *(ushort16*)p_in_u16L0_3;    
			vLine2_4 = *(ushort16*)p_in_u16L0_4;  
			#endif
			
			p_in_u16L0_0 += 2*stride;   
			p_in_u16L0_1 += 2*stride;   
			p_in_u16L0_2 += 2*stride; 
			#if COL_4
			p_in_u16L0_3 += 2*stride; 
			p_in_u16L0_4 += 2*stride; 
			#endif


			// line3 --> line1
			vLine1_0 = vLine3_0;
			vLine1_1 = vLine3_1;
			vLine1_2 = vLine3_2;
			#if COL_4
			vLine1_3 = vLine3_3;
			vLine1_4 = vLine3_4;
			#endif
			
			vLine3_0 = *(ushort16*)p_in_u16L1_0; 
			vLine3_1 = *(ushort16*)p_in_u16L1_1;    
			vLine3_2 = *(ushort16*)p_in_u16L1_2;  
			#if COL_4
			vLine3_3 = *(ushort16*)p_in_u16L1_3;  
			vLine3_4 = *(ushort16*)p_in_u16L1_4;  
			#endif

			p_in_u16L1_0 += 2*stride;   
			p_in_u16L1_1 += 2*stride;   
			p_in_u16L1_2 += 2*stride; 
			#if COL_4
			p_in_u16L1_3 += 2*stride; 
			p_in_u16L1_4 += 2*stride; 
			#endif

		

			//--------------------------
			// bilinear @ x  4 commands
			//--------------------------
			w1L0_0 = (ushort16)vselect(vmac3(splitsrc, psl, w1L0_0, w2L0_0, bifactor0, (uint16) 0, (unsigned char)8),w1L0_0, vprRightMask); 
			w1L0_1 = (ushort16)vmac3(splitsrc, psl, w1L0_1, w2L0_1, bifactor1, (uint16) 0, (unsigned char)8);
			#if COL_4
			w1L0_2 = (ushort16)vmac3(splitsrc, psl, w1L0_2, w2L0_2, bifactor2, (uint16) 0, (unsigned char)8); 
			w1L0_3 = (ushort16)vmac3(splitsrc, psl, w1L0_3, w2L0_3, bifactor3, (uint16) 0, (unsigned char)8);
			#endif

			w1L1_0 = (ushort16)vselect(vmac3(splitsrc, psl, w1L1_0, w2L1_0, bifactor0, (uint16) 0, (unsigned char)8),w1L1_0, vprRightMask); 
			w1L1_1 = (ushort16)vmac3(splitsrc, psl, w1L1_1, w2L1_1, bifactor1, (uint16) 0, (unsigned char)8);
			#if COL_4
			w1L1_2 = (ushort16)vmac3(splitsrc, psl, w1L1_2, w2L1_2, bifactor2, (uint16) 0, (unsigned char)8); 
			w1L1_3 = (ushort16)vmac3(splitsrc, psl, w1L1_3, w2L1_3, bifactor3, (uint16) 0, (unsigned char)8);
			#endif

			// << -------------------------------------------------------
			#if (WDR_C_MODEL && WDR_VECC)
			if(64 == cols)
			{
				ret += check_ushort16_vecc_result(weight_phase0,     w1L0_0, 16);
				ret += check_ushort16_vecc_result(weight_phase0+16,  w1L0_1, 16);
				

				assert(ret == 0);
				ret += check_ushort16_vecc_result(weight_phase0+32,  w1L0_2, 16);
				if (ret){
					PRINT_C_GROUP("weight_phase0", weight_phase0, 32, 16, stderr);
					PRINT_CEVA_VRF("w1L0_2",w1L0_2,stderr);

				}
				assert(ret == 0);	
			}
			#endif


			vsplit(w1L0_0, (unsigned char)4, vLine0_Idx0, vWeight0_0);
			vsplit(w1L0_1, (unsigned char)4, vLine0_Idx1, vWeight0_1);
			#if COL_4
			vsplit(w1L0_2, (unsigned char)4, vLine0_Idx2, vWeight0_2);
			vsplit(w1L0_3, (unsigned char)4, vLine0_Idx3, vWeight0_3);
			#endif

			vsplit(w1L1_0, (unsigned char)4, vLine1_Idx0, vWeight1_0);
			vsplit(w1L1_1, (unsigned char)4, vLine1_Idx1, vWeight1_1);
			#if COL_4
			vsplit(w1L1_2, (unsigned char)4, vLine1_Idx2, vWeight1_2);
			vsplit(w1L1_3, (unsigned char)4, vLine1_Idx3, vWeight1_3);
			#endif



			vMin0_0 = (short16)vsubsat((ushort16)vVeri0_Col0,(ushort)32);
			vMin0_1 = (short16)vsubsat((ushort16)vVeri0_Col1,(ushort)32);
			#if COL_4
			vMin0_2 = (short16)vsubsat((ushort16)vVeri0_Col2,(ushort)32);
			vMin0_3 = (short16)vsubsat((ushort16)vVeri0_Col3,(ushort)32);
			#endif

			vMax0_0 = (short16)vaddsat((ushort16)vVeri0_Col0,(ushort)32);
			vMax0_1 = (short16)vaddsat((ushort16)vVeri0_Col1,(ushort)32);
			#if COL_4
			vMax0_2 = (short16)vaddsat((ushort16)vVeri0_Col2,(ushort)32);
			vMax0_3 = (short16)vaddsat((ushort16)vVeri0_Col3,(ushort)32);
			#endif
			
			vMin1_0 = (short16)vsubsat((ushort16)vVeri1_Col0,(ushort)32);
			vMin1_1 = (short16)vsubsat((ushort16)vVeri1_Col1,(ushort)32);
			#if COL_4
			vMin1_2 = (short16)vsubsat((ushort16)vVeri1_Col2,(ushort)32);
			vMin1_3 = (short16)vsubsat((ushort16)vVeri1_Col3,(ushort)32);
			#endif
			
			vMax1_0 = (short16)vaddsat((ushort16)vVeri1_Col0,(ushort)32);
			vMax1_1 = (short16)vaddsat((ushort16)vVeri1_Col1,(ushort)32);
			#if COL_4
			vMax1_2 = (short16)vaddsat((ushort16)vVeri1_Col2,(ushort)32);
			vMax1_3 = (short16)vaddsat((ushort16)vVeri1_Col3,(ushort)32);
			#endif

			// << -------------------------------------------------------


			wL0_0_s = (ushort16)vclip(vLine0_Idx0, vMin0_0, vMax0_0);
			wL0_1_s = (ushort16)vclip(vLine0_Idx1, vMin0_1, vMax0_1);
			#if COL_4
			wL0_2_s = (ushort16)vclip(vLine0_Idx2, vMin0_2, vMax0_2);
			wL0_3_s = (ushort16)vclip(vLine0_Idx3, vMin0_3, vMax0_3);
			#endif
			
			wL1_0_s = (ushort16)vclip(vLine1_Idx0, vMin1_0, vMax1_0);    
			wL1_1_s = (ushort16)vclip(vLine1_Idx1, vMin1_1, vMax1_1);    
			#if COL_4
			wL1_2_s = (ushort16)vclip(vLine1_Idx2, vMin1_2, vMax1_2);    
			wL1_3_s = (ushort16)vclip(vLine1_Idx3, vMin1_3, vMax1_3);    
			#endif

			wL0_0_s = vsubsat(wL0_0_s, (unsigned char)64);
			wL0_1_s = vsubsat(wL0_1_s, (unsigned char)64);
			#if COL_4
			wL0_2_s = vsubsat(wL0_2_s, (unsigned char)64);
			wL0_3_s = vsubsat(wL0_3_s, (unsigned char)64);
			#endif
			
			wL1_0_s = vsubsat(wL1_0_s, (unsigned char)64);
			wL1_1_s = vsubsat(wL1_1_s, (unsigned char)64);
			#if COL_4
			wL1_2_s = vsubsat(wL1_2_s, (unsigned char)64);
			wL1_3_s = vsubsat(wL1_3_s, (unsigned char)64);
			#endif


			// << -------------------------------------------------------
			#if (WDR_C_MODEL && WDR_VECC)
			if(64 == cols)
			{	
				ret += check_ushort16_vecc_result(lindex2_16,     wL0_0_s, 16);
				if (ret){
					PRINT_C_GROUP("w", lindex2_16, 0, 16, stderr);
					PRINT_CEVA_VRF("w0",wL0_0_s,stderr);

				}
				ret += check_ushort16_vecc_result(lindex2_16+16,  wL0_1_s, 16);
				if (ret){
					PRINT_C_GROUP("w", lindex2_16, 16, 16, stderr);
					PRINT_CEVA_VRF("w1",wL0_1_s,stderr);

				}
				ret += check_ushort16_vecc_result(lindex2_16+32,  wL0_2_s, 16);
				if (ret){
					PRINT_C_GROUP("lindex2_16", lindex2_16, 32, 16, stderr);
					PRINT_CEVA_VRF("wL0_2_s",wL0_2_s,stderr);

				}
				assert(ret == 0);

				ret += check_ushort16_vecc_result(lindex2_16_bak,     wL1_0_s, 16);
				if (ret){
					PRINT_C_GROUP("w", lindex2_16_bak, 0, 16, stderr);
					PRINT_CEVA_VRF("w0",wL1_0_s,stderr);

				}
				ret += check_ushort16_vecc_result(lindex2_16_bak+16,  wL1_1_s, 16);
				if (ret){
					PRINT_C_GROUP("w", lindex2_16_bak, 16, 16, stderr);
					PRINT_CEVA_VRF("w1",wL1_1_s,stderr);

				}

				assert(ret == 0);
			}
			#endif

			
			vuIdx0_0 = (uchar32) vcast(vsub((unsigned short)16,vWeight0_0),vWeight0_0);// lindex[0  -15]
			vuIdx0_1 = (uchar32) vcast(vsub((unsigned short)16,vWeight0_1),vWeight0_1);// lindex[16- 31]
			#if COL_4
			vuIdx0_2 = (uchar32) vcast(vsub((unsigned short)16,vWeight0_2),vWeight0_2);// lindex[0  -15]
			vuIdx0_3 = (uchar32) vcast(vsub((unsigned short)16,vWeight0_3),vWeight0_3);// lindex[16- 31]
			#endif
			
			vuIdx1_0 = (uchar32) vcast(vsub((unsigned short)16,vWeight1_0),vWeight1_0);// lindex[0  -15]
			vuIdx1_1 = (uchar32) vcast(vsub((unsigned short)16,vWeight1_1),vWeight1_1);// lindex[16- 31]
			#if COL_4
			vuIdx1_2 = (uchar32) vcast(vsub((unsigned short)16,vWeight1_2),vWeight1_2);// lindex[0  -15]
			vuIdx1_3 = (uchar32) vcast(vsub((unsigned short)16,vWeight1_3),vWeight1_3);// lindex[16- 31]
			#endif


			//--------------------------
			// LUT 12 command
			//--------------------------		
		#if 0
			vpld((short*)scale_table , (short16)wL0_0_s, vLeft0_0, vRigh0_0);// vLeft0is scale_table[lindex[k]], vRigh0is scale_table[lindex[k]+1]
			vpld((short*)scale_table , (short16)wL0_1_s, vLeft0_1, vRigh0_1);// vLeft1is scale_table[lindex[k]], vRigh1is scale_table[lindex[k]+1]
			#if COL_4
			vpld((short*)scale_table , (short16)wL0_2_s, vLeft0_2, vRigh0_2);// vLeft0is scale_table[lindex[k]], vRigh0is scale_table[lindex[k]+1]
			vpld((short*)scale_table , (short16)wL0_3_s, vLeft0_3, vRigh0_3);// vLeft1is scale_table[lindex[k]], vRigh1is scale_table[lindex[k]+1]
			#endif

			vpld((short*)scale_table , (short16)wL1_0_s, vLeft1_0, vRigh1_0);
			vpld((short*)scale_table , (short16)wL1_1_s, vLeft1_1, vRigh1_1);
			#if COL_4
			vpld((short*)scale_table , (short16)wL1_2_s, vLeft1_2, vRigh1_2);
			vpld((short*)scale_table , (short16)wL1_3_s, vLeft1_3, vRigh1_3);
			#endif

		#else
			vLeft0_0 = (short16)vpld(rel,pScaleTab , (short16)wL0_0_s);
			vRigh0_0 = (short16)vpld(rel,pScaleTab , vadd(wL0_0_s,(unsigned short)1)); 

			vLeft0_1 = (short16)vpld(rel,pScaleTab , (short16)wL0_1_s);
			vRigh0_1 = (short16)vpld(rel,pScaleTab , vadd(wL0_1_s,(unsigned short)1));

			vLeft0_2 = (short16)vpld(rel,pScaleTab , (short16)wL0_2_s);
			vRigh0_2 = (short16)vpld(rel,pScaleTab , vadd(wL0_2_s,(unsigned short)1)); 

			vLeft0_3 = (short16)vpld(rel,pScaleTab , (short16)wL0_3_s);
			vRigh0_3 = (short16)vpld(rel,pScaleTab , vadd(wL0_3_s,(unsigned short)1)); 


			vLeft1_0 = (short16)vpld(rel,pScaleTab , (short16)wL1_0_s);
			vRigh1_0 = (short16)vpld(rel,pScaleTab , vadd(wL1_0_s,(unsigned short)1)); 

			vLeft1_1 = (short16)vpld(rel,pScaleTab , (short16)wL1_1_s);
			vRigh1_1 = (short16)vpld(rel,pScaleTab , vadd(wL1_1_s,(unsigned short)1)); 

			vLeft1_2 = (short16)vpld(rel,pScaleTab , (short16)wL1_2_s);
			vRigh1_2 = (short16)vpld(rel,pScaleTab , vadd(wL1_2_s,(unsigned short)1)); 

			vLeft1_3 = (short16)vpld(rel,pScaleTab , (short16)wL1_3_s);
			vRigh1_3 = (short16)vpld(rel,pScaleTab , vadd(wL1_3_s,(unsigned short)1)); 
		#endif

			w0L0 = (ushort16)vmac3(splitsrc, psl, vLeft0_0, vRigh0_0, vuIdx0_0, (uint16) 8, (unsigned char)4); 
			w1L0 = (ushort16)vmac3(splitsrc, psl, vLeft0_1, vRigh0_1, vuIdx0_1, (uint16) 8, (unsigned char)4);
			#if COL_4
			w2L0 = (ushort16)vmac3(splitsrc, psl, vLeft0_2, vRigh0_2, vuIdx0_2, (uint16) 8, (unsigned char)4); 
			w3L0 = (ushort16)vmac3(splitsrc, psl, vLeft0_3, vRigh0_3, vuIdx0_3, (uint16) 8, (unsigned char)4);
			#endif

			w0L1 = (ushort16)vmac3(splitsrc, psl, vLeft1_0, vRigh1_0, vuIdx1_0, (uint16) 8, (unsigned char)4); 
			w1L1 = (ushort16)vmac3(splitsrc, psl, vLeft1_1, vRigh1_1, vuIdx1_1, (uint16) 8, (unsigned char)4);
			#if COL_4
			w2L1 = (ushort16)vmac3(splitsrc, psl, vLeft1_2, vRigh1_2, vuIdx1_2, (uint16) 8, (unsigned char)4); 
			w3L1 = (ushort16)vmac3(splitsrc, psl, vLeft1_3, vRigh1_3, vuIdx1_3, (uint16) 8, (unsigned char)4);
			#endif

			// << -------------------------------------------------------
			#if (WDR_C_MODEL && WDR_VECC)
			if(64 == cols)
			{
				ret += check_ushort16_vecc_result(weight,     w0L0, 16);
				if (ret){
					PRINT_C_GROUP("w", weight, 0, 16, stderr);
					PRINT_CEVA_VRF("w0",w0L0,stderr);

				}
				ret += check_ushort16_vecc_result(weight+16,  w1L0, 16);
				if (ret){
					PRINT_C_GROUP("w", weight, 16, 16, stderr);
					PRINT_CEVA_VRF("w1",w1L0,stderr);

				}

				ret += check_ushort16_vecc_result(weight+32,  w2L0, 16);
				if (ret){
					PRINT_C_GROUP("w", weight, 32, 16, stderr);
					PRINT_CEVA_VRF("w1",w2L0,stderr);

				}
				assert(ret == 0);
				ret += check_ushort16_vecc_result(weight_bak,     w0L1, 16);
				if (ret){
					PRINT_C_GROUP("w", weight_bak, 0, 16, stderr);
					PRINT_CEVA_VRF("w0",w0L1,stderr);

				}
				ret += check_ushort16_vecc_result(weight_bak+16,  w1L1, 16);
				if (ret){
					PRINT_C_GROUP("w", weight_bak, 16, 16, stderr);
					PRINT_CEVA_VRF("w1",w1L1,stderr);

				}

				ret += check_ushort16_vecc_result(weight_bak+48,  w3L1, 16);
				if (ret){
					PRINT_C_GROUP("w", weight_bak, 48, 16, stderr);
					PRINT_CEVA_VRF("w1",w3L1,stderr);

				}
				assert(ret == 0);
			}
			#endif

			vDataIn0	= (short16)vsubsat((ushort16)vDataIn0,	(ushort)(blacklevel*2));
			vDataIn1	= (short16)vsubsat((ushort16)vDataIn1,	(ushort)(blacklevel*2));
			#if COL_4
			vDataIn2	= (short16)vsubsat((ushort16)vDataIn2,	(ushort)(blacklevel*2));
			vDataIn3	= (short16)vsubsat((ushort16)vDataIn3,	(ushort)(blacklevel*2));
			#endif

			vDataIn4	= (short16)vsubsat((ushort16)vDataIn4,	(ushort)(blacklevel*2));
			vDataIn5	= (short16)vsubsat((ushort16)vDataIn5,	(ushort)(blacklevel*2));
			#if COL_4
			vDataIn6	= (short16)vsubsat((ushort16)vDataIn6,	(ushort)(blacklevel*2));
			vDataIn7	= (short16)vsubsat((ushort16)vDataIn7,	(ushort)(blacklevel*2));
			#endif

			vDataOut0	= vmac(psl, vDataIn0, w0L0, blackMpy256, (unsigned char)10);   
			vDataOut1 	= vmac(psl, vDataIn1, w1L0, blackMpy256, (unsigned char)10);    
			#if COL_4
			vDataOut2	= vmac(psl, vDataIn2, w2L0, blackMpy256, (unsigned char)10);   
			vDataOut3 	= vmac(psl, vDataIn3, w3L0, blackMpy256, (unsigned char)10);    
			#endif
			
			vDataOut4	= vmac(psl, vDataIn4, w0L1, blackMpy256, (unsigned char)10);   
			vDataOut5 	= vmac(psl, vDataIn5, w1L1, blackMpy256, (unsigned char)10);    
			#if COL_4
			vDataOut6	= vmac(psl, vDataIn6, w2L1, blackMpy256, (unsigned char)10);   
			vDataOut7 	= vmac(psl, vDataIn7, w3L1, blackMpy256, (unsigned char)10);    
			#endif

			vDataOut0	= vmin(vDataOut0, (short16) 1023);   
			vDataOut1 	= vmin(vDataOut1, (short16) 1023);    
			#if COL_4
			vDataOut2	= vmin(vDataOut2, (short16) 1023);   
			vDataOut3 	= vmin(vDataOut3, (short16) 1023);    
			#endif
			
			vDataOut4	= vmin(vDataOut4, (short16) 1023);   
			vDataOut5 	= vmin(vDataOut5, (short16) 1023);   
			#if COL_4
			vDataOut6	= vmin(vDataOut6, (short16) 1023);   
			vDataOut7 	= vmin(vDataOut7, (short16) 1023);   
			#endif
			
			vst(vDataOut0,(short16*)(pTmpOut_vecc)   ,			vprOutMask0);  
			vst(vDataOut1,(short16*)(pTmpOut_vecc+16),			vprOutMask1);  
			#if COL_4
			vst(vDataOut2,(short16*)(pTmpOut_vecc+32),			vprOutMask2);  
			vst(vDataOut3,(short16*)(pTmpOut_vecc+48),			vprOutMask3);  
			#endif
			vst(vDataOut4,(short16*)(pTmpOut_vecc+blockWidth),			vprOutMask0);  
			vst(vDataOut5,(short16*)(pTmpOut_vecc+blockWidth+16),		vprOutMask1);  
			#if COL_4
			vst(vDataOut6,(short16*)(pTmpOut_vecc+blockWidth+32),		vprOutMask2);  
			vst(vDataOut7,(short16*)(pTmpOut_vecc+blockWidth+48),		vprOutMask3);  
			#endif
			pTmpOut_vecc += 2*blockWidth;                                        

			#if (WDR_C_MODEL && WDR_VECC)
				ret += check_result(pTmpOut-2*blockWidth, pTmpOut_vecc-2*blockWidth, min_ (64,cols), 2,blockWidth,blockWidth);	
				if (ret){
					PRINT_CEVA_VRF("vDataOut0",vDataOut0,stderr);
				}	
				assert(ret == 0);
			#endif
			
		#endif	
		}

	#ifdef __XM4__
		PROFILER_END();
	#endif
		/*
		if (y_base < 32)
		{
			for ( i = 31 ; i > 0 ; i-- )
			{
				memcpy(pPixel_out+(i-1)*64,pPixel_out+i*64,sizeof(RK_U16)*64);
			}
		}*/
#endif
	

} // wdr_process_block()



//////////////////////////////////////////////////////////////////////////

#if WDR_DEBUG
CODE_MFNR_EX
int check_short16_vecc_result(RK_U16* data1, short16 data2, int  num)
{
#if WIN32
	for ( int i = 0 ; i < num ; i++ )
	{
		if(data1[i] != data2[i] )
		{	
			return -1;
		}
	}
#endif
	return 0;
}

CODE_MFNR_EX
int check_ushort16_vecc_result(RK_U16* data1, ushort16 data2, int  num)
{
#if WIN32
	for ( int i = 0 ; i < num ; i++ )
	{
		if(data1[i] != data2[i] )
		{	
			return -1;
		}
	}
#endif
	return 0;
}

CODE_MFNR_EX
int check_result(RK_U16* data1, RK_U16* data2,int Wid ,int  Hgt, int stride1, int stride2)
{
	for ( int i = 0 ; i < Hgt ; i++ )
	{
		for ( int j = 0 ; j < Wid ; j++ )
		{
			if(data1[i*stride1 + j] != data2[i*stride2 + j] )
			{	
				return -1;
			}
		}
	}

	return 0;
}

CODE_MFNR_EX
int check_vector_unsigned_reg(ushort16 data1, ushort16 data2)
{
#if WIN32
	for ( int i = 0 ; i < data1.num_of_elements ; i++ )
	{
		if(data1[i] != data2[i] )
		{	
			return -1;
		}
	}
#endif
	return 0;
}

CODE_MFNR_EX
int check_vector_reg(short16 data1, short16 data2)
{
#if WIN32
	for ( int i = 0 ; i < data1.num_of_elements ; i++ )
	{
		if(data1[i] != data2[i] )
		{	
			return -1;
		}
	}
#endif
	return 0;
}

CODE_MFNR_EX
void cul_wdr_cure2(unsigned short *table, RK_U16 exp_times)
{

	int	idx;
	int	i;

	if (exp_times>24)
	{
		exp_times = 24;
	}
	exp_times = exp_times-1;

	idx = exp_times;
	if(exp_times == idx)
	{
		for(i=0;i<961;i++)
		{
			table[i] = cure_table[idx][i];
		}
	}
	else
	{
		int  s1,s2;

		s1 = idx+1-exp_times;
		s2 = exp_times-idx;
		for(i=0;i<961;i++)
		{
			table[i] = cure_table[idx][i]*s1+cure_table[idx+1][i]*s2;
		}
	}
}

CODE_MFNR_EX
unsigned short vsubsat_c(unsigned short a, unsigned short b)
{
	if ( (a-b) < 0)
		return 0;
	else
		return (a-b);

}
#endif


CODE_MFNR_EX
void set_char32(uchar32 &data, int offset)
{
	unsigned char chargroup[32];
	chargroup[0] = (MAX_BIT_VALUE - ((offset+0) & MAX_BIT_V_MINUS1));
	chargroup[1] = (MAX_BIT_VALUE - ((offset+1) & MAX_BIT_V_MINUS1));
	chargroup[2] = (MAX_BIT_VALUE - ((offset+2) & MAX_BIT_V_MINUS1));
	chargroup[3] = (MAX_BIT_VALUE - ((offset+3) & MAX_BIT_V_MINUS1));
	chargroup[4] = (MAX_BIT_VALUE - ((offset+4) & MAX_BIT_V_MINUS1));
	chargroup[5] = (MAX_BIT_VALUE - ((offset+5) & MAX_BIT_V_MINUS1));
	chargroup[6] = (MAX_BIT_VALUE - ((offset+6) & MAX_BIT_V_MINUS1));
	chargroup[7] = (MAX_BIT_VALUE - ((offset+7) & MAX_BIT_V_MINUS1));
	chargroup[8] = (MAX_BIT_VALUE - ((offset+8) & MAX_BIT_V_MINUS1));
	chargroup[9] = (MAX_BIT_VALUE - ((offset+9) & MAX_BIT_V_MINUS1));
	chargroup[10] = (MAX_BIT_VALUE - ((offset+10) & MAX_BIT_V_MINUS1));
	chargroup[11] = (MAX_BIT_VALUE - ((offset+11) & MAX_BIT_V_MINUS1));
	chargroup[12] = (MAX_BIT_VALUE - ((offset+12) & MAX_BIT_V_MINUS1));
	chargroup[13] = (MAX_BIT_VALUE - ((offset+13) & MAX_BIT_V_MINUS1));
	chargroup[14] = (MAX_BIT_VALUE - ((offset+14) & MAX_BIT_V_MINUS1));
	chargroup[15] = (MAX_BIT_VALUE - ((offset+15) & MAX_BIT_V_MINUS1));

	chargroup[16] = (offset+0)  & MAX_BIT_V_MINUS1 ;
	chargroup[17] = (offset+1)  & MAX_BIT_V_MINUS1 ;
	chargroup[18] = (offset+2)  & MAX_BIT_V_MINUS1 ;
	chargroup[19] = (offset+3)  & MAX_BIT_V_MINUS1 ;
	chargroup[20] = (offset+4)  & MAX_BIT_V_MINUS1 ;
	chargroup[21] = (offset+5)  & MAX_BIT_V_MINUS1 ;
	chargroup[22] = (offset+6)  & MAX_BIT_V_MINUS1 ;
	chargroup[23] = (offset+7)  & MAX_BIT_V_MINUS1 ;
	chargroup[24] = (offset+8)  & MAX_BIT_V_MINUS1 ;
	chargroup[25] = (offset+9)  & MAX_BIT_V_MINUS1 ;
	chargroup[26] = (offset+10) & MAX_BIT_V_MINUS1 ;
	chargroup[27] = (offset+11) & MAX_BIT_V_MINUS1 ;
	chargroup[28] = (offset+12) & MAX_BIT_V_MINUS1 ;
	chargroup[29] = (offset+13) & MAX_BIT_V_MINUS1 ;
	chargroup[30] = (offset+14) & MAX_BIT_V_MINUS1 ;
	chargroup[31] = (offset+15) & MAX_BIT_V_MINUS1 ;
	data = *(uchar32*)chargroup;
}

CODE_MFNR_EX
void wdrPreFilterBlock( ushort *pBaseRawData, ushort *p_dst, int BlockHeight, int BlockWidth )
{
	RK_U16		*p1, *p2, *p3;
	int			x,y;  
	int 		ScaleDownlight = 0;

	p1 = pBaseRawData;
	p2 = pBaseRawData + BlockWidth;
	p3 = pBaseRawData + 2 * BlockWidth;

	for (y = 1; y < BlockHeight - 1 ; y++)
	{
		for (x = 1; x < BlockWidth - 1; x++)
		{

			ScaleDownlight	= p1[x - 1] + 2 * p1[x] + p1[x + 1] + 2 * p2[x - 1] + 4 * p2[x] + 2 * p2[x + 1] + p3[x - 1] + 2 * p3[x] + p3[x + 1];

			ScaleDownlight >>= 6;

			p_dst[ ( y - 1 ) * BlockWidth + x - 1 ] = ScaleDownlight;
		}
		p1 += BlockWidth;
		p2 += BlockWidth;
		p3 += BlockWidth;
	}
}

CODE_MFNR_EX
void CalcuHist( ushort *p_src, ushort *pcount_mat, uint *pweight_mat, int BlockHeight, int BlockWidth, int statisticWidth, int row )
{
	RK_U16		lindex;
	int			x,y;
	int 		idx, idy; 
	int 		ScaleDownlight = 0;

	for (y = 0; y < BlockHeight - 2; y++)
	{
		for (x = 0; x < BlockWidth - 2; x++)
		{

			ScaleDownlight	= p_src[ y * BlockWidth + x ];
			lindex = (ScaleDownlight + 1024) >> 11;

			idx = (x + 16) >> 5;
			idy = ( row * 32 + (y + 16) ) >> 5;

			pcount_mat [lindex*256 + idy*statisticWidth + idx] = pcount_mat [lindex*256 + idy*statisticWidth + idx] + 1;

			pweight_mat[lindex*256 + idy*statisticWidth + idx] = pweight_mat[lindex*256 + idy*statisticWidth + idx] + ScaleDownlight;

		}
	}
}

CODE_MFNR_EX
void HistFilter( ushort *pcount_mat, uint *pweight_mat, int statisticHeight, int statisticWidth )
{
	int			i,x,y;  
	//filter
	for (i = 0; i < 9; i++)
	{
		RK_U32	tl, tm, tr;
		for (y = 0; y < statisticHeight; y++)
		{
			tl = 0;
			tm = 0;
			tr = pcount_mat[i*256 + y*statisticWidth];
			for (x = 0; x < statisticWidth; x++)
			{
				tl = tm;
				tm = tr;
				if (x < statisticWidth - 1)
					tr = pcount_mat[i*256 + y*statisticWidth + x + 1];
				else
					tr = 0;
				pcount_mat[i*256 + y*statisticWidth + x] = (tl) + (tm *2) + (tr);
			}
		}
		for (x = 0; x < statisticWidth; x++)
		{
			tl = 0;
			tm = 0;
			tr = pcount_mat[i*256 + x];
			for (y = 0; y < statisticHeight; y++)
			{
				tl = tm;
				tm = tr;
				if (y < statisticHeight - 1)
					tr = pcount_mat[i*256 + (y + 1)*statisticWidth + x];
				else
					tr = 0;
				pcount_mat[i*256 + y*statisticWidth + x] = (tl) + (tm *2) + (tr );
			}
		}
	}



	// do the 3-rd array filter .

	for (y = 0; y < statisticHeight; y++)
	{
		for (x = 0; x < statisticWidth; x++)
		{
			RK_U32	tl, tm, tr;

			tl = 0;
			tm = 0;
			tr = pcount_mat[0*256 + y*statisticWidth + x];
			for (i = 0; i < 9; i++)
			{
				tl = tm;
				tm = tr;
				if (i < 8)
					tr = pcount_mat[(i + 1)*256 + y*statisticWidth + x];
				else
					tr = 0;
				pcount_mat[i*256 + y*statisticWidth + x] = (tl) + (tm *2) + (tr);// enlarge 4 times.
			}
		}
	}

	//filter
	// do the 3-rd array filter .
	for (i = 0; i < 9; i++)
	{
		RK_U32	tl, tm, tr;
		for (y = 0; y < statisticHeight; y++)
		{
			tl = 0;
			tm = 0;
			tr = pweight_mat[i*256 + y*statisticWidth];
			for (x = 0; x < statisticWidth; x++)
			{
				tl = tm;
				tm = tr;
				if (x < statisticWidth - 1)
					tr = pweight_mat[i*256 + y*statisticWidth + x + 1];
				else
					tr = 0;
				pweight_mat[i*256 + y*statisticWidth + x] = (tl >> 2) + (tm >> 1) + (tr >> 2);
			}
		}
		for (x = 0; x < statisticWidth; x++)
		{
			tl = 0;
			tm = 0;
			tr = pweight_mat[i*256 + x];
			for (y = 0; y < statisticHeight; y++)
			{
				tl = tm;
				tm = tr;
				if (y < statisticHeight - 1)
					tr = pweight_mat[i*256 + (y + 1)*statisticWidth + x];
				else
					tr = 0;
				pweight_mat[i*256 + y*statisticWidth + x] = (tl >> 2) + (tm >> 1) + (tr >> 2);
			}
		}
	}

	for (y = 0; y < statisticHeight; y++)
	{
		for (x = 0; x < statisticWidth; x++)
		{
			RK_U32	tl, tm, tr;

			tl = 0;
			tm = 0;
			tr = pweight_mat[0*256 + y*statisticWidth + x];
			for (i = 0; i < 9; i++)
			{
				tl = tm;
				tm = tr;
				if (i < 8)
					tr = pweight_mat[(i + 1)*256 + y*statisticWidth + x];
				else
					tr = 0;
				pweight_mat[i*256 + y*statisticWidth + x] = (tl >> 2) + (tm >> 1) + (tr >> 2);
			}
		}
	}
}

CODE_MFNR_EX
void normalizeWeight( ushort *pcount_mat, uint *pweight_mat, int statisticHeight, int statisticWidth )
{
	int			i,x,y;  
	for (i = 0; i < 9; i++)
	{
		for (y = 0; y < statisticHeight; y++)
		{
			for (x = 0; x < statisticWidth; x++)
			{
				if (pcount_mat[i*256 + y*statisticWidth + x])
				{
					pcount_mat[i*256 +y*statisticWidth + x] = (RK_U16)(1*pweight_mat[i*256 + y*statisticWidth + x] / pcount_mat[i*256 + y*statisticWidth + x]);
				}
				else
				{
					pcount_mat[i*256 +y*statisticWidth + x] = 0;
				}
				if (pcount_mat[i*256 +y*statisticWidth + x] > 16*1023){
					pcount_mat[i*256 +y*statisticWidth + x] = 16*1023;
				}
			}
		}
	}
}


CODE_MFNR_EX
void CalcuHistTranspose( ushort *p_src, ushort *pcount_mat, uint *pweight_mat, int BlockHeight, int BlockWidth, int statisticWidth, int row )
{
	RK_U16		lindex;
	int			x,y,blacklevel=256;  
	int 		idx, idy; 
	int 		ScaleDownlight = 0;

	for (y = 1; y < BlockHeight - 1; y++)
	{
		for (x = 1; x < BlockWidth - 1; x++)
		{

			ScaleDownlight	= p_src[ y * BlockWidth + x ];
			lindex = (ScaleDownlight + 1024) >> 11;
			assert(lindex < 9);
			idx = (x  - 1 + 16) >> 5;
			idy = row * 32 + ((y - 1 + 16) >> 5);

			pcount_mat [(idy*statisticWidth + idx)*16 + lindex] = pcount_mat [(idy*statisticWidth + idx)*16 + lindex] + 1;

			pweight_mat[(idy*statisticWidth + idx)*16 + lindex] = pweight_mat[(idy*statisticWidth + idx)*16 + lindex] + ScaleDownlight;

		}
	}
}
CODE_MFNR_EX
void HistFilterTranspose( ushort *pcount_mat, uint *pweight_mat, int statisticHeight, int statisticWidth )
{
	int i,x,y;
	for (i = 0; i < 9; i++)
	{
		RK_U32	tl, tm, tr;
		for (y = 0; y < statisticHeight; y++)
		{
			tl = 0;
			tm = 0;
			tr = pcount_mat[i + y*statisticWidth*16];
			for (x = 0; x < statisticWidth; x++)
			{
				tl = tm;
				tm = tr;
				if (x < statisticWidth - 1)
					tr = pcount_mat[i + (y*statisticWidth + x + 1)*16 ];
				else
					tr = 0;
				pcount_mat[i + (y*statisticWidth + x)*16 ] = (tl) + (tm *2) + (tr);
			}
		}
		for (x = 0; x < statisticWidth; x++)
		{
			tl = 0;
			tm = 0;
			tr = pcount_mat[i + x*16];
			for (y = 0; y < statisticHeight; y++)
			{
				tl = tm;
				tm = tr;
				if (y < statisticHeight - 1)
					tr = pcount_mat[i + ((y + 1)*statisticWidth + x)*16];
				else
					tr = 0;
				pcount_mat[i + (y*statisticWidth + x)*16] = (tl) + (tm *2) + (tr );
			}
		}
	}


	
	// do the 3-rd array filter .
	
	for (y = 0; y < statisticHeight; y++)
	{
		for (x = 0; x < statisticWidth; x++)
		{
			RK_U32	tl, tm, tr;

			tl = 0;
			tm = 0;
			tr = pcount_mat[0 + (y*statisticWidth + x)*16];
			for (i = 0; i < 9; i++)
			{
				tl = tm;
				tm = tr;
				if (i < 8)
					tr = pcount_mat[(i + 1) + (y*statisticWidth + x)*16];
				else
					tr = 0;
				pcount_mat[i + (y*statisticWidth + x)*16] = (tl) + (tm *2) + (tr);// enlarge 4 times.
			}
		}
	}
	//filter
	// do the 3-rd array filter .
	for (i = 0; i < 9; i++)
	{
		RK_U32	tl, tm, tr;
		for (y = 0; y < statisticHeight; y++)
		{
			tl = 0;
			tm = 0;

			tr = pweight_mat[i + (y*statisticWidth)*16];

			for (x = 0; x < statisticWidth; x++)
			{
				tl = tm;
				tm = tr;
				if (x < statisticWidth - 1)
				{
					tr = pweight_mat[i + (y*statisticWidth + x + 1)*16];
				}
				else
					tr = 0;

				pweight_mat[i + (y*statisticWidth + x)*16] = (tl >> 2) + (tm >> 1) + (tr >> 2);
				
			}
		}
		for (x = 0; x < statisticWidth; x++)
		{
			tl = 0;
			tm = 0;
			tr = pweight_mat[i + x*16];

			for (y = 0; y < statisticHeight; y++)
			{
				tl = tm;
				tm = tr;
				if (y < statisticHeight - 1)
				{
					tr = pweight_mat[i + ((y + 1)*statisticWidth + x)*16];
				}
				else
					tr = 0;

				pweight_mat[i+ (y*statisticWidth + x)*16] = (tl >> 2) + (tm >> 1) + (tr >> 2);
			}
		}
	}
	
	for (y = 0; y < statisticHeight; y++)
	{
		for (x = 0; x < statisticWidth; x++)
		{
			RK_U32	tl, tm, tr;

			tl = 0;
			tm = 0;

			tr = pweight_mat[(y*statisticWidth + x)*16];

			for (i = 0; i < 9; i++)
			{
				tl = tm;
				tm = tr;
				if (i < 8)
				{

					tr = pweight_mat[(i + 1)+ (y*statisticWidth + x)*16];

				}
				else
					tr = 0;

				pweight_mat[i + (y*statisticWidth + x)*16] = (tl >> 2) + (tm >> 1) + (tr >> 2);
			}
		}
	}

}
CODE_MFNR_EX
void normalizeWeightTranspose( ushort *pcount_mat, uint *pweight_mat, int statisticHeight, int statisticWidth )
{
	int			i,x,y;  
	for (i = 0; i < 9; i++)
	{
		for (y = 0; y < statisticHeight; y++)
		{
			for (x = 0; x < statisticWidth; x++)
			{
				if (pcount_mat[i + (y*statisticWidth + x)*16])
				{
					pcount_mat[i + (y*statisticWidth + x)*16] = (RK_U16)(1*pweight_mat[i + (y*statisticWidth + x)*16] / pcount_mat[i + (y*statisticWidth + x)*16]);
				}
				else
				{
					pcount_mat[i + (y*statisticWidth + x)*16] = 0;
				}
				if (pcount_mat[i + (y*statisticWidth + x)*16] > 16*1023){
					pcount_mat[i + (y*statisticWidth + x)*16] = 16*1023;// mux for RK_U16, OUTPUT.
				}
			}
		}
	}
}









//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// add by shm @2016.08.31
//   1 2 1      v0_0      v0_1     v0_2     v0_3      v0_4
//   2 4 2      v1_0      v1_1     v1_2     v1_3      v1_4   --->result0_0    result0_1    result0_0    result0_1
//   1 2 1      v2_0      v2_1     v2_2     v2_3      v2_4   --->result1_0    result1_1    result1_0    result1_1
//              v3_0      v3_1     v3_2     v3_3      v3_4   v2_x, v3_x ---> used for next loop
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CODE_MFNR_EX
void wdrPreFilterBlock_Vec( RK_U16 *p_src,      // <<! [ in ]: src data( boundaries have been copyied )
							RK_U16 *p_dst,      // <<! [ out ]: filtered data
							RK_S32 u32Rows,     // <<! [ in ]: row of src data
							RK_S32 u32Cols,     // <<! [ in ]: col of src data
							RK_S32 srcStride )  // <<! [ in ]: stride of src data
{
	RK_U16 vprMask1 = -1;
	RK_U16 vprMask2 = -1;
	RK_U16 vprMask3 = -1;
	RK_U16 vprMask4 = -1;

	RK_U16 vprMaskLastCol1 = -1;
	RK_U16 vprMaskLastCol2 = -1;
	RK_U16 vprMaskLastCol3 = -1;
	RK_U16 vprMaskLastCol4 = -1;

	if( u32Cols & 63 )
	{
		RK_U32 bits = u32Cols & 63;

		if ( bits < 16 ) 
		{
			vprMaskLastCol1 = ( 1 << bits ) - 1;
			vprMaskLastCol2 = 0;
			vprMaskLastCol3 = 0;
			vprMaskLastCol4 = 0;
		}
		else if ( bits >= 16 && bits < 32 ) 
		{
			vprMaskLastCol2 = ( 1 << ( bits - 16 ) ) - 1;
			vprMaskLastCol3 = 0;
			vprMaskLastCol4 = 0;
		}
		else if ( bits >= 32 && bits < 48 ) 
		{
			vprMaskLastCol3 = ( 1 << ( bits - 32 ) ) - 1;
			vprMaskLastCol4 = 0;
		}
		else
		{
			vprMaskLastCol4 = ( 1 << ( bits - 48 ) ) - 1;
		}
	}

	ushort16 v0_0, v0_1, v0_2, v0_3, v0_4;
	ushort16 v1_0, v1_1, v1_2, v1_3, v1_4;
	ushort16 v2_0, v2_1, v2_2, v2_3, v2_4;
	ushort16 v3_0, v3_1, v3_2, v3_3, v3_4;
	uint16 vacc0_0, vacc0_1, vacc1_0, vacc1_1;
	ushort16 result0_0, result0_1, result1_0, result1_1;

	RK_U8 coeff[32] = { 1, 2, 1, 0, 2, 4, 2, 0, 1, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	uchar32 v_coeff = *(uchar32*)coeff;

	RK_S32 outLoopX = ( u32Cols + 63 ) >> 6;

	RK_U16 *p0, *p1, *p2, *p3, *p4;
	RK_U16 *p_dst1, *p_dst2, *p_dst3, *p_dst4;

	//one loop: 2 * 64
	for( RK_S32 x = 0; x < outLoopX; x ++ )
	{
		if( x == outLoopX - 1 )
		{
			vprMask1 = vprMaskLastCol1;
			vprMask2 = vprMaskLastCol2;
			vprMask3 = vprMaskLastCol3;
			vprMask4 = vprMaskLastCol4;
		}
		//load data
		p0 = p_src + x * 64; p1 = p0 + 16; p2 = p1 + 16; p3 = p2 + 16; p4 = p3 + 16;
		p_dst1 = p_dst + x * 64; p_dst2 = p_dst1 + 16; p_dst3 = p_dst2 + 16; p_dst4 = p_dst3 + 16;

		v0_0 = *( ushort16* )( p0 ); v0_1 = *( ushort16* )( p1 ); v0_2 = *( ushort16* )( p2 );
		v0_3 = *( ushort16* )( p3 ); v0_4 = *( ushort16* )( p4 );
		p0 += srcStride; p1 += srcStride; p2 += srcStride; p3 += srcStride; p4 += srcStride;

		v1_0 = *( ushort16* )( p0 ); v1_1 = *( ushort16* )( p1 ); v1_2 = *( ushort16* )( p2 );
		v1_3 = *( ushort16* )( p3 ); v1_4 = *( ushort16* )( p4 );
		p0 += srcStride; p1 += srcStride; p2 += srcStride; p3 += srcStride; p4 += srcStride;

		v2_0 = *( ushort16* )( p0 ); v2_1 = *( ushort16* )( p1 ); v2_2 = *( ushort16* )( p2 );
		v2_3 = *( ushort16* )( p3 ); v2_4 = *( ushort16* )( p4 );
		p0 += srcStride; p1 += srcStride; p2 += srcStride; p3 += srcStride; p4 += srcStride;

		v3_0 = *( ushort16* )( p0 ); v3_1 = *( ushort16* )( p1 ); v3_2 = *( ushort16* )( p2 );
		v3_3 = *( ushort16* )( p3 ); v3_4 = *( ushort16* )( p4 );
		p0 += srcStride; p1 += srcStride; p2 += srcStride; p3 += srcStride; p4 += srcStride;

		RK_S32 y = 0;
		for( ; y < u32Rows - 1; y += 2 )
		{
			//even line, 0~31
			vacc0_0 = (uint16)vswmpy5(v0_0, v0_1, v_coeff, SW_CONFIG(0, 0, 0, 0, 0, 0));
			vacc0_1 = (uint16)vswmpy5(v0_1, v0_2, v_coeff, SW_CONFIG(0, 0, 0, 0, 0, 0));

			vacc0_0 = vswmac5(accumulate, v1_0, v1_1, v_coeff, SW_CONFIG(0, 0, 0, 4, 0, 0), vacc0_0);
			vacc0_1 = vswmac5(accumulate, v1_1, v1_2, v_coeff, SW_CONFIG(0, 0, 0, 4, 0, 0), vacc0_1);

			result0_0 = ( ushort16 )vswmac5(psl, v2_0, v2_1, v_coeff, SW_CONFIG(6, 0, 0, 8, 0, 0), vacc0_0);
			result0_1 = ( ushort16 )vswmac5(psl, v2_1, v2_2, v_coeff, SW_CONFIG(6, 0, 0, 8, 0, 0), vacc0_1);

			vst( result0_0, p_dst1, vprMask1 ); p_dst1 += srcStride;
			vst( result0_1, p_dst2, vprMask2 ); p_dst2 += srcStride;

			//even line, 32~63
			vacc0_0 = (uint16)vswmpy5(v0_2, v0_3, v_coeff, SW_CONFIG(0, 0, 0, 0, 0, 0));
			vacc0_1 = (uint16)vswmpy5(v0_3, v0_4, v_coeff, SW_CONFIG(0, 0, 0, 0, 0, 0));

			vacc0_0 = vswmac5(accumulate, v1_2, v1_3, v_coeff, SW_CONFIG(0, 0, 0, 4, 0, 0), vacc0_0);
			vacc0_1 = vswmac5(accumulate, v1_3, v1_4, v_coeff, SW_CONFIG(0, 0, 0, 4, 0, 0), vacc0_1);

			result0_0 = ( ushort16 )vswmac5(psl, v2_2, v2_3, v_coeff, SW_CONFIG(6, 0, 0, 8, 0, 0), vacc0_0);
			result0_1 = ( ushort16 )vswmac5(psl, v2_3, v2_4, v_coeff, SW_CONFIG(6, 0, 0, 8, 0, 0), vacc0_1);

			vst( result0_0, p_dst3, vprMask3 ); p_dst3 += srcStride;
			vst( result0_1, p_dst4, vprMask4 ); p_dst4 += srcStride;

			//odd line, 0~31
			vacc1_0 = (uint16)vswmpy5(v1_0, v1_1, v_coeff, SW_CONFIG(0, 0, 0, 0, 0, 0));
			vacc1_1 = (uint16)vswmpy5(v1_1, v1_2, v_coeff, SW_CONFIG(0, 0, 0, 0, 0, 0));

			vacc1_0 = vswmac5(accumulate, v2_0, v2_1, v_coeff, SW_CONFIG(0, 0, 0, 4, 0, 0), vacc1_0);
			vacc1_1 = vswmac5(accumulate, v2_1, v2_2, v_coeff, SW_CONFIG(0, 0, 0, 4, 0, 0), vacc1_1);

			result1_0 = ( ushort16 )vswmac5(psl, v3_0, v3_1, v_coeff, SW_CONFIG(6, 0, 0, 8, 0, 0), vacc1_0);
			result1_1 = ( ushort16 )vswmac5(psl, v3_1, v3_2, v_coeff, SW_CONFIG(6, 0, 0, 8, 0, 0), vacc1_1);

			vst( result1_0, p_dst1, vprMask1 ); p_dst1 += srcStride;
			vst( result1_1, p_dst2, vprMask2 ); p_dst2 += srcStride;

			//odd line, 32~63
			vacc1_0 = (uint16)vswmpy5(v1_2, v1_3, v_coeff, SW_CONFIG(0, 0, 0, 0, 0, 0));
			vacc1_1 = (uint16)vswmpy5(v1_3, v1_4, v_coeff, SW_CONFIG(0, 0, 0, 0, 0, 0));

			vacc1_0 = vswmac5(accumulate, v2_2, v2_3, v_coeff, SW_CONFIG(0, 0, 0, 4, 0, 0), vacc1_0);
			vacc1_1 = vswmac5(accumulate, v2_3, v2_4, v_coeff, SW_CONFIG(0, 0, 0, 4, 0, 0), vacc1_1);

			result1_0 = ( ushort16 )vswmac5(psl, v3_2, v3_3, v_coeff, SW_CONFIG(6, 0, 0, 8, 0, 0), vacc1_0);
			result1_1 = ( ushort16 )vswmac5(psl, v3_3, v3_4, v_coeff, SW_CONFIG(6, 0, 0, 8, 0, 0), vacc1_1);

			vst( result1_0, p_dst3, vprMask3 ); p_dst3 += srcStride;
			vst( result1_1, p_dst4, vprMask4 ); p_dst4 += srcStride;

			//next loop
			v0_0 = v2_0; v0_1 = v2_1; v0_2 = v2_2; v0_3 = v2_3; v0_4 = v2_4;
			v1_0 = v3_0; v1_1 = v3_1; v1_2 = v3_2; v1_3 = v3_3; v1_4 = v3_4;

			v2_0 = *( ushort16* )( p0 ); v2_1 = *( ushort16* )( p1 ); v2_2 = *( ushort16* )( p2 );
			v2_3 = *( ushort16* )( p3 ); v2_4 = *( ushort16* )( p4 );
			p0 += srcStride; p1 += srcStride; p2 += srcStride; p3 += srcStride; p4 += srcStride;

			v3_0 = *( ushort16* )( p0 ); v3_1 = *( ushort16* )( p1 ); v3_2 = *( ushort16* )( p2 );
			v3_3 = *( ushort16* )( p3 ); v3_4 = *( ushort16* )( p4 );
			p0 += srcStride; p1 += srcStride; p2 += srcStride; p3 += srcStride; p4 += srcStride;

		}
		for ( ; y < u32Rows; y ++ )
		{
			//even line, 0~31
			vacc0_0 = (uint16)vswmpy5(v0_0, v0_1, v_coeff, SW_CONFIG(0, 0, 0, 0, 0, 0));
			vacc0_1 = (uint16)vswmpy5(v0_1, v0_2, v_coeff, SW_CONFIG(0, 0, 0, 0, 0, 0));

			vacc0_0 = vswmac5(accumulate, v1_0, v1_1, v_coeff, SW_CONFIG(0, 0, 0, 4, 0, 0), vacc0_0);
			vacc0_1 = vswmac5(accumulate, v1_1, v1_2, v_coeff, SW_CONFIG(0, 0, 0, 4, 0, 0), vacc0_1);

			result0_0 = ( ushort16 )vswmac5(psl, v2_0, v2_1, v_coeff, SW_CONFIG(6, 0, 0, 8, 0, 0), vacc0_0);
			result0_1 = ( ushort16 )vswmac5(psl, v2_1, v2_2, v_coeff, SW_CONFIG(6, 0, 0, 8, 0, 0), vacc0_1);

			vst( result0_0, p_dst1, vprMask1 ); p_dst1 += srcStride;
			vst( result0_1, p_dst2, vprMask2 ); p_dst2 += srcStride;

			//even line, 32~63
			vacc0_0 = (uint16)vswmpy5(v0_2, v0_3, v_coeff, SW_CONFIG(0, 0, 0, 0, 0, 0));
			vacc0_1 = (uint16)vswmpy5(v0_3, v0_4, v_coeff, SW_CONFIG(0, 0, 0, 0, 0, 0));

			vacc0_0 = vswmac5(accumulate, v1_2, v1_3, v_coeff, SW_CONFIG(0, 0, 0, 4, 0, 0), vacc0_0);
			vacc0_1 = vswmac5(accumulate, v1_3, v1_4, v_coeff, SW_CONFIG(0, 0, 0, 4, 0, 0), vacc0_1);

			result0_0 = ( ushort16 )vswmac5(psl, v2_2, v2_3, v_coeff, SW_CONFIG(6, 0, 0, 8, 0, 0), vacc0_0);
			result0_1 = ( ushort16 )vswmac5(psl, v2_3, v2_4, v_coeff, SW_CONFIG(6, 0, 0, 8, 0, 0), vacc0_1);

			vst( result0_0, p_dst3, vprMask3 ); p_dst3 += srcStride;
			vst( result0_1, p_dst4, vprMask4 ); p_dst4 += srcStride;
		}
	}

}

CODE_MFNR_EX
/////////////////////////////////////////////////////////////////////
// add by shm @2016.08.31
// sum histogram of RK_S32 tpye( only used in vs )
//  v0
//  v1
//  v2
/////////////////////////////////////////////////////////////////////
void sum_histogram_vs( RK_S32* p_u16SrcB0, // <<! [ in ]: srcB0
					   RK_U32* p_u32Dst )  // <<! [ in ]: hist
{

	short i;
	int8 v0_lo, v0_hi, v1_lo, v1_hi;

	RK_S32 *p0;

	for ( i = 0; i < 4; i ++ )
	{
		p0 = p_u16SrcB0 + i * 32;
		vldchk( p0, v0_lo, v1_lo );
		p0 += 16;
		vldchk( p0, v0_hi, v1_hi );

		p_u32Dst[ 2 * i ] = vintrasum(v0_lo) + vintrasum(v0_hi);
		p_u32Dst[ 2 * i + 1 ] = vintrasum(v1_lo) + vintrasum(v1_hi);
	}

	p0 = p_u16SrcB0 + 4 * 32;
	vldchk( p0, v0_lo, v1_lo );
	p0 += 16;
	vldchk( p0, v0_hi, v1_hi );

	p_u32Dst[ 8 ] = vintrasum(v0_lo) + vintrasum(v0_hi);
}

CODE_MFNR_EX
/////////////////////////////////////////////////////////////////////
// add by shm @2016.08.31
// sum histogram of RK_S32 tpye( only used in ceva )
//  v0
//  v1
//  v2
/////////////////////////////////////////////////////////////////////
void sum_histogram_vec( RK_S32* p_u16SrcB0, // <<! [ in ]: srcB0
						RK_U32* p_u32Dst )  // <<! [ in ]: hist
{

	short i;
	int16 v0, v1;

	RK_S32 *p0;

	for ( i = 0; i < 4; i ++ )
	{
		p0 = p_u16SrcB0 + i * 32;
		//even line
		v0 = *( int16* )p0; p0 += 16;
		//odd line
		v1 = *( int16* )p0;

		p_u32Dst[ 2 * i ] = vintrasum( vunpack_lo( v0 ) ) + vintrasum( vunpack_hi( v0 ) );
		p_u32Dst[ 2 * i + 1 ] = vintrasum( vunpack_lo( v1 ) ) + vintrasum( vunpack_hi( v1 ) );
	}

	p0 = p_u16SrcB0 + 8 * 16;
	//even line
	v0 = *( int16* )p0;

	p_u32Dst[ 8 ] = vintrasum( vunpack_lo( v0 ) ) + vintrasum( vunpack_hi( v0 ) );
}

CODE_MFNR_EX
/////////////////////////////////////////////////////////////////////
// add by shm @2016.08.31
// sum histogram of short tpye
//  v0 v1 v0 v1 v0 v1
/////////////////////////////////////////////////////////////////////
void sum_histogram2( short* p_u16SrcB0,  // <<! [ in ]: srcB0
					 RK_U32* p_u32Dst )  // <<! [ in ]: hist
{

	short i;
	short16 v0, v1 = (short16) 0;
	short16 v2 = (short16)0;
	int8 vsum;
	RK_S32 sum;

	for (i=0;i<9;i++)
	{
		v0 = vpld(rel, p_u16SrcB0, v2);
		v2 = vadd(v2, (short)1); //pm
		vsum = (int8) vintrasum(v0, v1);
		sum = vintrasum(vsum);
		p_u32Dst[i] = sum;
	}
}

CODE_MFNR_EX
void ccv_memset( void *p_dst, // <<! [ in/out ]: memory need to memset
				 char val,    // <<! [ in ]: init value 
				 RK_S32 size) // <<! [ in ]: memory size(char RK_U32)
{
	RK_S32 i;
	char32 vval = (char32)val; 
	char32* pv_dst = (char32*)p_dst; 
	//char* pc_dst = (char*)((char*)p_dst + (RK_S32)(size & ~0x1f));
	char* pc_dst = (char*)((char*)p_dst);

	for (i = 0; i < size - 31; i += 32)
	{
		*pv_dst = vval; 
		pv_dst++; 

		pc_dst += 32; 
	}

	for (; i < size; i++)
	{
		pc_dst[i] = val;
	}

}


//caculate p_count hist and p_weight hist
CODE_MFNR_EX
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// add by shm @2016.08.31
// calculate p_count hist and p_weight hist
//  v0_0           v0_1
//  v1_0           v1_1
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CalcuHist_Vec( RK_U16 *p_src,       // <<![ in ]: data used to calculate hist
					RK_U16 *tmp,         // <<![ in ]: tmp pointer used to store hist( only correct when run in vs )
					RK_U16 *pcount,      // <<![ out ]: 32 * 32 data hist in 0 ~8
					RK_U32 *pweight,     // <<![ out ]: 32 * 32 data accumulate in 0 ~8
					RK_S32 u32Rows,      // <<![ in ]: row of data
					RK_S32 u32Cols,      // <<![ in ]: col of data
					RK_S32 srcStride,    // <<![ in ]: stride of data
					RK_S32 sw,           // <<![ in ]: num of 32 in width, sw = ceil( width / 32 ) + 1
					RK_S32 row )         // <<![ in ]: num of 32 * 522
{
	//idx of sw,0~15: 0, 16~47: 1, idx = (x + 16) >> 5;
	RK_U32 idx_pre[ 17 ] ={ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
	//pre of idy, idy = (y + 16) >> 5; idy = row + idy_pre;
	RK_U32 idy_pre[ 2 ] = { 0, 1 };

	//memory size used to hist, must be aligned to 32 byte 
	RK_S32 size = 5 * 32;

	//mask
	RK_U16 vprMaskLastCol1 = -1;
	RK_U16 vprMaskLastCol2 = -1;

	if( ( u32Cols - 16 ) & 31 )
	{
		RK_U32 bits = ( u32Cols - 16 ) & 31;

		if ( bits < 16 ) 
		{
			vprMaskLastCol1 = ( 1 << bits ) - 1;
			vprMaskLastCol2 = 0;
		}
		else
		{
			vprMaskLastCol2 = ( 1 << ( bits - 16 ) ) - 1;
		}
	}

	//outloop of x: from col 31, each block is 32 * 32
	RK_U32 outLoopX = ( u32Cols - 16 + 31 ) >> 5;
	//outloop of y: row of src is 0~31, need to divided into two parts, 0~15, 16~31
	RK_U32 outLoopY = ( u32Rows + 15 ) >> 4;
	//for the src that doesn't have 16 rows;
	RK_U32 u32BottomRes = 16;
	if( u32Rows & 15 )
	{
		u32BottomRes = u32Rows & 15;
	}
	RK_U32 u32Bottom = 16;

	short16 v0_0, v0_1, v1_0, v1_1;
	short16 index0_0, index0_1,index1_0, index1_1;

	RK_U16 *p_srcTmp, *p0, *p1;

#if CEVA_XM4_DEBUG == 0

	//note:some part of the tmp may be used later
	RK_S16* p_u16dstB0 = ( RK_S16* )( tmp + 3 * srcStride + 10 );
	RK_S32* p_u16dstB1 = ( RK_S32* )( tmp + 3 * srcStride + 330 );

#else

	//Block0, aligned to 32 byte address
	RK_U32 a = ( RK_U32 )( u16dstB0 );
	a = ( ( ( a + 31 ) >> 5 ) << 5 ) - a;
	RK_S16 *p_u16dstB0 MEM_BLOCK(0) = u16dstB0 + ( a >> 1 );

	//Block2
	RK_U32 b = ( RK_U32 )( u16dstB1 );
	b = ( ( ( b + 31 ) >> 5 ) << 5 ) - b;
	RK_S32	*p_u16dstB1 MEM_BLOCK(1) = u16dstB1 + ( b >> 2 );

#endif

	//init p_count and p_weight
	RK_U32 p_count[ 9 ] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	RK_U32 p_weight[ 9 ] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	RK_U32 idx, idy;

	RK_U32 y = 0;
	for( RK_U32 ky = 0; ky < outLoopY; ky ++ )
	{
		idy = row + idy_pre[ ky ];
		p_srcTmp = p_src + ky * 16 * srcStride;
		//col£º0~15
		if( ky == outLoopY - 1 )
		{
			u32Bottom = u32BottomRes;
		}
		p0 = p_srcTmp;

		//init hist to 0
		ccv_memset( p_u16dstB0, 0, size * 2 );
		ccv_memset( p_u16dstB1, 0, size * 4 );

		y = 0;
		for( ; y < u32Bottom - 1; y += 2 )
		{
			v0_0 = ( short16 )*( ushort16* )p0; p0 += srcStride;
			index0_0 = vshiftr( v0_0 + 1024, RK_U8( 11 ) );
			v1_0 = ( short16 )*( ushort16* )p0; p0 += srcStride;
			index1_0 = vshiftr( v1_0 + 1024, RK_U8( 11 ) );

			//even line,hist of p_count
			vhist( ( short* )p_u16dstB0, index0_0, RK_U16( - 1 ) );
			//even line,hist of p_weight
			vhist( v0_0, ( RK_S32* )p_u16dstB1, index0_0, RK_U16( - 1 ) );

			//odd line,hist of p_count
			vhist( ( short* )p_u16dstB0, index1_0, RK_U16( - 1 ) );
			//odd line,hist of p_weight
			vhist( v1_0, ( RK_S32* )p_u16dstB1, index1_0, RK_U16( - 1 ) );
		}
		for( ; y < u32Bottom; y ++ )
		{
			v0_0 = ( short16 )*( ushort16* )p0; p0 += srcStride;
			index0_0 = vshiftr( v0_0 + 1024, RK_U8( 11 ) );

			//even line,hist of p_count
			vhist( ( short* )p_u16dstB0, index0_0, RK_U16( - 1 ) );
			//even line,hist of p_weight
			vhist( v0_0, ( RK_S32* )p_u16dstB1, index0_0, RK_U16( - 1 ) );
		}
		idx = idx_pre[ 0 ];

		//sum histogram of p_count
		sum_histogram2( p_u16dstB0, p_count );

		//sum histogram of p_weight
#if CEVA_XM4_DEBUG == 0
		sum_histogram_vs( p_u16dstB1, p_weight );
#else
		sum_histogram_vec( p_u16dstB1, p_weight );
#endif
		//store into pcount( 9 * 256 ) 
		for( RK_U32 kk = 0; kk < 9; kk ++ )
		{
			pcount[ kk * 256 + idy * sw + idx ] += RK_U16( p_count[ kk ] );

			pweight[ kk * 256 + idy * sw + idx ] += p_weight[ kk ];
		}
		//col£º16 + kx * 32
		RK_U16 vprMask1 = -1;
		RK_U16 vprMask2 = -1;
		for( RK_U32 kx = 0; kx < outLoopX; kx ++ )
		{
			if( kx == outLoopX - 1 )
			{
				vprMask1 = vprMaskLastCol1;
				vprMask2 = vprMaskLastCol2;
			}
			p0 = p_srcTmp + 16 + kx * 32; p1 = p0 + 16;

			//init hist to 0
			ccv_memset( p_u16dstB0, 0, size * 2 );
			ccv_memset( p_u16dstB1, 0, size * 4 );

			y = 0;
			for( ; y < u32Bottom - 1; y += 2 )
			{
				//even line
				v0_0 = ( short16 )*( ushort16* )p0; p0 += srcStride;
				index0_0 = vshiftr( v0_0 + 1024, RK_U8( 11 ) );
				v0_1 = ( short16 )*( ushort16* )p1; p1 += srcStride;
				index0_1 = vshiftr( v0_1 + 1024, RK_U8( 11 ) );

				//odd line
				v1_0 = ( short16 )*( ushort16* )p0; p0 += srcStride;
				index1_0 = vshiftr( v1_0 + 1024, RK_U8( 11 ) );
				v1_1 = ( short16 )*( ushort16* )p1; p1 += srcStride;
				index1_1 = vshiftr( v1_1 + 1024, RK_U8( 11 ) );

				//even line,first 16 cols, hist of p_count
				vhist( ( short* )p_u16dstB0, index0_0, vprMask1 );
				//even line,first 16 cols, hist of p_weight
				vhist( v0_0, ( RK_S32* )p_u16dstB1, index0_0, vprMask1 );	

				//even line,second 16 cols, hist of p_count
				vhist( ( short* )p_u16dstB0, index0_1, vprMask2 );
				//even line,second 16 cols, hist of p_weight
				vhist( v0_1, ( RK_S32* )p_u16dstB1, index0_1, vprMask2 );

				//odd line
				vhist( ( short* )p_u16dstB0, index1_0, vprMask1 );
				vhist( v1_0, ( RK_S32* )p_u16dstB1, index1_0, vprMask1 );

				vhist( ( short* )p_u16dstB0, index1_1, vprMask2 );
				vhist( v1_1, ( RK_S32* )p_u16dstB1, index1_1, vprMask2 );
			}
			for( ; y < u32Bottom; y ++ )
			{
				v0_0 = ( short16 )*( ushort16* )p0; p0 += srcStride;
				index0_0 = vshiftr( v0_0 + 1024, RK_U8( 11 ) );

				v0_1 = ( short16 )*( ushort16* )p1; p1 += srcStride;
				index0_1 = vshiftr( v0_1 + 1024, RK_U8( 11 ) );

				//even line
				vhist( ( short* )p_u16dstB0, index0_0, vprMask1 );
				vhist( v0_0, ( RK_S32* )p_u16dstB1, index0_0, vprMask1 );

				vhist( ( short* )p_u16dstB0, index0_1, vprMask2 );
				vhist( v0_1, ( RK_S32* )p_u16dstB1, index0_1, vprMask2 );
			}

			idx = idx_pre[ kx + 1 ];
			sum_histogram2( p_u16dstB0, p_count );

#if CEVA_XM4_DEBUG == 0
			sum_histogram_vs( p_u16dstB1,  p_weight );
#else
			sum_histogram_vec( p_u16dstB1, p_weight );
#endif

			for( RK_U32 kk = 0; kk < 9; kk ++ )
			{
				pcount[ kk * 256 + idy * sw + idx ] += RK_U16( p_count[ kk ] );
				pweight[ kk * 256 + idy * sw + idx ] += p_weight[ kk ];
			}
		}
	}
}

CODE_MFNR_EX
//////////////////////////////////////////////////////////////////////////////////////////////////
// add by shm @2016.08.31
// three-dimensional filter for pcount
//  1 2 1   v0_0           v0_1
//  2 4 2   v1_0           v1_1
//  1 2 1   v2_0           v2_1
//          v3_0           v3_1
//third dimensional filter: 1 2 1
//////////////////////////////////////////////////////////////////////////////////////////////////
void countFilter_Vec( RK_U16 *p_src,     // <<! [ in/out ]: pcount need to filter
					  RK_S32 sh,         // <<! [ in ]: sh of pcount: ( ( mRawHgt + 128 ) >> 8 ) + 1	
					  RK_S32 sw,         // <<! [ in ]: sh of pcount: ( ( mRawWid + 128 ) >> 8 ) + 1
					  RK_S32 srcStride ) // <<! [ in ]: stride of pcount: 256
{
	RK_U16 vprMask1 = -1;
	RK_U16 vprMask2 = -1;

	RK_U16 vprMaskLastCol1 = -1;
	RK_U16 vprMaskLastCol2 = -1;

	if( sw & 31 )
	{
		RK_U32 bits = sw & 31;

		if ( bits < 16 ) 
		{
			vprMaskLastCol1 = ( 1 << bits ) - 1;
			vprMaskLastCol2 = 0;
		}
		else
		{
			vprMaskLastCol2 = ( 1 << ( bits - 16 ) ) - 1;
		}
	}

	ushort16 v0_0, v0_1;
	ushort16 v1_0, v1_1;
	ushort16 v2_0, v2_1;
	ushort16 v3_0, v3_1;
	ushort16 dummy = ( ushort16 )( 0 );
	uint16 vacc0_0, vacc0_1, vacc1_0, vacc1_1;
	ushort16 result0_0, result0_1, result1_0, result1_1;

	RK_U8 coeff[32] = { 1, 2, 1, 0, 2, 4, 2, 0, 1, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	uchar32 v_coeff = *(uchar32*)coeff;

	RK_S32 outLoopX = ( sw + 31 ) >> 5;

	RK_U16 *p0, *p1;
	RK_U16 *p_srcTmp;
	RK_U16 *p_dst1, *p_dst2;
	RK_U16 *p_dstTmp;

	//one loop: 2 * 32
	for( RK_S32 i = 0; i < 9; i ++ )
	{
		p_srcTmp = p_src + i * srcStride - 1;
		p_dstTmp = p_src + i * srcStride;

		for( RK_S32 x = 0; x < outLoopX; x ++ )
		{
			if( x == outLoopX - 1 )
			{
				vprMask1 = vprMaskLastCol1;
				vprMask2 = vprMaskLastCol2;
			}
			p0 = p_srcTmp + x * sw; p1 = p0 + 16;
			p_dst1 = p_dstTmp + x * sw; p_dst2 = p_dst1 + 16;

			v0_0 = dummy; v0_1 = dummy; 
			v1_0 = *( ushort16* )( p0 ); v1_1 = *( ushort16* )( p1 ); p0 += sw; p1 += sw;
			v2_0 = *( ushort16* )( p0 ); v2_1 = *( ushort16* )( p1 ); p0 += sw; p1 += sw;
			v3_0 = *( ushort16* )( p0 ); v3_1 = *( ushort16* )( p1 ); p0 += sw; p1 += sw;

			RK_S32 y = 0;
			for ( ; y < sh - 1; y += 2 )
			{
				//even line, 0~31
				vacc0_0 = (uint16)vswmpy5(v0_0, v0_1, v_coeff, SW_CONFIG(0, 0, 0, 0, 0, 0));
				vacc0_1 = (uint16)vswmpy5(v0_1, v0_1, v_coeff, SW_CONFIG(0, 0, 0, 0, 0, 0));

				vacc0_0 = vswmac5(accumulate, v1_0, v1_1, v_coeff, SW_CONFIG(0, 0, 0, 4, 0, 0), vacc0_0);
				vacc0_1 = vswmac5(accumulate, v1_1, v1_1, v_coeff, SW_CONFIG(0, 0, 0, 4, 0, 0), vacc0_1);

				result0_0 = ( ushort16 )vswmac5(accumulate, v2_0, v2_1, v_coeff, SW_CONFIG(0, 0, 0, 8, 0, 0), vacc0_0);
				result0_1 = ( ushort16 )vswmac5(accumulate, v2_1, v2_1, v_coeff, SW_CONFIG(0, 0, 0, 8, 0, 0), vacc0_1);

				vst( result0_0, p_dst1, vprMask1 ); p_dst1 += sw;
				vst( result0_1, p_dst2, vprMask2 ); p_dst2 += sw;

				//odd line, 0~31
				vacc1_0 = (uint16)vswmpy5(v1_0, v1_1, v_coeff, SW_CONFIG(0, 0, 0, 0, 0, 0));
				vacc1_1 = (uint16)vswmpy5(v1_1, v1_1, v_coeff, SW_CONFIG(0, 0, 0, 0, 0, 0));

				vacc1_0 = vswmac5(accumulate, v2_0, v2_1, v_coeff, SW_CONFIG(0, 0, 0, 4, 0, 0), vacc1_0);
				vacc1_1 = vswmac5(accumulate, v2_1, v2_1, v_coeff, SW_CONFIG(0, 0, 0, 4, 0, 0), vacc1_1);

				result1_0 = ( ushort16 )vswmac5(accumulate, v3_0, v3_1, v_coeff, SW_CONFIG(0, 0, 0, 8, 0, 0), vacc1_0);
				result1_1 = ( ushort16 )vswmac5(accumulate, v3_1, v3_1, v_coeff, SW_CONFIG(0, 0, 0, 8, 0, 0), vacc1_1);

				vst( result1_0, p_dst1, vprMask1 ); p_dst1 += sw;
				vst( result1_1, p_dst2, vprMask2 ); p_dst2 += sw;

				//next loop
				v0_0 = v2_0; v0_1 = v2_1;
				v1_0 = v3_0; v1_1 = v3_1;

				v2_0 = *( ushort16* )( p0 ); v2_1 = *( ushort16* )( p1 ); p0 += sw; p1 += sw;
				v3_0 = *( ushort16* )( p0 ); v3_1 = *( ushort16* )( p1 ); p0 += sw; p1 += sw;
			}
			for ( ; y < sh; y ++ )
			{
				//even line, 0~31
				vacc0_0 = (uint16)vswmpy5(v0_0, v0_1, v_coeff, SW_CONFIG(0, 0, 0, 0, 0, 0));
				vacc0_1 = (uint16)vswmpy5(v0_1, v0_1, v_coeff, SW_CONFIG(0, 0, 0, 0, 0, 0));

				vacc0_0 = vswmac5(accumulate, v1_0, v1_1, v_coeff, SW_CONFIG(0, 0, 0, 4, 0, 0), vacc0_0);
				vacc0_1 = vswmac5(accumulate, v1_1, v1_1, v_coeff, SW_CONFIG(0, 0, 0, 4, 0, 0), vacc0_1);

				result0_0 = ( ushort16 )vswmac5(accumulate, v2_0, v2_1, v_coeff, SW_CONFIG(0, 0, 0, 8, 0, 0), vacc0_0);
				result0_1 = ( ushort16 )vswmac5(accumulate, v2_1, v2_1, v_coeff, SW_CONFIG(0, 0, 0, 8, 0, 0), vacc0_1);

				vst( result0_0, p_dst1, vprMask1 ); p_dst1 += sw;
				vst( result0_1, p_dst2, vprMask2 ); p_dst2 += sw;
			}
		}
	}	

	//third dimensional filter: 1 2 1
	vprMask1 = -1;
	vprMask2 = -1;
	vprMaskLastCol1 = -1;
	vprMaskLastCol2 = -1;
	if( ( sw * sh ) & 31 )
	{
		RK_U32 bits = ( sw * sh ) & 31;

		if ( bits < 16 ) 
		{
			vprMaskLastCol1 = ( 1 << bits ) - 1;
			vprMaskLastCol2 = 0;
		}
		else
		{
			vprMaskLastCol2 = ( 1 << ( bits - 16 ) ) - 1;
		}
	}
	//one loop: 2 * 32
	outLoopX = ( sw * sh + 31 ) >> 5;
	for( RK_S32 x = 0; x < outLoopX; x ++ )
	{
		if( x == outLoopX - 1 )
		{
			vprMask1 = vprMaskLastCol1;
			vprMask2 = vprMaskLastCol2;
		}

		p0 = p_src + x * 32; p1 = p0 + 16;
		p_dst1 = p_src + x * 32; p_dst2 = p_dst1 + 16;

		v0_0 = dummy; v0_1 = dummy; 
		v1_0 = *( ushort16* )( p0 ); v1_1 = *( ushort16* )( p1 ); p0 += srcStride; p1 += srcStride;
		v2_0 = *( ushort16* )( p0 ); v2_1 = *( ushort16* )( p1 ); p0 += srcStride; p1 += srcStride;
		v3_0 = *( ushort16* )( p0 ); v3_1 = *( ushort16* )( p1 ); p0 += srcStride; p1 += srcStride;

		RK_S32 y = 0;
		for( ; y < 9 - 1; y += 2 )
		{
			//even line, 0~31
			result0_0 = v0_0 + 2 * v1_0 + v2_0;
			result0_1 = v0_1 + 2 * v1_1 + v2_1;

			vst( result0_0, p_dst1, vprMask1 ); p_dst1 += srcStride;
			vst( result0_1, p_dst2, vprMask2 ); p_dst2 += srcStride;

			//odd line, 0~31
			result1_0 = v1_0 + 2 * v2_0 + v3_0;
			result1_1 = v1_1 + 2 * v2_1 + v3_1;

			vst( result1_0, p_dst1, vprMask1 ); p_dst1 += srcStride;
			vst( result1_1, p_dst2, vprMask2 ); p_dst2 += srcStride;

			//next loop
			v0_0 = v2_0; v0_1 = v2_1;
			v1_0 = v3_0; v1_1 = v3_1;

			v2_0 = *( ushort16* )( p0 ); v2_1 = *( ushort16* )( p1 ); p0 += srcStride; p1 += srcStride;
			v3_0 = *( ushort16* )( p0 ); v3_1 = *( ushort16* )( p1 ); p0 += srcStride; p1 += srcStride;
		}
		for ( ; y < 9; y ++ )
		{
			//even line, 0~31
			result0_0 = v0_0 + 2 * v1_0;
			result0_1 = v0_1 + 2 * v1_1;

			vst( result0_0, p_dst1, vprMask1 ); p_dst1 += srcStride;
			vst( result0_1, p_dst2, vprMask2 ); p_dst2 += srcStride;
		}
	}

}

CODE_MFNR_EX
/////////////////////////////////////////////////////////////////////////////////////////////////////
// add by shm @2016.08.31
// three-dimensional filter for pweight
//  1/4  1/16  1/4   v0_0           v0_1
//  1/2  1/8   1/2   v1_0           v1_1
//  1/4  1/16  1/4   v2_0           v2_1
//                   v3_0           v3_1
//third dimensional filter: 1/2 1/4 1/2
/////////////////////////////////////////////////////////////////////////////////////////////////////
void weightFilter_Vec( RK_U32 *p_src,     // <<! [ in/out ]: pweight
					   RK_S32 sh,         // <<! [ in ]: sh of pcount: ( ( mRawHgt + 128 ) >> 8 ) + 1
					   RK_S32 sw,         // <<! [ in ]: sw of pcount: ( ( mRawWgt + 128 ) >> 8 ) + 1
					   RK_S32 srcStride ) // <<! [in ]: src stride: 256
{
	RK_U8 vprMask1 = -1;
	RK_U8 vprMask2 = -1;
	RK_U8 vprMask3 = -1;

	RK_U8 vprMaskLastCol1 = -1;
	RK_U8 vprMaskLastCol2 = -1;
	RK_U8 vprMaskLastCol3 = -1;

	//sw = 17
	if( sw & 31 )
	{
		RK_U32 bits = sw & 31;

		if ( bits < 8 ) 
		{
			vprMaskLastCol1 = ( 1 << bits ) - 1;
			vprMaskLastCol2 = 0;
			vprMaskLastCol3 = 0;
		}
		else if ( bits >= 8 && bits < 16 ) 
		{
			vprMaskLastCol2 = ( 1 << ( bits - 8 ) ) - 1;
			vprMaskLastCol3 = 0;
		}
		else if ( bits >= 16 && bits < 24 ) 
		{
			vprMaskLastCol3 = ( 1 << ( bits - 16 ) ) - 1;
		}
	}

	uint8 v0_0, v0_1, v0_2, v0_3, v0_4, v0_5, v0_6, v0_7, v0_8;
	uint8 v1_0, v1_1, v1_2, v1_3, v1_4, v1_5, v1_6, v1_7, v1_8;
	uint8 v2_0, v2_1, v2_2, v2_3, v2_4, v2_5, v2_6, v2_7, v2_8;
	uint8 v3_0, v3_1, v3_2, v3_3;
	uint8 dummy = ( uint8 )( 0 );

	uint8 result0_0, result0_1, result0_2, result0_3;
	uint8 result1_0, result1_1, result1_2, result1_3;
	uint8 result2_0, result2_1, result2_2;

	//sw = 17
	RK_S32 outLoopX = ( sw + 23 ) / 24;

	RK_U32 *p0, *p1, *p2, *p3;
	RK_U32 *p_srcTmp;
	RK_U32 *p_dst1, *p_dst2, *p_dst3, *p_dst4;
	RK_U32 *p_dstTmp;

	//one loop: 2 * 32
	for( RK_S32 i = 0; i < 9; i ++ )
	{
		p_srcTmp = p_src + i * srcStride - 1;
		p_dstTmp = p_src + i * srcStride;

		for( RK_S32 x = 0; x < outLoopX; x ++ )
		{
			if( x == outLoopX - 1 )
			{
				vprMask1 = vprMaskLastCol1;
				vprMask2 = vprMaskLastCol2;
				vprMask3 = vprMaskLastCol3;
			}
			p0 = p_srcTmp + x * sw; p1 = p0 + 8; p2 = p1 + 8; p3 = p2 + 8;
			p_dst1 = p_dstTmp + x * sw; p_dst2 = p_dst1 + 8; p_dst3 = p_dst2 + 8;


			//line 0
			v0_0 = ( uint8 )( 0 ); v0_1 = ( uint8 )( 0 ); v0_2 = ( uint8 )( 0 ); 
			v0_3 = ( uint8 )( 0 ); v0_4 = ( uint8 )( 0 ); v0_5 = ( uint8 )( 0 );
			v0_6 = ( uint8 )( 0 ); v0_7 = ( uint8 )( 0 ); v0_8 = ( uint8 )( 0 );

			//line 1
			vldov( p0, v1_0, v1_1, v1_2, dummy ); p0 += sw;
			vldov( p1, v1_3, v1_4, v1_5, dummy ); p1 += sw;
			vldov( p2, v1_6, v1_7, v1_8, dummy ); p2 += sw;

			//line 2
			vldov( p0, v2_0, v2_1, v2_2, dummy ); p0 += sw;
			vldov( p1, v2_3, v2_4, v2_5, dummy ); p1 += sw;
			vldov( p2, v2_6, v2_7, v2_8, dummy ); p2 += sw;

			for ( RK_S32 y = 0; y < sh; y ++ )
			{
				//col 0~7
				result0_0 = v0_0 + 2 * v0_1 + v0_2;
				result0_1 = v0_3 + 2 * v0_4 + v0_5;
				result0_2 = v0_6 + 2 * v0_7 + v0_8;

				//col 8~15
				result1_0 = v1_0 + 2 * v1_1 + v1_2;
				result1_1 = v1_3 + 2 * v1_4 + v1_5;
				result1_2 = v1_6 + 2 * v1_7 + v1_8;

				//col 16~sw
				result2_0 = v2_0 + 2 * v2_1 + v2_2;
				result2_1 = v2_3 + 2 * v2_4 + v2_5;
				result2_2 = v2_6 + 2 * v2_7 + v2_8;

				result0_0 = result0_0 + 2 * result1_0 + result2_0;
				result0_1 = result0_1 + 2 * result1_1 + result2_1;
				result0_2 = result0_2 + 2 * result1_2 + result2_2;

				vst( result0_0 >> 4, p_dst1, vprMask1 ); p_dst1 += sw;
				vst( result0_1 >> 4, p_dst2, vprMask2 ); p_dst2 += sw;
				vst( result0_2 >> 4, p_dst3, vprMask3 ); p_dst3 += sw;

				//next loop
				v0_0 = v1_0; v0_1 = v1_1; v0_2 = v1_2; v0_3 = v1_3;
				v0_4 = v1_4; v0_5 = v1_5; v0_6 = v1_6; v0_7 = v1_7; v0_8 = v1_8;

				v1_0 = v2_0; v1_1 = v2_1; v1_2 = v2_2; v1_3 = v2_3;
				v1_4 = v2_4; v1_5 = v2_5; v1_6 = v2_6; v1_7 = v2_7; v1_8 = v2_8;

				//line 2
				vldov( p0, v2_0, v2_1, v2_2, dummy ); p0 += sw;
				vldov( p1, v2_3, v2_4, v2_5, dummy ); p1 += sw;
				vldov( p2, v2_6, v2_7, v2_8, dummy ); p2 += sw;
			}

		}
	}	

	//third dimension, 1/2, 1/4, 1/2
	vprMask1 = -1;
	vprMask2 = -1;
	vprMask3 = -1;
	RK_U8 vprMask4 = -1;

	vprMaskLastCol1 = -1;
	vprMaskLastCol2 = -1;
	vprMaskLastCol3 = -1;
	RK_U8 vprMaskLastCol4 = -1;

	if( ( sw * sh ) & 31 )
	{
		RK_U32 bits = ( sw * sh ) & 31;

		if ( bits < 8 ) 
		{
			vprMaskLastCol1 = ( 1 << bits ) - 1;
			vprMaskLastCol2 = 0;
			vprMaskLastCol3 = 0;
			vprMaskLastCol4 = 0;
		}
		else if ( bits >= 8 && bits < 16 ) 
		{
			vprMaskLastCol2 = ( 1 << ( bits - 8 ) ) - 1;
			vprMaskLastCol3 = 0;
			vprMaskLastCol4 = 0;
		}
		else if ( bits >= 16 && bits < 24 ) 
		{
			vprMaskLastCol3 = ( 1 << ( bits - 16 ) ) - 1;
			vprMaskLastCol4 = 0;
		}
		else
		{
			vprMaskLastCol4 = ( 1 << ( bits - 24 ) ) - 1;
		}
	}

	//one loop: 2 * 32
	outLoopX = ( sw * sh + 31 ) >> 5;
	for( RK_S32 x = 0; x < outLoopX; x ++ )
	{
		if( x == outLoopX - 1 )
		{
			vprMask1 = vprMaskLastCol1;
			vprMask2 = vprMaskLastCol2;
			vprMask3 = vprMaskLastCol3;
			vprMask4 = vprMaskLastCol4;
		}

		p0 = p_src + x * 32; p1 = p0 + 8; p2 = p1 + 8; p3 = p2 + 8;
		p_dst1 = p_src + x * 32; p_dst2 = p_dst1 + 8; p_dst3 = p_dst2 + 8;  p_dst4 = p_dst3 + 8;

		//line 0,col 0~31
		v0_0 = ( uint8 )( 0 ); v0_1 = ( uint8 )( 0 ); v0_2 = ( uint8 )( 0 ); v0_3 = ( uint8 )( 0 );

		//line 1,col 0~31
		v1_0 = *( uint8* )( p0 ); v1_1 = *( uint8* )( p1 ); p0 += srcStride; p1 += srcStride;
		v1_2 = *( uint8* )( p2 ); v1_3 = *( uint8* )( p3 ); p2 += srcStride; p3 += srcStride;

		//line 2,col 0~31
		v2_0 = *( uint8* )( p0 ); v2_1 = *( uint8* )( p1 ); p0 += srcStride; p1 += srcStride;
		v2_2 = *( uint8* )( p2 ); v2_3 = *( uint8* )( p3 ); p2 += srcStride; p3 += srcStride;

		//line 3,col 0~31
		v3_0 = *( uint8* )( p0 ); v3_1 = *( uint8* )( p1 ); p0 += srcStride; p1 += srcStride;
		v3_2 = *( uint8* )( p2 ); v3_3 = *( uint8* )( p3 ); p2 += srcStride; p3 += srcStride;

		RK_S32 y = 0;
		for( ; y < 9 - 1; y += 2 )
		{
			//even line, 0~31
			result0_0 = v0_0 + 2 * v1_0 + v2_0;
			result0_1 = v0_1 + 2 * v1_1 + v2_1;
			result0_2 = v0_2 + 2 * v1_2 + v2_2;
			result0_3 = v0_3 + 2 * v1_3 + v2_3;

			vst( result0_0 >> 2, p_dst1, vprMask1 ); p_dst1 += srcStride;
			vst( result0_1 >> 2, p_dst2, vprMask2 ); p_dst2 += srcStride;
			vst( result0_2 >> 2, p_dst3, vprMask3 ); p_dst3 += srcStride;
			vst( result0_3 >> 2, p_dst4, vprMask4 ); p_dst4 += srcStride;

			//odd line, 0~31
			result1_0 = v1_0 + 2 * v2_0 + v3_0;
			result1_1 = v1_1 + 2 * v2_1 + v3_1;
			result1_2 = v1_2 + 2 * v2_2 + v3_2;
			result1_3 = v1_3 + 2 * v2_3 + v3_3;

			vst( result1_0 >> 2, p_dst1, vprMask1 ); p_dst1 += srcStride;
			vst( result1_1 >> 2, p_dst2, vprMask2 ); p_dst2 += srcStride;
			vst( result1_2 >> 2, p_dst3, vprMask3 ); p_dst3 += srcStride;
			vst( result1_3 >> 2, p_dst4, vprMask4 ); p_dst4 += srcStride;

			//next loop
			v0_0 = v2_0; v0_1 = v2_1; v0_2 = v2_2; v0_3 = v2_3;
			v1_0 = v3_0; v1_1 = v3_1; v1_2 = v3_2; v1_3 = v3_3;

			v2_0 = *( uint8* )( p0 ); v2_1 = *( uint8* )( p1 ); p0 += srcStride; p1 += srcStride;
			v2_2 = *( uint8* )( p2 ); v2_3 = *( uint8* )( p3 ); p2 += srcStride; p3 += srcStride;

			v3_0 = *( uint8* )( p0 ); v3_1 = *( uint8* )( p1 ); p0 += srcStride; p1 += srcStride;
			v3_2 = *( uint8* )( p2 ); v3_3 = *( uint8* )( p3 ); p2 += srcStride; p3 += srcStride;

		}
		for ( ; y < 9; y ++ )
		{
			//even line, 0~31
			result0_0 = v0_0 + 2 * v1_0;
			result0_1 = v0_1 + 2 * v1_1;
			result0_2 = v0_2 + 2 * v1_2;
			result0_3 = v0_3 + 2 * v1_3;

			vst( result0_0 >> 2, p_dst1, vprMask1 ); p_dst1 += srcStride;
			vst( result0_1 >> 2, p_dst2, vprMask2 ); p_dst2 += srcStride;
			vst( result0_2 >> 2, p_dst3, vprMask3 ); p_dst3 += srcStride;
			vst( result0_3 >> 2, p_dst4, vprMask4 ); p_dst4 += srcStride;
		}
	}
}

CODE_MFNR_EX
/////////////////////////////////////////////////////////////////////////////////////////////////////
// add by shm @2016.08.31
// normalize pweight: pweight / pcount
//     count0           count1
/////////////////////////////////////////////////////////////////////////////////////////////////////
void normalizeWeight_Vec( RK_U16 *pcount,     // <<! [ in/out ]: pcount
						  RK_U32 *pweight,    // <<! [ in ]: pweight
						  RK_S32 sh,          // <<! [ in ]: sh of pcount: ( ( mRawHgt + 128 ) >> 8 ) + 1
						  RK_S32 sw,          // <<! [ in ]: sw of pcount: ( ( mRawWgt + 128 ) >> 8 ) + 1
						  RK_S32 srcStride )  // <<! [in ]: src stride: 256
{
	RK_U16 *p_count0, *p_count1;
	RK_U32 *p_weight0, *p_weight1;
	RK_U16 *p_dst0, *p_dst1;

	uint16 weight0, weight1;
	ushort16 count0, count1;

	uint8 weight0_lo, weight0_hi;
	uint8 weight1_lo, weight1_hi;

	ushort16 count0_mantissa, count1_mantissa;
	short16 count0_shiftdiv, count1_shiftdiv;

	uint8 count0_mantissa_lo, count0_mantissa_hi, count0_shiftdiv_lo, count0_shiftdiv_hi;
	uint8 count1_mantissa_lo, count1_mantissa_hi, count1_shiftdiv_lo, count1_shiftdiv_hi;

	int8 result0_lo, result0_hi, result1_lo, result1_hi;

	ushort16 result0, result1;

	//valid size
	RK_S32 cols = sh * sw;
	RK_U32 outLoop = ( cols + 31 ) >> 5;

	RK_U16 vprMask1 = -1;
	RK_U16 vprMask2 = -1;

	RK_U16 vprMaskLastCol1 = -1;
	RK_U16 vprMaskLastCol2 = -1;

	if( cols & 31 )
	{
		RK_U32 bits = cols & 31;

		if ( bits < 16 ) 
		{
			vprMaskLastCol1 = ( 1 << bits ) - 1;
			vprMaskLastCol2 = 0;
		}
		else
		{
			vprMaskLastCol2 = ( 1 << ( bits - 16 ) ) - 1;
		}
	}
	//0~8
	for( RK_U32 y = 0; y < 9; y ++ )
	{
		p_count0 = pcount + y * srcStride; p_count1 = p_count0 + 16;
		p_weight0 = pweight + y * srcStride; p_weight1 = p_weight0 + 16;
		p_dst0 = pcount + y * srcStride; p_dst1 = p_dst0 + 16;

		for( RK_U32 x = 0; x < outLoop; x ++ )
		{
			vprMask1 = -1;
			vprMask2 = -1;
			if( x == outLoop - 1 )
			{
				vprMask1 = vprMaskLastCol1;
				vprMask2 = vprMaskLastCol2;
			}

			count0 = *( ushort16* )( p_count0 ); count1 = *( ushort16* )( p_count1 ); 
			p_count0 += 32; p_count1 += 32;

			weight0 = *( uint16* )( p_weight0 ); weight1 = *( uint16* )( p_weight1 );
			p_weight0 += 32; p_weight1 += 32;

			vinv( absexp, count0, count0_mantissa, count0_shiftdiv );
			vinv( absexp, count1, count1_mantissa, count1_shiftdiv );

			count0_shiftdiv += 16; count1_shiftdiv += 16;

			//count > 256, shiftdiv = shiftdiv - 8
			count0_shiftdiv = vcmpmov( lt, count0, RK_U16( 256 ), count0_shiftdiv, count0_shiftdiv - 8 );
			count1_shiftdiv = vcmpmov( lt, count1, RK_U16( 256 ), count1_shiftdiv, count1_shiftdiv - 8 );

			//count > 256, weight >> 8
			weight0_lo = vunpack_lo( weight0 ); weight1_lo = vunpack_lo( weight1 );
			weight0_hi = vunpack_hi( weight0 ); weight1_hi = vunpack_hi( weight1 );

			weight0_lo = uint8( vcmpmov( lt, ( uint8 )vunpack_lo( count0 ), RK_U32( 256 ), weight0_lo, weight0_lo >> 8 ) );
			weight0_hi = uint8( vcmpmov( lt, ( uint8 )vunpack_hi( count0 ), RK_U32( 256 ), weight0_hi, weight0_hi >> 8 ) );

			weight1_lo = uint8( vcmpmov( lt, ( uint8 )vunpack_lo( count1 ), RK_U32( 256 ), weight1_lo, weight1_lo >> 8 ) );
			weight1_hi = uint8( vcmpmov( lt, ( uint8 )vunpack_hi( count1 ), RK_U32( 256 ), weight1_hi, weight1_hi >> 8 ) );

			//multiply
			count0_mantissa_lo = ( uint8 )vunpack_lo( count0_mantissa ); count1_mantissa_lo = ( uint8 )vunpack_lo( count1_mantissa );
			count0_mantissa_hi = ( uint8 )vunpack_hi( count0_mantissa ); count1_mantissa_hi = ( uint8 )vunpack_hi( count1_mantissa );

			count0_shiftdiv_lo = ( uint8 )vunpack_lo( count0_shiftdiv ); count1_shiftdiv_lo = ( uint8 )vunpack_lo( count1_shiftdiv );
			count0_shiftdiv_hi = ( uint8 )vunpack_hi( count0_shiftdiv ); count1_shiftdiv_hi = ( uint8 )vunpack_hi( count1_shiftdiv );

			result0_lo = vshiftr( weight0_lo * count0_mantissa_lo, count0_shiftdiv_lo );
			result0_hi = vshiftr( weight0_hi * count0_mantissa_hi, count0_shiftdiv_hi );

			result1_lo = vshiftr( weight1_lo * count1_mantissa_lo, count1_shiftdiv_lo );
			result1_hi = vshiftr( weight1_hi * count1_mantissa_hi, count1_shiftdiv_hi );

			result0 = ( ushort16 )( vpack( result0_lo, result0_hi ) );
			result1 = ( ushort16 )( vpack( result1_lo, result1_hi ) );

			result0 = ( ushort16 )vcmpmov( lt, result0, RK_U16( 16*1023 ), result0, RK_U16( 16*1023 ) );
			result1 = ( ushort16 )vcmpmov( lt, result1, RK_U16( 16*1023 ), result1, RK_U16( 16*1023 ) );

			//p_count = 0
			RK_U16 zero_mask0 = vcmp( eq, count0, RK_U16( 0 ) );
			RK_U16 zero_mask1 = vcmp( eq, count1, RK_U16( 0 ) );

			vst( ushort16( 0 ), p_dst0, zero_mask0 );
			vprMask1 = vprMask1 & ~zero_mask0;
			vst( result0, p_dst0, vprMask1 ); p_dst0 += 32;

			vst( ushort16( 0 ), p_dst1, zero_mask1 );
			vprMask2 = vprMask2 & ~zero_mask1;
			vst( result1, p_dst1, vprMask2 ); p_dst1 += 32;
		}
	}	

}
