/*
 * Memory setup for SMDK3520 board based on S5P3520
 *
 * Copyright (C) 2012 Samsung Electronics
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <config.h>
#include <asm/io.h>
#include <asm/arch/dmc.h>
#include <asm/arch/clock.h>
#include <asm/arch/power.h>
#include <asm/arch/cpu.h>
#include <asm/arch/smc.h>
#include "setup.h"

#define LPDDR3	0
#define DDR3	1
#define LPDDR2	2

#define Outp32(addr, data)	(*(volatile u32 *)(addr) = (data))
#define Inp32(addr)		(*(volatile u32 *)(addr))
#define SetBits(uAddr, uBaseBit, uMaskValue, uSetValue) \
				Outp32(uAddr, (Inp32(uAddr) & ~((uMaskValue)<<(uBaseBit))) \
				| (((uMaskValue)&(uSetValue))<<(uBaseBit)))

#define PWMTIMER_BASE			(0x139D0000)

#define rTCFG0					(PWMTIMER_BASE+0x00)
#define rTCFG1					(PWMTIMER_BASE+0x04)
#define rTCON					(PWMTIMER_BASE+0x08)
#define rTCNTB0					(PWMTIMER_BASE+0x0C)
#define rTCMPB0					(PWMTIMER_BASE+0x10)
#define rTCNTO0					(PWMTIMER_BASE+0x14)
#define rTINT_CSTAT				(PWMTIMER_BASE+0x44)
#define dmcOutp32(a, d) Outp32(g_uBaseAddr+a, d)
#define dmcInp32(a) Inp32(g_uBaseAddr+a)

#define	 PHY_BASE 0x106d0000
#define	 DMC_BASE 0x105f0000
#define	 TZASC_BASE 0x10600000

#define	 mem_type 0   //0=LPDDR3, 1=DDR3, 2 =LPDDR2
#define	 mclk 400

#define	pulldown_dq 0X0
#define	pulldown_dqs 0Xf

#define	mem_term_en 0X0
#define	phy_term_en 0X0

#define cs_num 0x1

#define cs0_base 0x40							//;;-ch0 base address(Physical 0x4000_0000)
#define cs1_base 0x60							//;;-ch1 base address(Physical 0x6000_0000)
//;&cs1_base=0x80							;;-ch1 base address(Physical 0x8000_0000) ; for 16Gb
#define cs0_mask 0x7E0							//;;-ch0 mask address(chip size is 512MB, then chip_mask is 0x7E0)
//;&cs0_mask=0x7C0						;;-ch0 mask address(chip size is 1GB, then chip_mask is 0x7C0); for 16Gb
#define cs1_mask 0x7E0						//	;;-ch1 mask address(chip size is 512MB, then chip_mask is 0x7E0)
//;&cs1_mask=0x7C0						;;-ch1 mask address(chip size is 1GB, then chip_mask



//static u32 g_uBaseAddr;
//static const u32 IROM_ARM_CLK = 1;

/*   CARMEN EVT1
static void DMC_Delay(u32 uFreqMhz, u32 uMicroSec)
{
	volatile u32 uDelay;
	volatile u32 uNop;

	for(uDelay = 0; uDelay < (uFreqMhz * uMicroSec); uDelay++)
	{
		uNop = uNop + 1;
		uNop = Inp32(0x10600000+0x0048);
	}
}
*/

static const u32 IROM_ARM_CLK = 1;

static void StartTimer0(void)
{
	u32 uTimer=0;
	u32 uTemp0,uTemp1;
	u32 uEnInt=0;
	u32 uDivider=0;
	u32 uDzlen=0;
	u32 uPrescaler=66;												//- Silicon : uPrescaler=66   /  FPGA : uPrescaler=24
	u32 uEnDz=0;
	u32 uAutoreload=1;
	u32 uEnInverter=0;
	u32 uTCNTB=0xffffffff;
	u32 uTCMPB=(u32)(0xffffffff/2);

	{
		uTemp0 = Inp32(rTCFG1);						// 1 changed into 0xf; dohyun kim 090211
		uTemp0 = (uTemp0 & (~ (0xf << 4*uTimer) )) | (uDivider<<4*uTimer);
		// TCFG1			init. selected MUX 0~4, 	select 1/1~1/16

		Outp32(rTCFG1,uTemp0);

		uTemp0 = Inp32(rTINT_CSTAT);
		uTemp0 = (uTemp0 & (~(1<<uTimer)))|(uEnInt<<(uTimer));
		//		selected timer disable, 		selected timer enable or diable.
		Outp32(rTINT_CSTAT,uTemp0);
	}

	{
		uTemp0 = Inp32(rTCON);
		uTemp0 = uTemp0 & (0xfffffffe);
		Outp32(rTCON, uTemp0);								// Timer0 stop

		uTemp0 = Inp32(rTCFG0);
		uTemp0 = (uTemp0 & (~(0xff00ff))) | ((uPrescaler-1)<<0) |(uDzlen<<16);
		//			init. except prescaler 1 value, 	set the Prescaler 0 value,	set the dead zone length.
		Outp32(rTCFG0, uTemp0);

		Outp32(rTCNTB0, uTCNTB);
		Outp32(rTCMPB0, uTCMPB);


		uTemp1 = Inp32(rTCON);
		uTemp1 = (uTemp1 & (~(0x1f))) |(uEnDz<<4)|(uAutoreload<<3)|(uEnInverter<<2)|(1<<1)|(0<<0);
		//		set all zero in the Tiemr 0 con., Deadzone En or dis, set autoload, 	set invert,  manual uptate, timer stop
		Outp32(rTCON, uTemp1);									//timer0 manual update
		uTemp1 = (uTemp1 & (~(0x1f))) |(uEnDz<<4)|(uAutoreload<<3)|(uEnInverter<<2)|(0<<1)|(1<<0);
		//		set all zero in the Tiemr 0 con., Deadzone En or dis, set autoload, 	set invert,  manual uptate disable, timer start
		Outp32(rTCON, uTemp1);									// timer0 start
	}
}

static void PWM_stop(u32 uNum)
{
    u32 uTemp;

    uTemp = Inp32(rTCON);

    if(uNum == 0)
		uTemp &= ~(0x1);
    else
		uTemp &= ~((0x10)<<(uNum*4));

    Outp32(rTCON,uTemp);
}

static u32 StopTimer0(void)
{
	u32 uVal = 0;

	PWM_stop(0);

	uVal = Inp32(rTCNTO0);
	uVal = 0xffffffff - uVal;

	return uVal;
}

#if 0
void DMC_Delay(int msec)
{
	volatile u32 i;
	for(;msec > 0; msec--);
		for(i=0; i<1000; i++) ;
}
#else
void DMC_Delay(u32 usec)
{
	u32 uElapsedTime, uVal;

	StartTimer0();

	while(1){
		uElapsedTime = Inp32(rTCNTO0);
		uElapsedTime = 0xffffffff - uElapsedTime;

		if(uElapsedTime > usec){
			StopTimer0();
			break;
		}
	}
}
#endif

void Check_MRStatus(u32 ch, u32 cs)
{
	u32 mr0;
	u32 resp, temp;
	u32 loop_cnt = 1000;

	#if 1 // Due to Pega W1 issue of MRR read , mrr_byte [26:25] bit of Memcontrol register must set 0x1 that MR Status Read location 0x1[15:8]
	temp = Inp32(DMC_BASE+0x0004);
	temp = temp | (0x1<<25);
	Outp32(DMC_BASE+0x0004, temp);
	#endif

	if(cs == 0)				mr0=0x09000000;  // Direct CMD 0x9=MRR(mode register reading), chip=0
	else					mr0=0x09100000;  // Direct CMD 0x9=MRR(mode register reading), chip=1

	if(ch == 0){
		do{
			Outp32(DMC_BASE+0x0010, mr0);
			resp=Inp32(DMC_BASE+0x0054)&0x1;
			loop_cnt--;

			if(loop_cnt==0){
				DMC_Delay(10);							//- Although DAI is not completed during polling it, end the loop when 10us delay is time over.
				break;
			}
		}while(resp);
	}
	else{
		do{
			Outp32(DMC_BASE+0x0010, mr0);
			resp=Inp32(DMC_BASE+0x0054)&0x1;
			loop_cnt--;

			if(loop_cnt==0){
				DMC_Delay(10);							//- Although DAI is not completed during polling it, end the loop when 10us delay is time over.
				break;
			}
		}while(resp);
	}

	return ;
}

#define MPLL_USE_FOR_MIF
void set_mem_clock(u32 uMemClk)
{
	u32 uBits;

#if defined(MPLL_USE_FOR_MIF)
	//Turn Off MPLL
	//MPLL(12)
	uBits = (1 << 12) | (1 << 8) | (1 << 4);
	Outp32(0x105C0300, Inp32(0x105C0300) & ~uBits);	// rCLK_SRC_CORE

	if ((uMemClk==533)||(uMemClk==400))
		// DMC(1/2:27), DPHY(1/2:23), DMC_PRE(1/1:19), COREP(1/2:15), CORED(1/2:11), AUDIOCODEC(1/16:0)
		uBits = (0 << 27) | (0 << 23) | (0 << 19) | (1 << 15) | (1 << 11) | (3<<0);
	else if ((uMemClk==266)||(uMemClk==200))
		// DMC(1/2:27), DPHY(1/2:23), DMC_PRE(1/2:19), COREP(1/2:15), CORED(1/2:11), AUDIOCODEC(1/16:0)
		uBits = (0 << 27) | (0 << 23) | (1 << 19) | (1 << 15) | (1 << 11) | (3<<0);
	else if ((uMemClk==133)||(uMemClk==100))
		// DMC(1/2:27), DPHY(1/2:23), DMC_PRE(1/4:19), COREP(1/2:15), CORED(1/2:11), AUDIOCODEC(1/16:0)
		uBits = (0 << 27) | (0 << 23) | (3 << 19) | (1 << 15) | (1 << 11) | (3<<0);
	else if (uMemClk==50)
		// DMC(1/2:27), DPHY(1/1:23), DMC_PRE(1/4:19), COREP(1/2:15), CORED(1/2:11), AUDIOCODEC(1/16:0)
		uBits = (1 << 27) | (0 << 23) | (3 << 19) | (1 << 15) | (1 << 11) | (3<<0);

	// Set CMU_CORE_L, MUX & DIV
	Outp32(0x105C0504, uBits);	// rCLK_DIV_CORE_1

	uBits = (1 << 12) | (0 << 10) | (0 << 8) | (0 << 4);	// use MPLL_MIF source for DPHY,DMC
	Outp32(0x105C0300, Inp32(0x105C0300) | uBits); // rCLK_SRC_CORE
	while(Inp32(0x105c0400)&((1<<18)|(1<<14)|(1<<10)|(1<<6)));

	SetBits(0x10020024,0,0x1,0x0);	//CP_CTRL CP_CTRL[0] (addr:x1002_0024) 1b0
	SetBits(0x105c0218,31,0x1,0x0);	//BPLL disable - CMU
	SetBits(0x10020074,31,0x1,0x0);	//BPLL disable - alive
#else
	//Turn Off BPLL
	//BPLL(0:10)
	uBits = (1 << 10);
	Outp32(0x105C0300, Inp32(0x105C0300) & ~uBits);	// rCLK_SRC_CORE

	// ENABLE(1:31), MDIV(m:16), PDIV(p:8), SDIV(s:0)
	if ((uMemClk==533)||(uMemClk==266))
		uBits = (1 << 31) | (533 << 16) | (6 << 8) | (2 << 0);	// BPLL=1066MHz(4:328:1), for DREX PHY
	else if ((uMemClk==400)||(uMemClk==200)||(uMemClk==133)||(uMemClk==100)||(uMemClk==50))
		uBits = (1 << 31) | (200 << 16) | (3 << 8) | (2 << 0);	// BPLL= 800MHz(13:800:1), for DREX PHY
	else
		uBits = (1 << 31) | (200 << 16) | (3 << 8) | (2 << 0);	// BPLL= 800MHz(13:800:1), for DREX PHY

	//Outp32(0x105C0218, uBits);							// rBPLL_CON0 in CORE_L
	//while ((Inp32(0x105C0218) & (1 << 29)) == 0);			//Wait BPLL Core_l locked

	Outp32(0x10020074, uBits);							// rBPLL_CON0 in ALIVE
	while ((Inp32(0x10020074) & (1 << 29)) == 0);			//Wait BPLL ALIVE locked


	if ((uMemClk==533)||(uMemClk==400))
		// DMC(1/2:27), DPHY(1/2:23), DMC_PRE(1/1:19), COREP(1/2:15), CORED(1/2:11), AUDIOCODEC(1/16:0)
		uBits = (0 << 27) | (0 << 23) | (0 << 19) | (1 << 15) | (1 << 11) | (3<<0);
	else if ((uMemClk==266)||(uMemClk==200))
		// DMC(1/2:27), DPHY(1/2:23), DMC_PRE(1/2:19), COREP(1/2:15), CORED(1/2:11), AUDIOCODEC(1/16:0)
		uBits = (0 << 27) | (0 << 23) | (1 << 19) | (1 << 15) | (1 << 11) | (3<<0);
	else if (uMemClk==133)
		// DMC(1/2:27), DPHY(1/2:23), DMC_PRE(1/4:19), COREP(1/2:15), CORED(1/2:11), AUDIOCODEC(1/16:0)
		uBits = (0 << 27) | (0 << 23) | (2 << 19) | (1 << 15) | (1 << 11) | (3<<0);
	else if (uMemClk==100)
		// DMC(1/2:27), DPHY(1/2:23), DMC_PRE(1/4:19), COREP(1/2:15), CORED(1/2:11), AUDIOCODEC(1/16:0)
		uBits = (0 << 27) | (0 << 23) | (3 << 19) | (1 << 15) | (1 << 11) | (3<<0);
	else if (uMemClk==50)
		// DMC(1/2:27), DPHY(1/1:23), DMC_PRE(1/4:19), COREP(1/2:15), CORED(1/2:11), AUDIOCODEC(1/16:0)
		uBits = (1 << 27) | (0 << 23) | (3 << 19) | (1 << 15) | (1 << 11) | (3<<0);


	// Set CMU_CORE_L, MUX & DIV
	Outp32(0x105C0504, uBits);	// rCLK_DIV_CORE_1

	// BPLL(0:10)
	uBits = (1 << 10);
	Outp32(0x105C0300, Inp32(0x105C0300) | uBits); // rCLK_SRC_CORE
#endif // defined(MPLL_USE_FOR_MIF)
}

void phy_mem_type(u32 mem_type_data) //0=LPDDR3, 1=DDR3, 2 =LPDDR2
{
	u32 uBits, temp;

	if (mem_type_data==0)
		uBits=0x1800;
	else if (mem_type_data==1)
		uBits=0x800;
	else if (mem_type_data==2)
		uBits=0x1000;

	temp = Inp32(PHY_BASE+0X0)&0xFFFFE7FF;
	temp |= uBits;

	Outp32(PHY_BASE+0X0, temp);
}

void pulldown_dq_dqs(u32 dq, u32 dqs)
{
	u32 temp;

	dq = dq&0xf;
	dq = dq<<8;
	dqs= dqs&0xf;

	temp = dq | dqs;

	Outp32(PHY_BASE+0X0038, temp);
}

void phy_set_bl_rl_wl(u32 mem, u32 clk)  //mem:0=LPDDR3, 1=DDR3, 2 =LPDDR2
{
	u32 bl, rl, wl, temp;

	temp = 0;
	if (mem==2)   //lpddr2
		bl = 4;
	else if (mem==0)
		bl = 8;

	if ((clk==400)||(clk==266)||(clk==200)||(clk==133)||(clk==100)||(clk==50))
		{
			rl=9;
			wl=5;  //In LPDDR3, the minimum wl is 5
		}
	else if (clk==533)
		{
			rl=9;
			wl=5;
		}

	bl=bl&0x1f;
	rl=rl&0x1f;
	temp=temp|(bl<<8)|rl;

	Outp32(PHY_BASE+0X00ac, temp);

	//DMC_Delay(IROM_ARM_CLK, 1000); // wait 1m////////////////////////////////////added by WangCL 2013.11.01

	wl=wl&0x1f;
	wl=wl+1;
	temp = Inp32(PHY_BASE+0X006c)&0xFFE0000F;
	temp=temp|(wl<<16);
	Outp32(PHY_BASE+0X006c, temp);
}

#if 0
void dram_init_cm1(void)
{
#if defined(CONFIG_SMC_CMD)
	reg_arr_t reg_arr;
#endif
	u32 temp, uBits;
	u32 i,j;
	u32 mem_var,bl_var,pzq_val;
	u32 row_param, data_param, power_param, rfcpb;
	u32 port, cs, nop, mr63, mr10, mr1, mr2, mr3, pall;

	//;-------------------------------------------------------------------------
	//;- Change CPU to 50Mhz
	//;----------------------------------------------------------------------------
	// MPLL_USER(1:24), HPM(0:20), CPU(1:16), APLL(0:0)
	uBits = (1 << 24) | (0 << 20) | (1 << 16) | (0 << 0);
	Outp32(0x10044200, uBits);			// rCLK_SRC_CPU ->mpll

	// CORE2(1/2:28), APLL(1/2:24), PCLK_DBG(1/8:20), ATB(1/4:16), COREM(1/2:4), CORE(1/4:0)
	uBits = (1 << 28) | (1 << 24) | (7 << 20) | (3 << 16) | (1 << 4) | (3<< 0);
	Outp32(0x10044500, uBits);			// rCLK_DIV_CPU0  ARMCLK=400/2/4=50MHz

	// MPLL_USER(1:24), HPM(0:20), CPU(0:16), APLL(0:0)
	uBits = (1 << 24) | (0 << 20) | (0 << 16) | (0 << 0);
	Outp32(0x10044200, uBits);			// rCLK_SRC_CPU->apll

	//while(1);   //for debug

	//;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	//;- CLK Pause enable
	//GOSUB write 0x105C1094 0x1		"CLK Pause Enable"
	//;---------------------------------------------------------------------
	//Outp32(0x105C1094, 0x1);

	//;-----------------------------------------------
	//;- CA SWAP Setting..!
	//;-----------------------------------------------
	//GOSUB add_comment		 	"CA SWAP Setting..!"
	//GOSUB CA_swap_lpddr2 &PHY_BASE &DMC_BASE;
	temp = Inp32(DMC_BASE+0X0000)&(~0x00000001)|(0x00000001);
	Outp32(DMC_BASE+0X0000, temp);

	//DMC_Delay(IROM_ARM_CLK, 1000); // wait 1m////////////////////////////////////added by WangCL 2013.11.01

	temp = Inp32(PHY_BASE+0X0064)&(~0x00000001)|(0x00000001);
	Outp32(PHY_BASE+0X0064, temp);

	set_mem_clock(50);

	phy_mem_type(mem_type); //LPDDR2

	pulldown_dq_dqs(pulldown_dq, pulldown_dqs);

	phy_set_bl_rl_wl(mem_type, mclk);

	//dmc_term(mem_term_en, phy_term_en);

	//---------------------------------------------------------------------------------
	//;- Set rd_fetch parameter
	//;----------------------------------------------------------------------------
	//GOSUB dmc_rd_fetch		&rd_fetch
	//&rd_fetch=0x2
	temp = Inp32(DMC_BASE+0X0000);
	temp = temp &0xFFFF8FFF;
	temp = temp | ((0x2&0x7)<<12);  //rd_fetch=0x2
	Outp32(DMC_BASE+0X0000, temp);

//no need by weicong@2013.11.4	DMC_Delay(IROM_ARM_CLK, 1000); // wait 1m////////////////////////////////////added by WangCL 2013.11.01

	//---------------------------------------------------------------------------------
	//- Assert the ConControl.dfi_init_start field then deassert
	//----------------------------------------------------------------------------
	//GOSUB dfi_init_start 		0x1						;;- dfi_init_start
	temp = Inp32(DMC_BASE+0X0000)|0x10000000;
	temp= temp&0xFFFFFFDF;
	Outp32(DMC_BASE+0X0000, temp);

////////////////////////////////20131106        DMC_Delay(IROM_ARM_CLK, 1000); // wait 1m////////////////////////////////////added by WangCL 2013.11.01

	//wait_status==1
	while(Inp32(0x105f0040)&0x00000008!=0x8); //wait dfi_init_complete
	temp=temp&0xEFFFFFFF;
	Outp32(DMC_BASE+0X0000, temp);

	//GOSUB fp_resync									;;- fp_resync
	temp = Inp32(DMC_BASE+0X0018);
	temp = temp |0x8;
	Outp32(DMC_BASE+0X0018, temp);

////////////////////////////////20131106 	DMC_Delay(IROM_ARM_CLK, 1000); // wait 1m////////////////////////////////////added by WangCL 2013.11.01 // short delay by weicong@2013.11.4

	temp=temp&0xFFFFFFF7;
	Outp32(DMC_BASE+0X0018, temp);

////////////////////////////////20131106 	DMC_Delay(IROM_ARM_CLK, 1000); // wait 1m////////////////////////////////////added by WangCL 2013.11.01 // short delay by weicong@2013.11.4

	//;---------------------------------------------------------------------------------
	//;- Set the memory base register
	//;----------------------------------------------------------------------------
	//(cs_num==0x1)
	//GOSUB dmc_membase_cfg 	0x0 &cs0_base &cs0_mask			;;- chip0 base
	//&cs0_base=0x40							;;-ch0 base address(Physical 0x4000_0000)
	//&cs0_mask=0x7E0							;;-ch0 mask address(chip size is 512MB, then chip_mask is 0x7E0)
	i = cs0_base&0x7ff;
	j = cs0_mask & 0x7ff;
	temp = 0;
	temp=temp|(i<<16);
	temp=temp|j;
#if defined(CONFIG_SMC_CMD)
	reg_arr.set0.addr = TZASC_BASE+0x0F00;
	reg_arr.set0.val = temp;

	set_secure_reg((u32)&reg_arr, 1);
#else
	Outp32(TZASC_BASE+0X0F00, temp);
#endif

////////////////////////////////20131106 	DMC_Delay(IROM_ARM_CLK, 1000); // wait 1m////////////////////////////////////added by WangCL 2013.11.01

	//GOSUB dmc_membase_cfg 	0x1 &cs1_base &cs1_mask			;;- chip1 base
	//&cs1_base=0x60							;;-ch1 base address(Physical 0x6000_0000)
	//&cs1_mask=0x7E0							;;-ch1 mask address(chip size is 512MB, then chip_mask is 0x7E0)
	i = cs1_base&0x7ff;
	j = cs1_mask & 0x7ff;
	temp = 0;
	temp=temp|(i<<16);
	temp=temp|j;
#if defined(CONFIG_SMC_CMD)
	reg_arr.set0.addr = TZASC_BASE+0x0F04;
	reg_arr.set0.val = temp;

	set_secure_reg((u32)&reg_arr, 1);
#else
	Outp32(TZASC_BASE+0X0F04, temp);
#endif

////////////////////////////////20131106 	DMC_Delay(IROM_ARM_CLK, 1000); // wait 1m////////////////////////////////////added by WangCL 2013.11.01

	//;---------------------------------------------------------------------------------
	//;- Set the memory control register
	//;----------------------------------------------------------------------------
	//GOSUB dmc_memcontrol	&memory &cs_num &add_lat_pall &pzq_en	;;- memcontrol
	//(cs_num==0x1)
	//&add_lat_pall=0x1						;;- Additional Latency for PALL in cclk cycle
	//	&pzq_en=0x0 							;;DDR3 periodic ZQ(ZQCS) enable

	if (mem_type==2)  //lpddr2
	{
		mem_var=0x5;
		bl_var=0x2;
	}
	else if (mem_type==0)  //lpddr3
	{
		mem_var=0x7;
		bl_var=0x3;
	}
	else if (mem_type==1)  //ddr3
	{
		mem_var=0x6;
		bl_var=0x3;
	}

	bl_var=bl_var&0x7;
	mem_var=mem_var&0xF;
	//pzq=pzq&0x1;
	//&chip=&chip&0xF
	//&add_pall=&add_pall&0xF

	temp=0;
	temp=temp|0x00002000;
	temp=temp|(0<<24);
	temp=temp|(bl_var<<20);
	temp=temp|(1<<16);
	temp=temp|(mem_var<<8);
	temp=temp|(1<<6);
	Outp32(DMC_BASE+0X0004, temp);

//no need by weicong@2013.11.4	DMC_Delay(IROM_ARM_CLK, 1000); // wait 1m////////////////////////////////////added by WangCL 2013.11.01

	//;---------------------------------------------------------------------------------
	//;- Set the memory config register
	//;----------------------------------------------------------------------------
	//GOSUB dmc_memconfig			&cs_num &addr_map &col_len &row_len &bank_num ;;- memconfig
	//cs_num=1   &addr_map=0x2  &col_len=0x3 &row_len=0x2 &bank_num=0x3
	//	ENTRY &cs &map &col &row &ba
	temp = 0;
	temp=temp|(0x2<<12);
	temp=temp|(0x3<<8);
	temp=temp|(0x2<<4);
	temp=temp|(0x3);
#if defined(CONFIG_SMC_CMD)
	reg_arr.set0.addr = TZASC_BASE+0x0F10;
	reg_arr.set0.val = temp;
	reg_arr.set1.addr = TZASC_BASE+0x0F14;
	reg_arr.set1.val = temp;

	set_secure_reg((u32)&reg_arr, 2);
#else
	Outp32(TZASC_BASE+0X0F10, temp);
	Outp32(TZASC_BASE+0X0F14, temp);
#endif

////////////////////////////////20131106 	DMC_Delay(IROM_ARM_CLK, 1000); // wait 1m////////////////////////////////////added by WangCL 2013.11.01

	//;---------------------------------------------------------------------------------
	//;- Address mapping method setting
	//;----------------------------------------------------------------------------
	//GOSUB map_dmc_memconfig			&bank_lsb &rank_inter_en &bit_sel_en &bit_sel	;mapping method
	//;;---------Address setting method-----------------------------------------------
	//&bank_lsb=0x4		;;choose the column width for the best performance.
	//&rank_inter_en=0x0		;;Rank Interleaved Address Mapping enable
	//&bit_sel_en=0x1			;;Enable Bit Selection for Randomized Interleaved Address enable
	//&bit_sel=0x2			;;0x0: bit position = [14:12] (if rank_inter_en is enabled, [15:12]),
	//ENTRY  &bank_lsb &rank_inter_en &bit_sel_en &bit_sel
#if defined(CONFIG_SMC_CMD)
	temp = read_secure_reg(TZASC_BASE+0X0F10) & 0xFF80FFFF;
#else
	temp = Inp32(TZASC_BASE+0X0F10)&0xFF80FFFF;
#endif
	temp=temp|(0X4<<20);
	temp=temp|(0<<19);
	temp=temp|(0X1<<18);
	temp=temp|(0X2<<16);
#if defined(CONFIG_SMC_CMD)
	reg_arr.set0.addr = TZASC_BASE+0x0F10;
	reg_arr.set0.val = temp;

	set_secure_reg((u32)&reg_arr, 1);
#else
	Outp32(TZASC_BASE+0X0F10, temp);
#endif

	//DMC_Delay(IROM_ARM_CLK, 1000); // wait 1m////////////////////////////////////added by WangCL 2013.11.01

#if defined(CONFIG_SMC_CMD)
	temp = read_secure_reg(TZASC_BASE+0X0F14) & 0xFF80FFFF;
#else
	temp = Inp32(TZASC_BASE+0X0F14)&0xFF80FFFF;
#endif
	temp=temp|(0X4<<20);
	temp=temp|(0<<19);
	temp=temp|(0X1<<18);
	temp=temp|(0X2<<16);
#if defined(CONFIG_SMC_CMD)
	reg_arr.set0.addr = TZASC_BASE+0x0F14;
	reg_arr.set0.val = temp;

	set_secure_reg((u32)&reg_arr, 1);
#else
	Outp32(TZASC_BASE+0X0F14, temp);
#endif

//no need by weicong@2013.11.4	DMC_Delay(IROM_ARM_CLK, 1000); // wait 1m////////////////////////////////////added by WangCL 2013.11.01

	//;---------------------------------------------------------------------------------
	//;- Set auto refresh interval
	//;----------------------------------------------------------------------------
	//GOSUB dmc_refresh_interval	&t_refipb &t_refi
	//;------ Set auto refresh interval--------------------------------
	//&t_refipb=0xc
	//&t_refi=0x65
	temp=0;
	temp=temp|(0XC<<16);
	temp=temp|0X65;
	//GOSUB set_dmc_config 0x0030 &temp "REFI=&t_refi"
	//GOSUB write &DMC_BASE+&offset &var &str
	Outp32(DMC_BASE+0X0030, temp);

////////////////////////////////20131106 	DMC_Delay(IROM_ARM_CLK, 1000); // wait 1m////////////////////////////////////added by WangCL 2013.11.01

	//;----------timing switch setting-------------------------
	//GOSUB timing_set_con  &timing_set_sw &timing_set_sw_con
	//;----------timing switch setting-------------------------
	//&timing_set_sw=0x0	;0x0 = Use timing parameter set #0
	//		;0x1 = Use timing parameter set #1
	//&timing_set_sw_con=0x0	;0x0 = Switching controlled by external port (port name is tim-ing_set_sw)
	//		;0x1 = Switching controlled by SFR;;;;;;;;;;;;;;;;;;;;;
	temp = (0x0<<4)|0x0;
	//GOSUB write &DMC_BASE+0x00E0 &temp
	Outp32(DMC_BASE+0X00E0, temp);

////////////////////////////////20131106 	DMC_Delay(IROM_ARM_CLK, 1000); // wait 1m////////////////////////////////////added by WangCL 2013.11.01 // short delay by weicong@2013.11.4

	//;---------------------------------------------------------------------------------
	//;- Set timing_refpb, timing_row, timing_data, timing_power
	//;----------------------------------------------------------------------------
	//GOSUB dmc_timing_parameter	&mclk					;;- set timing parameter
	//if (mclk==100)
	//	{
	//	rfcpb=(3<<8)|(3<<0);
	//	row_param=(7<<24)|(2<<20)|(2<<16)|(2<<12)|(4<<6)|(3<<0);
	//	data_param=(2<<28)|(2<<24)|(2<<20)|(3<<8)|(1<<4)|(6<<0);
	//	power_param=(4<<26)|(7<<16)|(2<<8)|(2<<4)|(5<<0);

	//	}
	//else if (mclk==133)
	//	{
	//	rfcpb=(4<<8)|(4<<0);
	//	row_param=(9<<24)|(2<<20)|(2<<16)|(2<<12)|(5<<6)|(3<<0);
	//	data_param=(2<<28)|(2<<24)|(2<<20)|(3<<8)|(1<<4)|(6<<0);
	//	power_param=(4<<26)|(10<<16)|(2<<8)|(2<<4)|(5<<0);

	//	}
	//else if (mclk==200)
	//	{
	//	rfcpb=(6<<8)|(6<<0);
	//	row_param=(13<<24)|(2<<20)|(3<<16)|(2<<12)|(7<<6)|(5<<0);
	//	data_param=(2<<28)|(2<<24)|(2<<20)|(3<<8)|(2<<4)|(6<<0);
	//	power_param=(5<<26)|(14<<16)|(2<<8)|(2<<4)|(5<<0);

	//	}
	//else if (mclk==266)
	//	{
	//	rfcpb=(8<<8)|(8<<0);
	//	row_param=(18<<24)|(2<<20)|(3<<16)|(3<<12)|(9<<6)|(6<<0);
	//	data_param=(2<<28)|(2<<24)|(2<<20)|(3<<8)|(2<<4)|(6<<0);
	//	power_param=(7<<26)|(19<<16)|(2<<8)|(2<<4)|(5<<0);

	//	}
	if (mclk==400)
		{
		rfcpb=(12<<8)|(12<<0);
		row_param=(26<<24)|(2<<20)|(5<<16)|(4<<12)|(13<<6)|(9<<0);
		data_param=(2<<28)|(3<<24)|(2<<20)|(3<<8)|(3<<4)|(6<<0);
		power_param=(10<<26)|(28<<16)|(2<<8)|(3<<4)|(5<<0);

		}
	else if (mclk==533)
		{
		rfcpb=(16<<8)|(16<<0);
		row_param=(35<<24)|(3<<20)|(6<<16)|(5<<12)|(17<<6)|(12<<0);
		data_param=(2<<28)|(4<<24)|(2<<20)|(4<<8)|(3<<4)|(8<<0);
		power_param=(14<<26)|(38<<16)|(2<<8)|(4<<4)|(5<<0);

		}
	//else if (mclk==50)
	//	{
	//	rfcpb=(2<<8)|(2<<0);
	//	row_param=(4<<24)|(2<<20)|(2<<16)|(2<<12)|(2<<6)|(2<<0);
	//	data_param=(2<<28)|(2<<24)|(2<<20)|(3<<8)|(1<<4)|(6<<0);
	//	power_param=(2<<26)|(4<<16)|(2<<8)|(2<<4)|(5<<0);
	//	}
		//GOSUB write &DMC_BASE+0x0020 &rfcpb "DREX.TIMINGROW=&row_param"
		//GOSUB write &DMC_BASE+0x0034 &row_param "DREX.TIMINGROW=&row_param"
		//GOSUB write &DMC_BASE+0x0038 &data_param "DREX.TIMINGDATA=&data_param"
		//GOSUB write &DMC_BASE+0x003C &power_param "DREX.TIMINGPOWER=&power_param"
		//GOSUB write &DMC_BASE+0x00E4 &row_param "DREX.TIMINGROW=&row_param"
		//GOSUB write &DMC_BASE+0x00E8 &data_param "DREX.TIMINGDATA=&data_param"
		//GOSUB write &DMC_BASE+0x00EC &power_param "DREX.TIMINGPOWER=&power_param"
		//GOSUB write &DMC_BASE+0x0044 0x00002270 "set the ETCTiming"	;ETCTiming
		Outp32(DMC_BASE+0X0020, rfcpb);
		//DMC_Delay(IROM_ARM_CLK, 1000); // wait 1m////////////////////////////////////added by WangCL 2013.11.01
		Outp32(DMC_BASE+0X0034, row_param);
		//DMC_Delay(IROM_ARM_CLK, 1000); // wait 1m////////////////////////////////////added by WangCL 2013.11.01
		Outp32(DMC_BASE+0X0038, data_param);
		//DMC_Delay(IROM_ARM_CLK, 1000); // wait 1m////////////////////////////////////added by WangCL 2013.11.01
		Outp32(DMC_BASE+0X003c, power_param);
		//DMC_Delay(IROM_ARM_CLK, 1000); // wait 1m////////////////////////////////////added by WangCL 2013.11.01
		Outp32(DMC_BASE+0X00e4, row_param);
		//DMC_Delay(IROM_ARM_CLK, 1000); // wait 1m////////////////////////////////////added by WangCL 2013.11.01
		Outp32(DMC_BASE+0X00e8, data_param);
		//DMC_Delay(IROM_ARM_CLK, 1000); // wait 1m////////////////////////////////////added by WangCL 2013.11.01
		Outp32(DMC_BASE+0X00ec, power_param);
		//DMC_Delay(IROM_ARM_CLK, 1000); // wait 1m////////////////////////////////////added by WangCL 2013.11.01
		Outp32(DMC_BASE+0X0044, 0x00002270);

////////////////////////////////20131106 		DMC_Delay(IROM_ARM_CLK, 1000); // wait 1m////////////////////////////////////added by WangCL 2013.11.01 // short delay by weicong@2013.11.4

		//;---------------------------------------------------------------------------------
		//;- low frequency operation
		//;----------------------------------------------------------------------------
		//GOSUB low_freq_op_mode

		//GOSUB set_offsetr 			 0x7F 0x7F 0x7F 0x7F	;
		//ENTRY &dq3 &dq2 &dq1 &dq0
		temp = 0x7f|(0x7f<<8)|(0x7f<<16)|(0x7f<<24);
		Outp32(PHY_BASE+0X0010, temp);

		//DMC_Delay(1000);  //1ms
		////////////////////////////////20131106 DMC_Delay(IROM_ARM_CLK, 100); // wait 1ms // short delay by weicong@2013.11.4

		//GOSUB set_offsetw 			 0x7F 0x7F 0x7F 0x7F	;
		//ENTRY &dq3 &dq2 &dq1 &dq0
		temp = 0x7f|(0x7f<<8)|(0x7f<<16)|(0x7f<<24);
		Outp32(PHY_BASE+0X0018, temp);

		//DMC_Delay(1000);  //1ms
		//DMC_Delay(IROM_ARM_CLK, 100); // wait 1ms // short delay by weicong@2013.11.4

		//GOSUB set_offsetd 			 0x7F	;
		//ENTRY  &dll
		temp = Inp32(PHY_BASE+0X0028)&0xFFFFFF00;
		temp = temp | 0x7f;
		Outp32(PHY_BASE+0X0028, temp);

		//DMC_Delay(1000);  //1ms
		////////////////////////////////20131106 DMC_Delay(IROM_ARM_CLK, 100); // wait 1ms // short delay by weicong@2013.11.4

		//GOSUB ca_deskew_code 0x60						;;- if low freq.
		//ENTRY &code
		temp = ((0x60&0xf)<<28)|(0x60<<21)|(0x60<<14)|(0x60<<7)|0x60;
		Outp32(PHY_BASE+0X0080, temp);

		//DMC_Delay(IROM_ARM_CLK, 1000); // wait 1m////////////////////////////////////added by WangCL 2013.11.01

		temp = ((0x60&0x1)<<31)|(0x60<<24)|(0x60<<17)|(0x60<<10)|(0x60<<3)|(0x60>>4)&0x3;
		Outp32(PHY_BASE+0X0084, temp);

		//DMC_Delay(IROM_ARM_CLK, 1000); // wait 1m////////////////////////////////////added by WangCL 2013.11.01

		temp = (0x60>>1)&0x3f;
		Outp32(PHY_BASE+0X0088, temp);

////////////////////////////////20131106 		DMC_Delay(IROM_ARM_CLK, 100); // wait 1m////////////////////////////////////added by WangCL 2013.11.01 // short delay by weicong@2013.11.4

		//GOSUB dll_off_forcing 		0x7F					;- dll off and lock value force
		//ENTRY &lock
		i = 0x7f<<0x08;
		temp = Inp32(PHY_BASE+0X0030)&0xFFFF80FF;
		i = i | temp;
		Outp32(PHY_BASE+0X0030, i);

////////////////////////////////20131106 		DMC_Delay(IROM_ARM_CLK, 100); // wait 1m////////////////////////////////////added by WangCL 2013.11.01 // short delay by weicong@2013.11.4

		temp = Inp32(PHY_BASE+0X0030)&0xFFFFFFDF;
		Outp32(PHY_BASE+0X0030, temp);

		//DMC_Delay(1000);  //1ms
////////////////////////////////20131106 DMC_Delay(IROM_ARM_CLK, 100); // wait 1ms // short delay by weicong@2013.11.4

		//GOSUB fp_resync										;- fp_resync
		temp = Inp32(DMC_BASE+0X0018)|0X8;
		Outp32(DMC_BASE+0X0018, temp);
////////////////////////////////20131106  		DMC_Delay(IROM_ARM_CLK, 100); // wait 1m////////////////////////////////////added by WangCL 2013.11.01 // short delay by weicong@2013.11.4
		temp=temp&0xFFFFFFF7;
		Outp32(DMC_BASE+0X0018, temp);

////////////////////////////////20131106  		DMC_Delay(IROM_ARM_CLK, 100); // wait 1m////////////////////////////////////added by WangCL 2013.11.01 // short delay by weicong@2013.11.4

		//;-------------------------------------------------------------------------
		//;- Issue command to intial memory(DDR)
		//;----------------------------------------------------------------------------
		//GOSUB mem_init 				&mclk &mem_ds 			;;- memory init
		//&mem_ds=0x1  ;To get the strong strength, set this value to the large value.
		//	ENTRY &clk &ds
		//LOCAL &port &cs &nop &mr63 &mr10 &mr1 &mr2 &mr3
		port=0;
	while (port<0x1)
		{
		nop=0x07000000;
		mr63=0x00071C00;
		mr10=0x00010BFC;

		if (mclk==400)
		{
			mr1=0x00000608;
			mr2=0x00000810;
		}
		else if (mclk==533)
		{
			mr1=0x00000708;
			mr2=0x00000818;
		}
		//else if (mclk==266)
		//	{
		//	mr1=0x00000508;
		//	mr2=0x00000810;
		//	}
		//else if (mclk==200)
		//	{
		//	mr1=0x00000508;
		//	mr2=0x00000810;
		//	}
		//else if (mclk==133)
		//	{
		//	mr1=0x00000508;
		//	mr2=0x00000810;
		//	}
		//else if (mclk==100)
		//	{
		//	mr1=0x00000508;
		//	mr2=0x00000810;
		//	}
		//else if (mclk==50)
		//	{
		//	mr1=0x00000508;
		//	mr2=0x00000810;
		//	}

		//;;- mr3 : I/O configuration
		//if (mem_ds==0x1)

		mr3=0x00000C04;		//; 34.3

		if (port==1)
		{
			nop=nop|0x10000000;
			mr63=mr63|0x10000000;
			mr10=mr10|0x10000000;
			mr1=mr1|0x10000000;
			mr2=mr2|0x10000000;
			mr3=mr3|0x10000000;
		}

		cs=0;
		while (cs<0x2)
		{
			if (cs==1)
			{
				nop=nop|0x100000;
				mr63=mr63|0x100000;
				mr10=mr10|0x100000;
				mr1=mr1|0x100000;
				mr2=mr2|0x100000;
				mr3=mr3|0x100000;
			}

			//GOSUB write &DMC_BASE+0x0010 &nop "port:&port, cs:&cs nop command"
			//GOSUB waiting 1ms
			Outp32(DMC_BASE+0X0010, nop);
			//DMC_Delay(1000);  //1ms
			DMC_Delay(IROM_ARM_CLK, 200); // wait 1ms

			//print "send mr63 command &mr63"
			//GOSUB write &DMC_BASE+0x0010 &mr63 "port:&port, cs:&cs mr63 command"
			//GOSUB waiting 100ms
			Outp32(DMC_BASE+0X0010, mr63);
			//DMC_Delay(100000);  //100ms
			DMC_Delay(IROM_ARM_CLK, 1); // wait 100ms

			//print "send mr10 command &mr10"
			//GOSUB write &DMC_BASE+0x0010 &mr10 "port:&port, cs:&cs mr10 command"
			//GOSUB waiting 100ms
			Outp32(DMC_BASE+0X0010, mr10);
			//DMC_Delay(100000);  //100ms
			DMC_Delay(IROM_ARM_CLK, 1); // wait 100ms

			//print "send mr1 command &mr1"
			//GOSUB write &DMC_BASE+0x0010 &mr1 "port:&port, cs:&cs mr1 command"
			//GOSUB waiting 1ms
			Outp32(DMC_BASE+0X0010, mr1);
			//DMC_Delay(1000);  //1ms
			//no need by WZW@2013.11.6 DMC_Delay(IROM_ARM_CLK, 1000); // wait 1ms

			//print "send mr2 command &mr2"
			//GOSUB write &DMC_BASE+0x0010 &mr2 "port:&port, cs:&cs mr2 command"
			//GOSUB waiting 1ms
			Outp32(DMC_BASE+0X0010, mr2);
			//DMC_Delay(1000);  //1ms
			//no need by WZW@2013.11.6 DMC_Delay(IROM_ARM_CLK, 1000); // wait 1ms

			//print "send mr3 command &mr3"
			//GOSUB write &DMC_BASE+0x0010 &mr3 "port:&port, cs:&cs mr3 command"
			//GOSUB waiting 1ms
			Outp32(DMC_BASE+0X0010, mr3);
			//DMC_Delay(1000);  //1ms
			//no need by WZW@2013.11.6 DMC_Delay(IROM_ARM_CLK, 1000); // wait 1ms

			cs=cs+1;
		}
		port=port+1;
	}

	//;-------------------------------------------------------------------------
	//;- Set memory clock to normal frequency
	//;----------------------------------------------------------------------------
	set_mem_clock(400);


	//;-------------------------------------------------------------------------
	//;- Change CORE_R frequency to 50Mhz
	//;----------------------------------------------------------------------------
	// COREP(1/2:16), CORED(1/2:12), DMC(1/2:8), ACP_PCLK(1/2:4), ACP(1/4:0)
	uBits = (1 << 16) | (1 << 12) | (1 << 8) | (1 << 4) | (3 << 0);
	Outp32(0x10450500, uBits);											// rCLK_DIV_CORE_R0

	//;-------------------------------------------------------------------------
	//;- Change CPU to 400Mhz
	//;----------------------------------------------------------------------------
	 // MPLL_USER(1:24), HPM(0:20), CPU(1:16), APLL(0:0)
	uBits = (1 << 24) | (0 << 20) | (1 << 16) | (0 << 0);
	Outp32(0x10044200, uBits);			// rCLK_SRC_CPU ->mpll


	// �� CORE2(1/1:28), APLL(1/2:24), PCLK_DBG(1/8:20), ATB(1/4:16), COREM(1/2:4), CORE(1/1:0)
	uBits = (0 << 28) | (1 << 24) | (7 << 20) | (3 << 16) | (1 << 4) | (0<< 0);
	Outp32(0x10044500, uBits);			// rCLK_DIV_CPU0  ARMCLK=400/1/1=400MHz

	// MPLL_USER(1:24), HPM(0:20), CPU(0:16), APLL(0:0)
	uBits = (1 << 24) | (0 << 20) | (0 << 16) | (0 << 0);
	Outp32(0x10044200, uBits);			// rCLK_SRC_CPU->apll

	//no need by WZW@2013.11.6 DMC_Delay(IROM_ARM_CLK, 100); // wait 1m////////////////////////////////////added by WangCL 2013.11.01 // short delay by weicong@2013.11.4

	//;-----------------------------------------------------------------------
	//;- Ch/CLK/CKE/CS/CA DDS Setting
	//;-----------------------------------------------------------------------
	//GOSUB set_dds			 &data_ds &clk_ds &cke_ds &cs_ds &ca_ds
	//ENTRY  &data_ds &clk_ds &cke_ds &cs_ds &ca_ds
	//&clk_ds=0x6
	//&cke_ds=0x6
	//&cs_ds=0x6
	//&ca_ds=0x6
	//&data_ds=0x6
	temp = Inp32(PHY_BASE+0X00A0)&0xF0000000;
	temp = temp |(0x7<<25)|(0x7<<22)|(0x7<<19)|(0x7<<16)|(0x7<<9)|(0x7<<6)|(0x7<<3)|0x7;
	Outp32(PHY_BASE+0X00A0, temp);

	//no need by WZW@2013.11.6 DMC_Delay(IROM_ARM_CLK, 100); // wait 1m////////////////////////////////////added by WangCL 2013.11.01 // short delay by weicong@2013.11.4

	//;-------------zq initial auto or one time--------------------------
	//&zq_cal_auto=0x0
	//;------------------------------------------------------------------------
	//;- ZQ_INIT/ZQ_DDS Setting.
	//;------------------------------------------------------------------------
	//GOSUB zq_init 				&zq_ds	&on_die_term					;;- zq init
	//&zq_ds=0x6
	//;------------on die termination resistor-------------------------
	//&on_die_term=0x0;;- 0x6, 0x5, 0x4
	//ENTRY &dds &term
	temp = Inp32(PHY_BASE+0X0040)|0x08040000|0x00080000|(0x7<<24)|(0<<21);
	Outp32(PHY_BASE+0X0040, temp);
	//DMC_Delay(IROM_ARM_CLK, 1000); // wait 1m////////////////////////////////////added by WangCL 2013.11.01
	temp = temp| 0x04;
	Outp32(PHY_BASE+0X0040, temp);
	//DMC_Delay(IROM_ARM_CLK, 1000); // wait 1m////////////////////////////////////added by WangCL 2013.11.01
	temp = temp| 0x02;
	Outp32(PHY_BASE+0X0040, temp);

	//no need by WZW@2013.11.6 DMC_Delay(IROM_ARM_CLK, 100); // wait 1m////////////////////////////////////added by WangCL 2013.11.01 // short delay by weicong@2013.11.4

	while(Inp32(PHY_BASE+0X0048)&0x00000001!=0x1); //"PHY0: wait for zq_done"

	temp = Inp32(PHY_BASE+0X0040)&0xFFFBFFFD;
	Outp32(PHY_BASE+0X0040, temp);

	//no need by WZW@2013.11.6 DMC_Delay(IROM_ARM_CLK, 100); // wait 1m////////////////////////////////////added by WangCL 2013.11.01 // short delay by weicong@2013.11.4

	//;---------------------------------------------------------------------------------
	//;- normal frequency operation
	//;----------------------------------------------------------------------------
	//&default_set=0
	//GOSUB set_offsetr 			0x08 0x08 0x08 0x08	;;
		//ENTRY &dq3 &dq2 &dq1 &dq0
		temp = 0x08|(0x08<<8)|(0x08<<16)|(0x08<<24);
		Outp32(PHY_BASE+0X0010, temp);

		//DMC_Delay(1000);  //1ms
		//no need by WZW@2013.11.6 DMC_Delay(IROM_ARM_CLK, 100); // wait 1ms // short delay by weicong@2013.11.4

		//GOSUB set_offsetw 			0x08 0x08 0x08 0x08	;
		//ENTRY &dq3 &dq2 &dq1 &dq0
		temp = 0x08|(0x08<<8)|(0x08<<16)|(0x08<<24);
		Outp32(PHY_BASE+0X0018, temp);

		//DMC_Delay(1000);  //1ms
		//no need by WZW@2013.11.6 DMC_Delay(IROM_ARM_CLK, 100); // wait 1ms // short delay by weicong@2013.11.4

		//GOSUB set_offsetd 			 0x08	;
		//ENTRY  &dll
		temp = Inp32(PHY_BASE+0X0028)&0xFFFFFF00;
		temp = temp | 0x08;
		Outp32(PHY_BASE+0X0028, temp);

		//DMC_Delay(1000);  //1ms
		//no need by WZW@2013.11.6  DMC_Delay(IROM_ARM_CLK, 100); // wait 1ms // short delay by weicong@2013.11.4

		//GOSUB ca_deskew_code 0x0						;;- if high freq.
		//ENTRY &code
		temp = ((0x0&0xf)<<28)|(0x0<<21)|(0x0<<14)|(0x6<<7)|0x0;
		Outp32(PHY_BASE+0X0080, temp);

		//DMC_Delay(IROM_ARM_CLK, 1000); // wait 1m////////////////////////////////////added by WangCL 2013.11.01

		temp = ((0x0&0x1)<<31)|(0x0<<24)|(0x0<<17)|(0x0<<10)|(0x0<<3)|(0x0>>4)&0x3;
		Outp32(PHY_BASE+0X0084, temp);

		//DMC_Delay(IROM_ARM_CLK, 1000); // wait 1m////////////////////////////////////added by WangCL 2013.11.01

		temp = (0x0>>1)&0x3f;
		Outp32(PHY_BASE+0X0088, temp);

		//no need by WZW@2013.11.6 DMC_Delay(IROM_ARM_CLK, 100); // wait 1m////////////////////////////////////added by WangCL 2013.11.01 // short delay by weicong@2013.11.4

		//GOSUB dll_on_start									;;- dll on & start

		temp = Inp32(PHY_BASE+0X0030)&0xFFFFFF9F;
		Outp32(PHY_BASE+0X0030, temp);

		DMC_Delay(IROM_ARM_CLK, 1); // wait 1m////////////////////////////////////added by WangCL 2013.11.01 // short delay by weicong@2013.11.4

		temp = temp|0X20;
		Outp32(PHY_BASE+0X0030, temp);

		//DMC_Delay(1000);  //1ms
		//DMC_Delay(IROM_ARM_CLK, 1000); // wait 1ms
		//temp =temp&0xFFFFFFBF;
		//Outp32(PHY_BASE+0X0030, temp);
		temp =temp|0x40;
		Outp32(PHY_BASE+0X0030, temp);
		//DMC_Delay(1000);  //1ms
		//DMC_Delay(IROM_ARM_CLK, 1000); // wait 1ms
		while(temp!=0x5)
		{
			i = Inp32(PHY_BASE+0X0034)&0xFFFFFF9F;
			temp = i & 0x5; 	                                //Modified 20131101
		}

		//GOSUB fp_resync										;- fp_resync
		temp = Inp32(DMC_BASE+0X0018)|0X8;
		Outp32(DMC_BASE+0X0018, temp);
////////////////////////////////20131106 by wzw  		//DMC_Delay(IROM_ARM_CLK, 1000); // wait 1m////////////////////////////////////added by WangCL 2013.11.01
		temp=temp&0xFFFFFFF7;
		Outp32(DMC_BASE+0X0018, temp);

	//;---------------------------------------------------------------------------------
	//;- disable ctrl_atgate
	//;----------------------------------------------------------------------------
	//GOSUB set_ctrl_at_gate		0x0 					;;- disable ctrl_atgate
	temp = Inp32(PHY_BASE+0X0000)&0xFFFFFFBF;
	Outp32(PHY_BASE+0X0000, temp);

	//;---------------------------------------------------------------------------------
	//;- Set auto refresh enable
	//;----------------------------------------------------------------------------
	//GOSUB aref_en										;;- enable aref
	temp = Inp32(DMC_BASE+0X0000)|0x20;
	Outp32(DMC_BASE+0X0000, temp);

	//GOSUB send_precharge_all							;;- send precharge all command
	port = 0;
	while(port<0x1)
	{
		pall=0x01000000;
		if (port==1)
		{
			 pall=pall|0x10000000;
		}

		cs=0;
		while(cs<0x2)
		{
			if (cs==1)
			{
				pall=pall|0x100000;
			}

			Outp32(DMC_BASE+0x0010, pall);

			//DMC_Delay(1000);  //1ms
			//DMC_Delay(IROM_ARM_CLK, 1000); // wait 1ms

			cs=cs+1;
		}

		port=port+1;
	}

	//;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	//;- CLK Pause enable
	//GOSUB write 0x105C1094 0x1		"CLK Pause Enable"
	//;---------------------------------------------------------------------
	Outp32(0x105C1094, 0x1);
}
#endif

void dram_init_w1(u32 nMEMCLK)
{
#if defined(CONFIG_SMC_CMD)
	reg_arr_t reg_arr;
#endif
	struct exynos4_power *pmu = (struct exynos4_power *)EXYNOS4_POWER_BASE;
	u32 temp, uBits;
	u32 i,j;
	u32 mem_var,bl_var,pzq_val;
	u32 row_param, data_param, power_param, rfcpb;
	u32 port, cs, nop, mr63, mr10, mr1, mr2, mr3, pall;
	u32 clk_ds, cke_ds, cs_ds, ca_ds, data_ds, zq_dds, zq_term;
	volatile unsigned int eBootStat;

	eBootStat = readl(&pmu->inform1);

	//;-----------------------------------------------
	//;- CA SWAP Setting..!
	//;-----------------------------------------------
#if 0
	temp = Inp32(DMC_BASE+0X0000)&(~0x00000001)|(0x00000001);
	Outp32(DMC_BASE+0X0000, temp);

	temp = Inp32(PHY_BASE+0X0064)&(~0x00000001)|(0x00000001);
	Outp32(PHY_BASE+0X0064, temp);
#endif

	set_mem_clock(50);

	//;---------------------------------------------------------------------
	//;- Set the memory type for PHY register
	//;----------------------------------------------------------------------
	phy_mem_type(LPDDR3); //LPDDR3


	//;---------------------------------------------------------------------
	//;- pulldown setting for DQ/DQS
	//;----------------------------------------------------------------------
	pulldown_dq_dqs(pulldown_dq, pulldown_dqs);


	//;-------------------------------------------------------------------------
	//;-  PHY Burst Length/Read, write latency setting
	//;---------------------------------------------------------------------------
	phy_set_bl_rl_wl(LPDDR3, nMEMCLK); //mem:0=LPDDR3, 1=DDR3, 2 =LPDDR2


	//;-------------------------------------------------------------------------------
	//;- dmc_term
	//;-------------------------------------------------------------------------------
	temp=Inp32(DMC_BASE+0x0018);
	temp=temp&0xFFFF1FFF;
	temp=temp|(0x5<<12);
	Outp32(DMC_BASE+0x0018,temp);

	//---------------------------------------------------------------------------------
	//;- Set rd_fetch parameter
	//;----------------------------------------------------------------------------
	//GOSUB dmc_rd_fetch		&rd_fetch
	//&rd_fetch=0x2
	temp = Inp32(DMC_BASE+0X0000);
	temp = temp &0xFFFF8FFF;
	temp = temp | ((0x2&0x7)<<12);  //rd_fetch=0x2
	Outp32(DMC_BASE+0X0000, temp);

	//---------------------------------------------------------------------------------
	//- Assert the ConControl.dfi_init_start field then deassert
	//----------------------------------------------------------------------------
	//GOSUB dfi_init_start 		0x1						;;- dfi_init_start
	temp = Inp32(DMC_BASE+0X0000)|0x10000000;
	temp= temp&0xFFFFFFDF; 					  // Auto Refresh counter disable
	Outp32(DMC_BASE+0X0000, temp);

	DMC_Delay(10);
	while(Inp32(0x105f0040)&0x00000008!=0x8); //wait dfi_init_complete
	temp=temp&0xEFFFFFFF;
	Outp32(DMC_BASE+0X0000, temp);

	//GOSUB fp_resync									;;- fp_resync
	temp = Inp32(DMC_BASE+0X0018);
	temp = temp |0x8;
	Outp32(DMC_BASE+0X0018, temp);

	temp=temp&0xFFFFFFF7;
	Outp32(DMC_BASE+0X0018, temp);

	//;---------------------------------------------------------------------------------
	//;- Set the memory base register
	//;----------------------------------------------------------------------------
	i = cs0_base&0x7ff;
	j = cs0_mask & 0x7ff;
	temp = 0;
	temp=temp|(i<<16);
	temp=temp|j;
#if defined(CONFIG_SMC_CMD)
	reg_arr.set0.addr = TZASC_BASE+0x0F00;
	reg_arr.set0.val = temp;

	set_secure_reg((u32)&reg_arr, 1);
#else
	Outp32(TZASC_BASE+0X0F00, temp);
#endif

	//;---------------------------------------------------------------------------------
	//;- Set the memory control register
	//;----------------------------------------------------------------------------
	if (mem_type==2)  //lpddr2
	{
		mem_var=0x5;
		bl_var=0x2;
	}
	else if (mem_type==0)  //lpddr3
	{
		mem_var=0x7;
		bl_var=0x3;
	}
	else if (mem_type==1)  //ddr3
	{
		mem_var=0x6;
		bl_var=0x3;
	}

	bl_var=bl_var&0x7;
	mem_var=mem_var&0xF;
	//pzq=pzq&0x1;
	//&chip=&chip&0xF
	//&add_pall=&add_pall&0xF

	temp=0;
	temp=temp|0x00002000;    //mem_width
	temp=temp|(0<<24);		 //pzq_en
	temp=temp|(bl_var<<20);  //if memory type is LPDDR3,  bl is 8
	temp=temp|(0<<16);		 //num_chip, 0x0=1chip, 0x1=2chips
	temp=temp|(mem_var<<8);	 // mem_tpye, 0x7=LPDDR3
	temp=temp|(0<<6);        //add_lat_pall, 0x0=0cycle, 0x1=1cycle, 0x2=2cycle
	Outp32(DMC_BASE+0X0004, temp);

	//;---------------------------------------------------------------------------------
	//;- Set the memory config register
	//;----------------------------------------------------------------------------
	temp = 0;
	temp=temp|(0x2<<12);  //chip_map, 0x2 = Split column interleaved
	temp=temp|(0x3<<8);   //chip_col, 0x3 = 10bits
	temp=temp|(0x2<<4);   //chip_row, 0x2 = 14bits
	temp=temp|(0x3);      //chip_bank, 0x3 = 8banks
#if defined(CONFIG_SMC_CMD)
	reg_arr.set0.addr = TZASC_BASE+0x0F10;
	reg_arr.set0.val = temp;

	set_secure_reg((u32)&reg_arr, 1);
#else
	Outp32(TZASC_BASE+0X0F10, temp);
#endif
	//Outp32(TZASC_BASE+0X0F14, temp);

	//;---------------------------------------------------------------------------------
	//;- Address mapping method setting
	//;----------------------------------------------------------------------------
#if defined(CONFIG_SMC_CMD)
	temp = read_secure_reg(TZASC_BASE+0X0F10) & 0xFF80FFFF;
#else
	temp = Inp32(TZASC_BASE+0X0F10)&0xFF80FFFF;
#endif
	temp=temp|(0X4<<20);   //bank_lsb, 0x4 = bit position [12](column low size = 4KB)
	temp=temp|(0<<19);     //rank_inter_en,
	temp=temp|(0X1<<18);   //bit_sel_en
	temp=temp|(0X2<<16);   //bit_sel, 0x2 = bit postion = [23:22](if rank_inter_en is enabled, [24:22])
#if defined(CONFIG_SMC_CMD)
	reg_arr.set0.addr = TZASC_BASE+0x0F10;
	reg_arr.set0.val = temp;

	set_secure_reg((u32)&reg_arr, 1);
#else
	Outp32(TZASC_BASE+0X0F10, temp);
#endif

	//;---------------------------------------------------------------------------------
	//;- Set auto refresh interval
	//;----------------------------------------------------------------------------
	temp=0;
	temp=temp|(0XC<<16); // t_refipb = 0.4875us * 24MHz = 11.7 = C
	temp=temp|0X5D;  // t_refi = 3.9us * 24MHz = 93 = 0x5D
	Outp32(DMC_BASE+0X0030, temp);


	//;----------timing switch setting-------------------------
	//GOSUB timing_set_con  &timing_set_sw &timing_set_sw_con
	//;----------timing switch setting-------------------------
	temp = (0x0<<4)|0x0;
	//GOSUB write &DMC_BASE+0x00E0 &temp
	Outp32(DMC_BASE+0X00E0, temp);
	SetBits(0x10020054,31,0x1,0x0);

	//;---------------------------------------------------------------------------------
	//;- Set timing_refpb, timing_row, timing_data, timing_power
	//;----------------------------------------------------------------------------
	//GOSUB dmc_timing_parameter	&mclk					;;- set timing parameter
	if (nMEMCLK==400)
		{
		rfcpb=(12<<8)|(12<<0);
		row_param=0x1A35538A;
		data_param=0x23200539;
		power_param=0x281C0225;
		}
	else if (nMEMCLK==533)
		{
		rfcpb=(16<<8)|(16<<0);
		row_param=0x2347648D;
		data_param=0x24200539;
		power_param=0x38260225;
		}
		Outp32(DMC_BASE+0X0020, rfcpb);
		Outp32(DMC_BASE+0X0034, row_param);
		Outp32(DMC_BASE+0X0038, data_param);
		Outp32(DMC_BASE+0X003c, power_param);
		Outp32(DMC_BASE+0X00e4, row_param);
		Outp32(DMC_BASE+0X00e8, data_param);
		Outp32(DMC_BASE+0X00ec, power_param);

		//;---------------------------------------------------------------------------------
		//;- low frequency operation
		//;----------------------------------------------------------------------------
		temp = 0x7f|(0x7f<<8)|(0x7f<<16)|(0x7f<<24);
		Outp32(PHY_BASE+0X0010, temp);

		temp = 0x7f|(0x7f<<8)|(0x7f<<16)|(0x7f<<24);
		Outp32(PHY_BASE+0X0018, temp);

		temp = Inp32(PHY_BASE+0X0028)&0xFFFFFF00;
		temp = temp | 0x7f;
		Outp32(PHY_BASE+0X0028, temp);

		temp = ((0x60&0xf)<<28)|(0x60<<21)|(0x60<<14)|(0x60<<7)|0x60;
		Outp32(PHY_BASE+0X0080, temp);

		temp = ((0x60&0x1)<<31)|(0x60<<24)|(0x60<<17)|(0x60<<10)|(0x60<<3)|(0x60>>4)&0x3;
		Outp32(PHY_BASE+0X0084, temp);

		temp = (0x60>>1)&0x3f;
		Outp32(PHY_BASE+0X0088, temp);

		i = 0x7f<<0x08;
		temp = Inp32(PHY_BASE+0X0030)&0xFFFF80FF;
		i = i | temp;
		Outp32(PHY_BASE+0X0030, i);

		temp = Inp32(PHY_BASE+0X0030)&0xFFFFFFDF;
		Outp32(PHY_BASE+0X0030, temp);

		//GOSUB fp_resync										;- fp_resync
		temp = Inp32(DMC_BASE+0X0018)|0X8;
		Outp32(DMC_BASE+0X0018, temp);
		temp=temp&0xFFFFFFF7;
		Outp32(DMC_BASE+0X0018, temp);

		//;-------------------------------------------------------------------------
		//;- Ch/CLK/CKE/CS/CA DDS Setting
		//;----------------------------------------------------------------------------
		clk_ds=0x4&0x7;
		cke_ds=0x4&0x7;
		cs_ds=0x4&0x7;
		ca_ds=0x4&0x7;
		data_ds=0x4&0x7;

		temp = Inp32(PHY_BASE+0X00A0);          // Read PHY_CON39
		temp=temp|(data_ds<<25)|(data_ds<<22)|(data_ds<<19)|(data_ds<<16)|(clk_ds<<9)|(cke_ds<<6)|(cs_ds<<3)|ca_ds;
		Outp32( PHY_BASE+0x00A0, temp );	//- PHY0.CON39[11:0]=0xDB60DB6

		//;------------------------------------------------------------------------
		//;- ZQ_INIT/ZQ_DDS Setting.
		//;------------------------------------------------------------------------
		zq_dds=0x4&0x7;
		zq_term=0x0&0x7;
		temp = Inp32(PHY_BASE+0X0040);
		temp=temp|0x08040000;
		temp=temp|0x00080000; // if zq_term == 0,
		temp=temp|(zq_dds<<24)|(zq_term<<21);
		//Outp32( 0x106D0000+0x0040, 0xE0C0304 );	//- dds=0x6000000, term=0x0
		Outp32( PHY_BASE+0x0040, temp );	//- dds=0x6000000, term=0x0
		temp=temp|0x4;
		//Outp32( 0x106D0000+0x0040, 0xE0C0304 );	//- long cal.
		Outp32( PHY_BASE+0x0040, temp );  	//- long cal.
		temp=temp|0x2;
		//Outp32( 0x106D0000+0x0040, 0xE0C0306 );	//- manual zq cal. start
		Outp32( PHY_BASE+0x0040, temp );  	//- manual zq cal. start
		while( ( Inp32( PHY_BASE+0x0048 ) & 0x1 ) != 0x1 );	//- PHY0: wait for zq_done

		temp = Inp32(PHY_BASE+0X0040);
		temp=temp&0xFFFBFFFD;				//termination disable, clk_div_en disable,calibration manual start disble
		Outp32( 0x106D0000+0x0040, temp );	//- ternination disable, clk div disable, manual calibration stop

	//;-------------------------------------------------------------------------
	//;- Issue command to intial memory(DDR)
	//;----------------------------------------------------------------------------
	if(eBootStat != S5P_CHECK_SLEEP)
	{
		port=0;
		while (port<0x1)
		{
		nop=0x07000000;
		mr63=0x00071C00;
		mr10=0x00010BFC; // MR10, OP=0xFF "Calibration command after initialization"

		if (nMEMCLK==400)
		{
			mr1=0x0000060C; // nWR = tWR / tCK = 15ns / 2.5ns = 6, BL = 8
			mr2=0x0000081C;	// RL = 9, WL = 5 because constraint of memory controller.
		}
		else if (nMEMCLK==533)
		{
			mr1=0x0000070C;
			mr2=0x0000081C;
		}

		mr3=0x00000C0C;		//; 0xC04 : 34.3, 0xC0C=48

		if (port==1)
			{
			nop=nop|0x10000000;
			mr63=mr63|0x10000000;
			mr10=mr10|0x10000000;
			mr1=mr1|0x10000000;
			mr2=mr2|0x10000000;
			mr3=mr3|0x10000000;
			}

		cs=0;
		while (cs<0x2)
		{
			if (cs==1)
			{
				nop=nop|0x100000;
				mr63=mr63|0x100000;
				mr10=mr10|0x100000;
				mr1=mr1|0x100000;
				mr2=mr2|0x100000;
				mr3=mr3|0x100000;
			}

			Outp32(DMC_BASE+0X0010, nop);
			DMC_Delay(200);

			Outp32(DMC_BASE+0X0010, mr63);
			#if 0
			DMC_Delay(10);
			#else
			DMC_Delay(1);
			Check_MRStatus(0,0);
			#endif

			Outp32(DMC_BASE+0X0010, mr10);
			//DMC_Delay(1);

			Outp32(DMC_BASE+0X0010, mr1);
			Outp32(DMC_BASE+0X0010, mr2);
			Outp32(DMC_BASE+0X0010, mr3);
			cs=cs+1;
			}
		port=port+1;
		}
	}
	else
	{
		Outp32(DMC_BASE+0X0010, 0x08000000);	//- CH0, CS0. Self Refresh Exit Command for only sleep & wakeup
	}
	//;-------------------------------------------------------------------------
	//;- Set memory clock to normal frequency
	//;----------------------------------------------------------------------------
	set_mem_clock(nMEMCLK);

	//;---------------------------------------------------------------------------------
	//;- normal frequency operation
	//;----------------------------------------------------------------------------
		temp = 0x08|(0x08<<8)|(0x08<<16)|(0x08<<24);
		Outp32(PHY_BASE+0X0010, temp);

		//GOSUB set_offsetw 			0x08 0x08 0x08 0x08	;
		//ENTRY &dq3 &dq2 &dq1 &dq0
		temp = 0x08|(0x08<<8)|(0x08<<16)|(0x08<<24);
		Outp32(PHY_BASE+0X0018, temp);

		//GOSUB set_offsetd 			 0x08	;
		//ENTRY  &dll
		temp = Inp32(PHY_BASE+0X0028)&0xFFFFFF00;
		temp = temp | 0x08;
		Outp32(PHY_BASE+0X0028, temp);

		//GOSUB ca_deskew_code 0x0						;;- if high freq.
		//ENTRY &code
		temp = ((0x0&0xf)<<28)|(0x0<<21)|(0x0<<14)|(0x0<<7)|0x0;
		Outp32(PHY_BASE+0X0080, temp);

		temp = ((0x0&0x1)<<31)|(0x0<<24)|(0x0<<17)|(0x0<<10)|(0x0<<3)|(0x0>>4)&0x3;
		Outp32(PHY_BASE+0X0084, temp);

		temp = (0x0>>1)&0x3f;
		Outp32(PHY_BASE+0X0088, temp);

		//GOSUB dll_on_start									;;- dll on & start

		temp = Inp32(PHY_BASE+0X0030)&0xFFFFFF9F;
		Outp32(PHY_BASE+0X0030, temp);

		temp = temp|0X20;
		Outp32(PHY_BASE+0X0030, temp);
		//DMC_Delay(IROM_ARM_CLK, 1000); // wait 1ms

		temp =temp|0x40;
		Outp32(PHY_BASE+0X0030, temp);
		//DMC_Delay(IROM_ARM_CLK, 2000); // wait 1ms

		while(temp!=0x5)
		{
			i = Inp32(PHY_BASE+0X0034);
			temp = i & 0x5;
		}

		//GOSUB fp_resync										;- fp_resync
		temp = Inp32(DMC_BASE+0X0018)|0X8;
		Outp32(DMC_BASE+0X0018, temp);
		temp=temp&0xFFFFFFF7;
		Outp32(DMC_BASE+0X0018, temp);

	//;---------------------------------------------------------------------------------
	//;- disable ctrl_atgate
	//;----------------------------------------------------------------------------
	//GOSUB set_ctrl_at_gate		0x0 					;;- disable ctrl_atgate
		temp = Inp32(PHY_BASE+0X0000)&0xFFFFFFBF;
		Outp32(PHY_BASE+0X0000, temp);

	//;---------------------------------------------------------------------------------
	//;- Set timeout precharge
	//;----------------------------------------------------------------------------
		temp = Inp32(DMC_BASE+0X0014); // read DREX.PrechConfig0
		temp=temp&0x0FFFFFFF;
		temp=temp|(0xf<<28);     // tp_en
		Outp32(DMC_BASE+0X0014, temp);

		temp = Inp32(DMC_BASE+0X001C); // read DREX.PrechConfig1
		temp=temp&0x0;
		temp=temp|0xffffffff;     //  tp_cnt
		Outp32(DMC_BASE+0X001C, temp);

	//;-------------------------------------------------------------------------------
	//;- dynamic_sref
	//;-------------------------------------------------------------------------------
		temp = Inp32(DMC_BASE+0X0004);
		temp=temp&0xFFFFFFDF;
		temp=temp|(0x1<<5);
		Outp32(DMC_BASE+0X0004, temp); //"MemControl"

	//;-------------------------------------------------------------------------------
	//;- dynamic_pwrdn
	//;-------------------------------------------------------------------------------
		temp = Inp32(DMC_BASE+0X0004);
		temp=temp&0xFFFFFFF1;
		temp=temp|(0x1<<1);
		Outp32(DMC_BASE+0X0004, temp); //"MemControl"

	//;-------------------------------------------------------------------------------
	//;- dmc_brb_control
	//;- dmc_brb_config
	//;-------------------------------------------------------------------------------
		temp = (0x0<<7)|(0x0<<6)|(0x0<<5)|(0x0<<4)|(0x0<<3)|(0x0<<2)|(0x0<<1)|(0x0);
		Outp32(DMC_BASE+0X0100, temp); //"DREX0 brbrsvcontrol""
		temp = (0x8<<28)|(0x8<<24)|(0x8<<20)|(0x8<<16)|(0x8<<12)|(0x8<<8)|(0x8<<4)|(0x8);
		Outp32(DMC_BASE+0X0104, temp); //"DREX0 brbrsvconfig""

	//;-------------------------------------------------------------------------------
	//;- dynamic_clkstop
	//;-------------------------------------------------------------------------------
		temp = Inp32(DMC_BASE+0X0004);
		temp=temp&0xFFFFFFFE;
		temp=temp|0x0;
		Outp32(DMC_BASE+0X0004, temp); //MemControl - dynamic_clkstop

		temp = Inp32(DMC_BASE+0X0008);
		temp=temp&0xFFFFFFEF;
		temp=temp|(0x1<<5);
		Outp32(DMC_BASE+0X0008, temp); //"DMC.CGControl.tz_cg_en=&tz_cg_en"

		temp = Inp32(DMC_BASE+0X0008);
		temp=temp&0xFFFFFFEF;
		temp=temp|(0x1<<4);
		Outp32(DMC_BASE+0X0008, temp); //"DMC.CGControl.phy_cg_en=&phy_cg_en"

		temp = Inp32(DMC_BASE+0X0008);
		temp=temp&0xFFFFFFF7;
		temp=temp|(0x1<<3);
		Outp32(DMC_BASE+0X0008, temp); //"DMC.CGControl.mem_if_cg_en=&mem_if_cg_en"

		temp = Inp32(DMC_BASE+0X0008);
		temp=temp&0xFFFFFFFB;
		temp=temp|(0x1<<2);
		Outp32(DMC_BASE+0X0008, temp); //"DMC.CGControl.scg_cg_en=&scg_cg_en"

		temp = Inp32(DMC_BASE+0X0008);
		temp=temp&0xFFFFFFFD;
		temp=temp|(0x1<<1);
		Outp32(DMC_BASE+0X0008, temp); //"DMC.CGControl.busif_wr_cg_en=&busif_wr_cg_en"

		temp = Inp32(DMC_BASE+0X0008);
		temp=temp&0xFFFFFFFE;
		temp=temp|(0x1<<0);
		Outp32(DMC_BASE+0X0008, temp); //"DMC0.CGControl.busif_rd_cg_en=&busif_rd_cg_en"

	//;-------------------------------------------------------------------------------
	//;- dmc_io_pd
	//;-------------------------------------------------------------------------------
		temp = Inp32(DMC_BASE+0X0000);
		temp=temp&0xFFFFFF3F;
		temp=temp|(0x2<<6);			   // 0x2 = Automatic control for ctrl_pd in none read state,
		Outp32(DMC_BASE+0X0000, temp); //"Set DREX0.ConControl.io_pd_con=&pd"

	//;---------------------------------------------------------------------------------
	//;- Set auto refresh enable
	//;----------------------------------------------------------------------------
	//GOSUB aref_en										;;- enable aref
		temp = Inp32(DMC_BASE+0X0000)|0x20;
		Outp32(DMC_BASE+0X0000, temp);

	//;---------------------------------------------------------------------------------
	//;- Set update_mode enable
	//;----------------------------------------------------------------------------
		temp = Inp32(DMC_BASE+0X0000)|0x8;				//0x1 = MC initiated update/acknowledge mode
		Outp32(DMC_BASE+0X0000, temp);

	//GOSUB send_precharge_all							;;- send precharge all command
		port = 0;
	while(port<0x1)
	{
		pall=0x01000000;

		if (port==1)
		{
			 pall=pall|0x10000000;
		}

		cs=0;
		while(cs<0x2)
		{
			if (cs==1)
			{
				pall=pall|0x100000;
			}

			Outp32(DMC_BASE+0x0010, pall);

			cs=cs+1;
		}

		port=port+1;
	}
	//;---------------------------------------------------------------------
	//;- ctrl_ref set to resolve for bus hang issue
	//;---------------------------------------------------------------------
	temp = Inp32(PHY_BASE+0X0030)|(0xf<<1);				//0xf = 4'b1111: Once ctrl_locked and dfi_init_complete are asserted, those won't be deasserted until rst_n is asserted.
	Outp32(PHY_BASE+0X0030, temp);

	//;---------------------------------------------------------------------
	//;- CLK Pause enable
	//;---------------------------------------------------------------------
	Outp32(0x105C1094, 0x1);
}

void mem_ctrl_init(void)
{
	u32 MemClk = 400;
	dram_init_w1(MemClk);
}
