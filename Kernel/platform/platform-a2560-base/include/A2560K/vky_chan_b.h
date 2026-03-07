/**
 * @file vky_chan_b.h
 *
 * Define register addresses needed for A2560K channel B text driver
 */

#ifndef __VKY_CHAN_B_H
#define __VKY_CHAN_B_H

/** Master Control Register for Channel B, and its supported bits */
#define VKY3_B_MCR          ((volatile unsigned long *)0xFEC80000)
#define VKY3_B_MCR_TEXT     0x00000001  /**< Text mode enable bit */
#define VKY3_B_MCR_TXT_OVR  0x00000002  /**< Text overlay enable bit */
#define VKY3_B_MCR_GRAPHICS 0x00000004  /**< Graphics mode enable bit */
#define VKY3_B_MCR_BITMAP   0x00000008  /**< Bitmap engineg enable bit */
#define VKY3_B_MCR_TILE     0x00000010  /**< Tile engine enable bit */
#define VKY3_B_MCR_SPRITE   0x00000020  /**< Sprite engine enable bit */
#define VKY3_B_MCR_BLANK    0x00000080  /**< Disable display engine enable bit */
#define VKY3_B_MODE0        0x00000100  /**< Video Mode Bit 0 */
#define VKY3_B_MODE1        0x00000200  /**< Video Mode Bit 1 */
#define VKY3_B_DOUBLE       0x00000400  /**< Pixel Double Enable bit */
#define VKY3_B_HIRES        0x40000000  /**< DIP switch for hires mode */
#define VKY3_B_PLL          0x00000800  /**< Controls dot clock */
#define VKY3_B_MCR_SLEEP    0x00040000  /**< Monitor sleep (synch disable) bit */
#define VKY3_B_CLK40        0x80000000  /**< Indicate if PLL is 25MHz (0) or 40MHz (1) */

/** Border control register for Channel B */
#define VKY3_B_BCR          ((volatile unsigned long *)0xFEC80004)
#define VKY3_B_BCR_ENABLE   0x00000001  /**< Bit to enable the display of the border */

/** Border color register for Channel B */
#define VKY3_B_BRDCOLOR     ((volatile unsigned long *)0xFEC80008)

/** Cursor Control Register for Channel B */
#define VKY3_B_CCR          ((volatile unsigned long *)0xFEC80010)
#define VKY3_B_CCR_ENABLE   0x00000001  /**< Bit to enable the display of the cursor */
#define VKY3_B_CCR_RATE0    0x00000002  /**< Bit0 to specify the blink rate */
#define VKY3_B_CCR_RATE1    0x00000004  /**< Bit1 to specify the blink rate */

/** Cursor Position Register for Channel B */
#define VKY3_B_CPR          ((volatile unsigned long *)0xFEC80014)

/** Font memory block for Channel B */
#define VKY3_B_FONT_MEMORY  ((volatile unsigned char *)0xFEC88000)

/** Text Matrix for Channel B */
#define VKY3_B_TEXT_MATRIX  ((volatile unsigned char *)0xFECA0000)

/** Color Matrix for Channel B */
#define VKY3_B_COLOR_MATRIX ((volatile unsigned char *)0xFECA8000)

/* Text Color LUTs for Channel B */
#define VKY3_B_LUT_SIZE     16
#define VKY3_B_TEXT_LUT_FG  ((volatile unsigned long *)0xFECAC400)  /**< Text foreground color look up table for channel B */
#define VKY3_B_TEXT_LUT_BG  ((volatile unsigned long *)0xFECAC440)  /**< Text background color look up table for channel B */

/*


// Video Clock Synced
assign Mstr_Ctrl_Text_Mode_Enable_o             = Master_Control_Reg[0];                // VICKY_MASTER_REG[0][0];
assign Mstr_Ctrl_Text_Mode_Overlay_o            = Master_Control_Reg[1];                // VICKY_MASTER_REG[0][1];
// CPU Clock Synced
assign Mstr_Ctrl_Graphic_Mode_Enable_o          = Master_Control_Reg[2];                // Graphics Mode On
assign Mstr_Ctrl_Bitmap_Enable_o                = Master_Control_Reg[3];                // VICKY_MASTER_REG[0][3];
assign Mstr_Ctrl_TileMap_Enable_o               = Master_Control_Reg[4];                // VICKY_MASTER_REG[0][4];
assign Mstr_Ctrl_Sprite_Enable_o                = Master_Control_Reg[5];                // VICKY_MASTER_REG[0][5];
assign Mstr_Ctrl_GAMMA_Enable_o                 = Mstr_Ctrl_GAMMA_int_extern ? Master_Control_Reg[6] : DIPSwitch_GAMMA_i;     //Master_Control_Reg_VidClk[2][17] : DIPSwitch_GAMMA_i;
assign Mstr_Ctrl_Disable_Video_o                = Master_Control_Reg[7];                 // VICKY_MASTER_REG[0][7];
assign Mstr_Ctrl_Video_Mode_o[1:0]              = Master_Control_Reg[9:8];               // Video Clock Synced
assign Mstr_Ctrl_Pixel_Division_o               = Master_Control_Reg[11:10];             // 00: Full Resolution, 01: divided by 2, 10: divided by 4
assign Mstr_Ctrl_Turn_Off_Sync_o                = !Master_Control_Reg[12];               // Turn-Off Sync to get the Monitor to Sleep
assign Mstr_Ctrl_FONT_Show_BG_in_Overlay_o      = Master_Control_Reg[13];                // Overlay with BG See-through
assign Mstr_Ctrl_MemText_Enable_o               = Master_Control_Reg[14] & !Master_Control_Reg[20];     // Memtext Mode ON <<<<< NEW
assign Mstr_Ctrl_MemText_ShowBG_o               = Master_Control_Reg[15];                // Memtext Mode BG See-Through <<<< NEW (Not tested)
assign Mstr_Ctrl_Game_GUI_Mode_o                = Master_Control_Reg[16];                // GUI = 0, GAME = 1 ????
assign Mstr_Ctrl_GAMMA_int_extern               = Master_Control_Reg[17];                // 0 = DipSwitch, 1 = Register Choice
// Nothing Yet @ VICKY_MASTER_REG[0][18];
// Nothing Yet @ VICKY_MASTER_REG[0][19];
assign Mstr_Ctrl_TOS_Graph_Enable_o                =  Master_Control_Reg[20] & !Master_Control_Reg[14];                // Enable EMUTOS Bitmap Mode (Mutually Exclusive with memtext)

// Nothing Yet @ VICKY_MASTER_REG[0][23];
assign Mstr_Ctrl_Game_Layer0_Enable_o            = Master_Control_Reg[24];                // Game Engine Layer0 Enable
assign Mstr_Ctrl_Game_Layer0_Type_o                = Master_Control_Reg[25];                // Game Engine Layer0 Type (0 = Bitmap, 1 = TileMap)
assign Mstr_Ctrl_Game_Layer1_Enable_o            = Master_Control_Reg[26];                // Game Engine Layer1 Enable
assign Mstr_Ctrl_Game_Layer1_Type_o                = Master_Control_Reg[27];                // Game Engine Layer1 Type (0 = Bitmap, 1 = TileMap)
assign Mstr_Ctrl_Game_Layer2_Enable_o            = Master_Control_Reg[28];                // Game Engine Layer2 Enable
assign Mstr_Ctrl_Game_Layer2_Type_o                = Master_Control_Reg[29];                // Game Engine Layer2 Type (0 = Bitmap, 1 = TileMap)
assign Mstr_Ctrl_Game_Layer3_Enable_o            = Master_Control_Reg[30];                // Game Engine Layer3 Enable
assign Mstr_Ctrl_Game_Layer3_Type_o                = Master_Control_Reg[31];                // Game Engine Layer3 Type (0 = Bitmap, 1 = TileMap)


*/

#endif
