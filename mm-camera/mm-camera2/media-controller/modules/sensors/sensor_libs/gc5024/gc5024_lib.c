/*============================================================================
 
  Copyright (c) 2013 Qualcomm Technologies, Inc. All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.

============================================================================*/
#include <stdio.h>
#include "sensor_lib.h"
#include "camera_dbg.h"
//#include <utils/Log.h>


#define SENSOR_MODEL_NO_GC5024 "gc5024"
#define GC5024_LOAD_CHROMATIX(n) \
  "libchromatix_"SENSOR_MODEL_NO_GC5024"_"#n".so"

//#define IMAGE_NORMAL
#define IMAGE_H_MIRROR 
//#define IMAGE_V_MIRROR 
//#define IMAGE_HV_MIRROR

#ifdef IMAGE_NORMAL
#define MIRROR 		  0xd4
#define PH_SWITCH 	  0x1b
#define BLK_VAL_H 	  0x3c
#define BLK_VAL_L 	  0x00
#define STARTX 		  0x0d
#define STARTY 		  0x03
#endif
#ifdef IMAGE_H_MIRROR
#define MIRROR 		  0xd5
#define PH_SWITCH 	  0x1a
#define BLK_VAL_H 	  0x3c
#define BLK_VAL_L 	  0x00
#define STARTX 		  0x02
#define STARTY 		  0x03
#endif
#ifdef IMAGE_V_MIRROR
#define MIRROR 		  0xd6
#define PH_SWITCH 	  0x1b
#define BLK_VAL_H 	  0x00
#define BLK_VAL_L 	  0x3c
#define STARTX 		  0x0d
#define STARTY 		  0x02
#endif
#ifdef IMAGE_HV_MIRROR
#define MIRROR  	  0xd7
#define PH_SWITCH 	  0x1a
#define BLK_VAL_H 	  0x00
#define BLK_VAL_L 	  0x3c
#define STARTX 		  0x02
#define STARTY 		  0x02
#endif



#define ANALOG_GAIN_1 64   // 1.000x
#define ANALOG_GAIN_2 88   // 1.375x
#define ANALOG_GAIN_3 121  // 1.891x 
#define ANALOG_GAIN_4 168  // 2.625x
#define ANALOG_GAIN_5 239  // 3.734x
#define ANALOG_GAIN_6 336  // 5.250x
#define ANALOG_GAIN_7 481  // 7.516x




static sensor_lib_t sensor_lib_ptr;

static struct msm_sensor_power_setting power_setting[] = {
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_STANDBY,
		.config_val = GPIO_OUT_HIGH,
		.delay = 10,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_LOW,
		.delay = 10,
	},
	{
		.seq_type = SENSOR_VREG,
		.seq_val = CAM_VIO,
		.config_val = 0,
		.delay = 10,
	},
    {
        .seq_type = SENSOR_GPIO,
        .seq_val = SENSOR_GPIO_VDIG,
        .config_val = GPIO_OUT_HIGH,
        .delay = 10,
    },
	{
		.seq_type = SENSOR_VREG,
		.seq_val = CAM_VANA,
		.config_val = 0,
		.delay = 10,
	},
	{
		.seq_type = SENSOR_CLK,
		.seq_val = SENSOR_CAM_MCLK,
		.config_val = 24000000,
		.delay = 5,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_STANDBY,
		.config_val = GPIO_OUT_LOW, 
		.delay = 10,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_HIGH,
		.delay = 10,
	},
	{
		.seq_type = SENSOR_I2C_MUX,
		.seq_val = 0,
		.config_val = 0,
		.delay = 0,
	},

    {
        .seq_type = SENSOR_GPIO,
        .seq_val = SENSOR_GPIO_VAF,
        .config_val = GPIO_OUT_HIGH,
        .delay = 10,
    },
	
};

static struct msm_sensor_power_setting power_down_setting[] = {
	{
		.seq_type = SENSOR_I2C_MUX,
		.seq_val = 0,
		.config_val = 0,
		.delay = 0,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_LOW,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_STANDBY,
		.config_val = GPIO_OUT_HIGH,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_CLK,
		.seq_val = SENSOR_CAM_MCLK,
		.config_val = 24000000,
		.delay = 5,
	},
    {
        .seq_type = SENSOR_VREG,
        .seq_val = CAM_VANA,
        .config_val = 0,
        .delay = 1,
    },
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_VDIG,
		.config_val = GPIO_OUT_LOW,
		.delay = 5,
	},
	{
		.seq_type = SENSOR_VREG,
		.seq_val = CAM_VIO,
		.config_val = 0,
		.delay = 10,
	},
	
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_VAF,
		.config_val = GPIO_OUT_LOW,
		.delay = 1,
	},
	
};

static struct msm_camera_sensor_slave_info sensor_slave_info = {
  /* Camera slot where this camera is mounted */
  .camera_id = CAMERA_0,
  /* sensor slave address */
  .slave_addr = 0x6e,
  /*sensor i2c frequency*/
  .i2c_freq_mode = I2C_FAST_MODE,
  /* sensor address type */
  .addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
  /* sensor id info*/
  .sensor_id_info = {
  /* sensor id register address */
  .sensor_id_reg_addr = 0xf0,
  /* sensor id */
  .sensor_id = 0x5024,
  },
  /* power up / down setting */
  .power_setting_array = {
    .power_setting = power_setting,
    .size = ARRAY_SIZE(power_setting),
    .power_down_setting = power_down_setting,
    .size_down = ARRAY_SIZE(power_down_setting),
  },
};

static struct msm_sensor_init_params sensor_init_params = {
  .modes_supported = 0,
  .position = 0,
  .sensor_mount_angle = 90,
};


static sensor_output_t sensor_output = {
  .output_format = SENSOR_BAYER,
  .connection_mode = SENSOR_MIPI_CSI,
  .raw_output = SENSOR_10_BIT_DIRECT,
};

static struct msm_sensor_output_reg_addr_t output_reg_addr = {
  .x_output = 0xff,
  .y_output = 0xff,
  .line_length_pclk = 0xff,
  .frame_length_lines = 0xff,
};

static struct msm_sensor_exp_gain_info_t exp_gain_info = {
  .coarse_int_time_addr = 0x03,
  .global_gain_addr = 0xb2,
  .vert_offset = 0,
};

static sensor_aec_data_t aec_info = {
  .max_gain = 5.0,
  .max_linecount = 8000, //4x2000
};

static sensor_lens_info_t default_lens_info = {
  .focal_length = 2.93,
  .pix_size = 1.75,
  .f_number = 2.8,
  .total_f_dist = 1.2,
  .hor_view_angle = 54.8,
  .ver_view_angle = 42.5,
};

static struct csi_lane_params_t csi_lane_params = {
  .csi_lane_assign = 0x4320,
  .csi_lane_mask = 0x7,
  .csi_if = 1,
  .csid_core = {0},
  .csi_phy_sel = 0,
};

static struct msm_camera_i2c_reg_array init_reg_array0[] = {
	  /*SYS*/
	  {0xfe,0x80},
	  {0xfe,0x80},
      {0xfe,0x80},
	  {0xf7,0x01},
	  {0xf8,0x0e}, //0x0e
	  {0xf9,0xae},
	  {0xfa,0x84},
	  {0xfc,0xae},	 
	  {0x88,0x03},
	  {0xe7,0xc0},
	  /*Analog*/
	  {0xfe,0x00},
	  {0x03,0x08},
	  {0x04,0xca},
	  {0x05,0x01},
	  {0x06,0xf4},
	  {0x07,0x00},
	  {0x08,0x1e},//0x0e
	  {0x0a,0x00},
	  {0x0c,0x00},
	  {0x0d,0x07},
	  {0x0e,0xa8},
	  {0x0f,0x0a},
	  {0x10,0x40},	  
	  {0x11,0x31},
	  {0x12,0x26},
	  {0x13,0x10},
	  {0x17,MIRROR},
	  {0x18,0x02},
	  {0x19,0x0d},
	  {0x1a,PH_SWITCH},
	  {0x1b,0x41},
	  {0x1c,0x2b},
	  {0x21,0x0f},
	  {0x24,0xb0},
	  {0x29,0x38},
	  {0x2d,0x16},
	  {0x2f,0x16},
	  {0x32,0x49}, 
	  {0xcd,0xaa},
	  {0xd0,0xc2},
	  {0xd1,0xc4},
	  {0xd2,0xcb},
	  {0xd3,0x73},
	  {0xd8,0x18},
	  {0xdc,0xba},
	  {0xe2,0x20},
	  {0xe4,0x78},
	  {0xe6,0x08},
	  /*ISP*/
	  {0x80,0x50},//50
	  {0x8d,0x07},	
	  {0x90,0x01},
	  {0x92,STARTY},
	  {0x94,STARTX},
	  {0x95,0x07},
	  {0x96,0x98},
	  {0x97,0x0a},
	  {0x98,0x20},
	  /*Gain*/
	  {0x99,0x01},
	  {0x9a,0x02},
	  {0x9b,0x03},
	  {0x9c,0x04},
	  {0x9d,0x0d},
	  {0x9e,0x15},
	  {0x9f,0x1d},
	  {0xb0,0x4b},
	  {0xb1,0x01},
	  {0xb2,0x00},
	  {0xb6,0x00},
	  /*Blk*/
	  {0x40,0x22},
	  {0x4e,BLK_VAL_H},
	  {0x4f,BLK_VAL_L},
	  {0x60,0x00},
	  {0x61,0x80},
	  {0xfe,0x02},
	  {0xa4,0x30},
	  {0xa5,0x00},
	  /*Dark Sun*/
	  {0x40,0x96},
	  {0x42,0x0f},
	  {0x45,0xca},
	  {0x47,0xff},
	  {0x48,0xc8},
	  /*DD*/
	  {0x80,0x10}, 
	  {0x81,0x50}, 
	  {0x82,0x60},
	  {0x84,0x20}, 
	  {0x85,0x10},
	  {0x86,0x04},
	  {0x87,0x20}, 
	  {0x88,0x10},
	  {0x89,0x04},
	  /*Degrid*/
	  {0x8a,0x0a},
	  /*MIPI*/
	  {0xfe,0x03},
	  {0x01,0x07}, 
	  {0x02,0x56}, //0x34
	  {0x03,0x15}, //0x13
	  {0x04,0x04}, 
	  {0x05,0x00},
	  {0x06,0x80},
	  {0x11,0x2b},
	  {0x12,0xa8},
	  {0x13,0x0c},
	  {0x15,0x00},
      {0x16,0x09},
	  {0x18,0x01},
	  {0x21,0x10},
	  {0x22,0x05},
	  {0x23,0x30},
	  {0x24,0x10},
	  {0x25,0x14},
	  {0x26,0x08},
	  {0x29,0x05},
	  {0x2a,0x0a},
	  {0x2b,0x08},
	  {0x42,0x20},
	  {0x43,0x0a},
	  {0xfe,0x00}, 
};


static struct msm_camera_i2c_reg_setting init_reg_setting[] = {
  {
    .reg_setting = init_reg_array0,
    .size = ARRAY_SIZE(init_reg_array0),
    .addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
    .data_type = MSM_CAMERA_I2C_BYTE_DATA,
    .delay = 1,//50,
  },
};

static struct sensor_lib_reg_settings_array init_settings_array = {
  .reg_settings = init_reg_setting,
  .size = 1,
};

static struct msm_camera_i2c_reg_array start_reg_array[] = {
     {0xfe,0x03},
     {0x10,0x91},
     {0xfe,0x00},
};

static  struct msm_camera_i2c_reg_setting start_settings = {
  .reg_setting = start_reg_array,
  .size = ARRAY_SIZE(start_reg_array),
  .addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
  .data_type = MSM_CAMERA_I2C_BYTE_DATA,
  .delay = 1, //10,
};

static struct msm_camera_i2c_reg_array stop_reg_array[] = {
     {0xfe,0x03},
     {0x10,0x00},
     {0xfe,0x00},

};

static struct msm_camera_i2c_reg_setting stop_settings = {
  .reg_setting = stop_reg_array,
  .size = ARRAY_SIZE(stop_reg_array),
  .addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
  .data_type = MSM_CAMERA_I2C_BYTE_DATA,
  .delay = 1, //10,
};

static struct    msm_camera_i2c_reg_array groupon_reg_array[] = {
    {0xfe, 0x00},
};

static struct msm_camera_i2c_reg_setting groupon_settings = {
  .reg_setting = groupon_reg_array,
  .size = ARRAY_SIZE(groupon_reg_array),
  .addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
  .data_type = MSM_CAMERA_I2C_BYTE_DATA,
  .delay = 0,
};

static struct msm_camera_i2c_reg_array groupoff_reg_array[] = {
    {0xfe, 0x00},
};

static struct msm_camera_i2c_reg_setting groupoff_settings = {
  .reg_setting = groupoff_reg_array,
  .size = ARRAY_SIZE(groupoff_reg_array),
  .addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
  .data_type = MSM_CAMERA_I2C_BYTE_DATA,
  .delay = 0,
};

static struct msm_camera_csid_vc_cfg gc5024_cid_cfg[] = {
  {0, CSI_RAW10, CSI_DECODE_10BIT},
  {1, CSI_EMBED_DATA, CSI_DECODE_8BIT},
};

static struct msm_camera_csi2_params gc5024_csi_params = {
  .csid_params = {
    .lane_cnt = 2,
    .lut_params = {
      .num_cid = ARRAY_SIZE(gc5024_cid_cfg),
      .vc_cfg = {
         &gc5024_cid_cfg[0],
         &gc5024_cid_cfg[1],
      },
    },
  },
  .csiphy_params = {
    .lane_cnt = 2,
    .settle_cnt = 0x14,//120ns
  },
};

struct sensor_pix_fmt_info_t rgb10[] =
{  //only a simbol rgb10
    {V4L2_PIX_FMT_SBGGR10},
};

struct sensor_pix_fmt_info_t meta[] =
{//only a simbol meta
    {MSM_V4L2_PIX_FMT_META},
};



static sensor_stream_info_t gc5024_stream_info[] = {
  {1, &gc5024_cid_cfg[0], rgb10},
  {1, &gc5024_cid_cfg[1], meta},
};

static sensor_stream_info_array_t gc5024_stream_info_array = {
  .sensor_stream_info = gc5024_stream_info,
  .size = ARRAY_SIZE(gc5024_stream_info),
};


static struct msm_camera_i2c_reg_array res0_reg_array[] = {
/* lane snap */
     {0xfe,0x00},

};

static struct msm_camera_i2c_reg_array res1_reg_array[] = {
/*  preveiw */
     {0xfe,0x00},

};

static struct msm_camera_i2c_reg_setting res_settings[] = {
{  //capture
    .reg_setting = res0_reg_array,
    .size = ARRAY_SIZE(res0_reg_array),
    .addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
    .data_type = MSM_CAMERA_I2C_BYTE_DATA,
    .delay = 0,
  },
  {//preview
    .reg_setting = res1_reg_array,
    .size = ARRAY_SIZE(res1_reg_array),
    .addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
    .data_type = MSM_CAMERA_I2C_BYTE_DATA,
    .delay = 0,
  },
};

static struct sensor_lib_reg_settings_array res_settings_array = {
  .reg_settings = res_settings,
  .size = ARRAY_SIZE(res_settings),
};


static struct msm_camera_csi2_params *csi_params[] = {
  &gc5024_csi_params, /* RES 0*/
  &gc5024_csi_params, /* RES 1*/
};

static struct sensor_lib_csi_params_array csi_params_array = {
  .csi2_params = &csi_params[0],
  .size = ARRAY_SIZE(csi_params),
};


static struct sensor_crop_parms_t crop_params[] = {
  {0, 0, 0, 0}, /* RES 0 */
  {0, 0, 0, 0}, /* RES 1 */
};


static struct sensor_lib_crop_params_array crop_params_array = {
  .crop_params = crop_params,
  .size = ARRAY_SIZE(crop_params),
};

static struct sensor_lib_out_info_t sensor_out_info[] = {
  {
/* For SNAPSHOT */
    .x_output = 2592,
    .y_output = 1944,
    .line_length_pclk =   2000,
    .frame_length_lines = 1500,  
    .vt_pixel_clk = 90000000,
    .op_pixel_clk = 320001000,
	.binning_factor = 1,    
    .max_fps = 30.0,
    .min_fps = 8,   
    .mode = SENSOR_DEFAULT_MODE,
  },
/* For PREVIEW */
  {

    .x_output = 2592,
    .y_output = 1944,
    .line_length_pclk =   2000,
    .frame_length_lines = 1500, 
    .vt_pixel_clk = 90000000,
    .op_pixel_clk = 228000000,  
	.binning_factor = 1,  
    .max_fps = 30.0,  
    .min_fps = 8,    
    .mode = SENSOR_DEFAULT_MODE,
  },
};

static struct sensor_lib_out_info_array out_info_array = {
  .out_info = sensor_out_info,
  .size = ARRAY_SIZE(sensor_out_info),
};

static sensor_res_cfg_type_t gc5024_res_cfg[] = {
  SENSOR_SET_STOP_STREAM,
  SENSOR_SET_NEW_RESOLUTION, /* set stream config */
  SENSOR_SET_CSIPHY_CFG,
  SENSOR_SET_CSID_CFG,
  SENSOR_LOAD_CHROMATIX, /* set chromatix prt */
  SENSOR_SEND_EVENT, /* send event */
  SENSOR_SET_START_STREAM,
};

static struct sensor_res_cfg_table_t gc5024_res_table = {
  .res_cfg_type = gc5024_res_cfg,
  .size = ARRAY_SIZE(gc5024_res_cfg),
};

static struct sensor_lib_chromatix_t gc5024_chromatix[] = {
  {
    .common_chromatix = GC5024_LOAD_CHROMATIX(common),
    .camera_preview_chromatix = GC5024_LOAD_CHROMATIX(preview), /* RES0 */
    .camera_snapshot_chromatix = GC5024_LOAD_CHROMATIX(preview), /* RES0 */
    .camcorder_chromatix = GC5024_LOAD_CHROMATIX(preview), /* RES0 */
  },
  {
    .common_chromatix = GC5024_LOAD_CHROMATIX(common),
    .camera_preview_chromatix = GC5024_LOAD_CHROMATIX(preview), /* RES0 */
    .camera_snapshot_chromatix = GC5024_LOAD_CHROMATIX(preview), /* RES0 */
    .camcorder_chromatix = GC5024_LOAD_CHROMATIX(preview), /* RES0 */
  },
};

static struct sensor_lib_chromatix_array gc5024_lib_chromatix_array = {
  .sensor_lib_chromatix = gc5024_chromatix,
  .size = ARRAY_SIZE(gc5024_chromatix),
};

/*===========================================================================
 * FUNCTION    - gc5024_real_to_register_gain -
 *
 * DESCRIPTION:
 *==========================================================================*/
static uint16_t gc5024_real_to_register_gain(float gain)
{
    uint16_t reg_gain;
    if (gain < 1.0)
        gain = 1.0;
    if (gain > 5.0)
        gain = 5.0;
//    ALOGE("gc5024_PETER,real_gain=%f" , gain);
    reg_gain = (uint16_t)(gain * 64.0f);
    return reg_gain;

}

/*===========================================================================
 * FUNCTION    - gc5024_register_to_real_gain -
 *
 * DESCRIPTION:
 *==========================================================================*/
static float gc5024_register_to_real_gain(uint16_t reg_gain)
{
    float gain;
    if (reg_gain > 0x0140)
        reg_gain = 0x0140;
//    ALOGE("gc5024_PETER register_gain=%u" , reg_gain);
    gain = (float)(reg_gain/64.0f);
    return gain;

}

/*===========================================================================
 * FUNCTION    - gc5024_calculate_exposure -
 *
 * DESCRIPTION:
 *==========================================================================*/
static int32_t gc5024_calculate_exposure(float real_gain,
  uint16_t line_count, sensor_exposure_info_t *exp_info)
{
  if (!exp_info) {
    return -1;
  }
  exp_info->reg_gain = gc5024_real_to_register_gain(real_gain);
  float sensor_real_gain = gc5024_register_to_real_gain(exp_info->reg_gain);
  exp_info->digital_gain = real_gain / sensor_real_gain;
  exp_info->line_count = line_count;
  return 0;
}

/*===========================================================================
 * FUNCTION    - gc5024_fill_exposure_array -
 *
 * DESCRIPTION:
 *==========================================================================*/

static int32_t gc5024_fill_exposure_array(uint16_t gain, uint32_t line,
  uint32_t fl_lines,int32_t luma_avg, uint32_t fgain,
   struct msm_camera_i2c_reg_setting *reg_setting)
{
	int32_t rc = 0;

	uint16_t reg_count = 0;
	uint16_t gain_b6,gain_b1,gain_b2;
	uint16_t iReg,temp,line_temp;
	int32_t i;


    //ALOGE("GC5024 DEVIN ,fl_lines=%d,gain=%d, line=%d\n",fl_lines,gain,line);
	
	if (1 == line && gain <= 64)
		return rc;	

	if (!reg_setting) {
		return -1;
	}
	
	reg_setting->reg_setting[reg_count].reg_addr =0xfe;
	reg_setting->reg_setting[reg_count].reg_data =0x00;
	reg_count++;

	iReg = gain;
	if(iReg < 0x40)
		iReg = 0x40;
	if((ANALOG_GAIN_1<= iReg)&&(iReg < ANALOG_GAIN_2))  //ANALOG_GAIN_1 == 64
	{
		reg_setting->reg_setting[reg_count].reg_addr =0x21;
		reg_setting->reg_setting[reg_count].reg_data =0x0f;
		reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0x29;
		reg_setting->reg_setting[reg_count].reg_data =0x0f;
		reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xe8;
		reg_setting->reg_setting[reg_count].reg_data =0x03;
		reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xe9;
		reg_setting->reg_setting[reg_count].reg_data =0x01;
		reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xea;
		reg_setting->reg_setting[reg_count].reg_data =0x02;
	    reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xeb;
		reg_setting->reg_setting[reg_count].reg_data =0x02;
	    reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xec;
		reg_setting->reg_setting[reg_count].reg_data =0x03;
		reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xed;
		reg_setting->reg_setting[reg_count].reg_data =0x01;
		reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xee;
		reg_setting->reg_setting[reg_count].reg_data =0x02;
	    reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xef;
		reg_setting->reg_setting[reg_count].reg_data =0x02;
	    reg_count++;
		//analog gain
        gain_b6 = 0x00;   
        temp = iReg;     
        gain_b1 = temp>>6;
        gain_b2 = (temp<<2)&0xfc;			
	}
	else if((ANALOG_GAIN_2<= iReg)&&(iReg < ANALOG_GAIN_3))
	{
		reg_setting->reg_setting[reg_count].reg_addr =0x21;
		reg_setting->reg_setting[reg_count].reg_data =0x0f;
		reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0x29;
		reg_setting->reg_setting[reg_count].reg_data =0x1b;
		reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xe8;
		reg_setting->reg_setting[reg_count].reg_data =0x03;
		reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xe9;
		reg_setting->reg_setting[reg_count].reg_data =0x01;
		reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xea;
		reg_setting->reg_setting[reg_count].reg_data =0x02;
	    reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xeb;
		reg_setting->reg_setting[reg_count].reg_data =0x02;
	    reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xec;
		reg_setting->reg_setting[reg_count].reg_data =0x03;
		reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xed;
		reg_setting->reg_setting[reg_count].reg_data =0x01;
		reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xee;
		reg_setting->reg_setting[reg_count].reg_data =0x02;
	    reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xef;
		reg_setting->reg_setting[reg_count].reg_data =0x02;
	    reg_count++;
		//analog gain
	  	gain_b6 = 0x01;   
	  	temp = 64*iReg/ANALOG_GAIN_2;    
	  	gain_b1 = temp>>6;
	  	gain_b2 = (temp<<2)&0xfc;		        	
	}
  	else if((ANALOG_GAIN_3<= iReg)&&(iReg < ANALOG_GAIN_4))
	{
		reg_setting->reg_setting[reg_count].reg_addr =0x21;
		reg_setting->reg_setting[reg_count].reg_data =0x0b;
		reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0x29;
		reg_setting->reg_setting[reg_count].reg_data =0x1b;
		reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xe8;
		reg_setting->reg_setting[reg_count].reg_data =0x03;
		reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xe9;
		reg_setting->reg_setting[reg_count].reg_data =0x01;
		reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xea;
		reg_setting->reg_setting[reg_count].reg_data =0x02;
	    reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xeb;
		reg_setting->reg_setting[reg_count].reg_data =0x02;
	    reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xec;
		reg_setting->reg_setting[reg_count].reg_data =0x03;
		reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xed;
		reg_setting->reg_setting[reg_count].reg_data =0x01;
		reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xee;
		reg_setting->reg_setting[reg_count].reg_data =0x02;
	    reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xef;
		reg_setting->reg_setting[reg_count].reg_data =0x02;
	    reg_count++;
		//analog gain           
	  	gain_b6 = 0x02;   
	  	temp = 64*iReg/ANALOG_GAIN_3;   
	  	gain_b1 = temp>>6;
	  	gain_b2 = (temp<<2)&0xfc;
	 	}
	else if((ANALOG_GAIN_4<= iReg)&&(iReg < ANALOG_GAIN_5))
	{
		reg_setting->reg_setting[reg_count].reg_addr =0x21;
		reg_setting->reg_setting[reg_count].reg_data =0x0d;
		reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0x29;
		reg_setting->reg_setting[reg_count].reg_data =0x1d;
		reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xe8;
		reg_setting->reg_setting[reg_count].reg_data =0x02;
		reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xe9;
		reg_setting->reg_setting[reg_count].reg_data =0x02;
		reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xea;
		reg_setting->reg_setting[reg_count].reg_data =0x02;
	    reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xeb;
		reg_setting->reg_setting[reg_count].reg_data =0x02;
	    reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xec;
		reg_setting->reg_setting[reg_count].reg_data =0x02;
		reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xed;
		reg_setting->reg_setting[reg_count].reg_data =0x02;
		reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xee;
		reg_setting->reg_setting[reg_count].reg_data =0x02;
	    reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xef;
		reg_setting->reg_setting[reg_count].reg_data =0x02;
	    reg_count++;
		//analog gain
	  	gain_b6 = 0x03;   
	  	temp = 64*iReg/ANALOG_GAIN_4;    
	  	gain_b1 = temp>>6;
	  	gain_b2 = (temp<<2)&0xfc;	        		
	} 
	else if((ANALOG_GAIN_5<= iReg)&&(iReg < ANALOG_GAIN_6))
	{
		reg_setting->reg_setting[reg_count].reg_addr =0x21;
		reg_setting->reg_setting[reg_count].reg_data =0x08;
		reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0x29;
		reg_setting->reg_setting[reg_count].reg_data =0x38;
		reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xe8;
		reg_setting->reg_setting[reg_count].reg_data =0x03;
		reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xe9;
		reg_setting->reg_setting[reg_count].reg_data =0x03;
		reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xea;
		reg_setting->reg_setting[reg_count].reg_data =0x03;
	    reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xeb;
		reg_setting->reg_setting[reg_count].reg_data =0x03;
	    reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xec;
		reg_setting->reg_setting[reg_count].reg_data =0x03;
		reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xed;
		reg_setting->reg_setting[reg_count].reg_data =0x03;
		reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xee;
		reg_setting->reg_setting[reg_count].reg_data =0x03;
	    reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xef;
		reg_setting->reg_setting[reg_count].reg_data =0x03;
	    reg_count++;
		//analog gain
	  	gain_b6 = 0x04;   
	  	temp = 64*iReg/ANALOG_GAIN_5;    
	  	gain_b1 = temp>>6;
	  	gain_b2 = (temp<<2)&0xfc;	        		
	} 
	else if((ANALOG_GAIN_6<= iReg)&&(iReg < ANALOG_GAIN_7))
	{
		reg_setting->reg_setting[reg_count].reg_addr =0x21;
		reg_setting->reg_setting[reg_count].reg_data =0x08;
		reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0x29;
		reg_setting->reg_setting[reg_count].reg_data =0x38;
		reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xe8;
		reg_setting->reg_setting[reg_count].reg_data =0x03;
		reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xe9;
		reg_setting->reg_setting[reg_count].reg_data =0x03;
		reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xea;
		reg_setting->reg_setting[reg_count].reg_data =0x03;
	    reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xeb;
		reg_setting->reg_setting[reg_count].reg_data =0x03;
	    reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xec;
		reg_setting->reg_setting[reg_count].reg_data =0x03;
		reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xed;
		reg_setting->reg_setting[reg_count].reg_data =0x03;
		reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xee;
		reg_setting->reg_setting[reg_count].reg_data =0x03;
	    reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xef;
		reg_setting->reg_setting[reg_count].reg_data =0x03;
	    reg_count++;
		//analog gain
	  	gain_b6 = 0x05;   
	  	temp = 64*iReg/ANALOG_GAIN_6;    
	  	gain_b1 = temp>>6;
	  	gain_b2 = (temp<<2)&0xfc;	        		
	}	
	else if(ANALOG_GAIN_7<= iReg)
	{
		reg_setting->reg_setting[reg_count].reg_addr =0x21;
		reg_setting->reg_setting[reg_count].reg_data =0x08;
		reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0x29;
		reg_setting->reg_setting[reg_count].reg_data =0x38;
		reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xe8;
		reg_setting->reg_setting[reg_count].reg_data =0x03;
		reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xe9;
		reg_setting->reg_setting[reg_count].reg_data =0x03;
		reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xea;
		reg_setting->reg_setting[reg_count].reg_data =0x03;
	    reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xeb;
		reg_setting->reg_setting[reg_count].reg_data =0x03;
	    reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xec;
		reg_setting->reg_setting[reg_count].reg_data =0x03;
		reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xed;
		reg_setting->reg_setting[reg_count].reg_data =0x03;
		reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xee;
		reg_setting->reg_setting[reg_count].reg_data =0x03;
	    reg_count++;
		
		reg_setting->reg_setting[reg_count].reg_addr =0xef;
		reg_setting->reg_setting[reg_count].reg_data =0x03;
	    reg_count++;
		//analog gain
	  	gain_b6 = 0x06;   
	  	temp = 64*iReg/ANALOG_GAIN_7;    
	  	gain_b1 = temp>>6;
	  	gain_b2 = (temp<<2)&0xfc;	        		
	}	

  


/***********************Shutter Start***************************************/
	if (!line) line = 1; /* avoid 0 */	
	if(line < 1) line = 1; /*anti color deviation on shot expoure*/
	if(line > 16383) line = 16383;

    if (line<=20)
	{
	    reg_setting->reg_setting[reg_count].reg_addr =0x32;
		reg_setting->reg_setting[reg_count].reg_data =0x09;
		reg_count++;	
		
	    reg_setting->reg_setting[reg_count].reg_addr =0xb0;
		reg_setting->reg_setting[reg_count].reg_data =0x53;
		reg_count++;
	}
	else
	{
	    reg_setting->reg_setting[reg_count].reg_addr =0x32;
		reg_setting->reg_setting[reg_count].reg_data =0x49;
		reg_count++;	
		
	    reg_setting->reg_setting[reg_count].reg_addr =0xb0;
		reg_setting->reg_setting[reg_count].reg_data =0x4b;
		reg_count++;
	}
	
    reg_setting->reg_setting[reg_count].reg_addr =
    	sensor_lib_ptr.exp_gain_info->coarse_int_time_addr;
  	reg_setting->reg_setting[reg_count].reg_data = (line & 0x3F00) >> 8;
  	reg_count++;

  	reg_setting->reg_setting[reg_count].reg_addr =
    	sensor_lib_ptr.exp_gain_info->coarse_int_time_addr + 1;
  	reg_setting->reg_setting[reg_count].reg_data = line & 0xFF;
  	reg_count++;




  /***********************Shutter End***************************************/

    reg_setting->reg_setting[reg_count].reg_addr =
    	sensor_lib_ptr.exp_gain_info->global_gain_addr + 4;
  	reg_setting->reg_setting[reg_count].reg_data = gain_b6; //0xb6
  	reg_count++;
  

  
    reg_setting->reg_setting[reg_count].reg_addr =
    	sensor_lib_ptr.exp_gain_info->global_gain_addr - 1;  //0xb1
  	reg_setting->reg_setting[reg_count].reg_data = gain_b1;
  	reg_count++;
  
  
    reg_setting->reg_setting[reg_count].reg_addr =
   	 	sensor_lib_ptr.exp_gain_info->global_gain_addr;
  	reg_setting->reg_setting[reg_count].reg_data = gain_b2;  //0xb2
  	reg_count++;
  
    
    reg_setting->size = reg_count;
    reg_setting->addr_type = MSM_CAMERA_I2C_BYTE_ADDR;
    reg_setting->data_type = MSM_CAMERA_I2C_BYTE_DATA;
    reg_setting->delay = 0;



  return rc;
}

static sensor_exposure_table_t gc5024_expsoure_tbl = {
  .sensor_calculate_exposure = gc5024_calculate_exposure,
  .sensor_fill_exposure_array = gc5024_fill_exposure_array,
};

static sensor_lib_t sensor_lib_ptr = {
  /* sensor slave info */
  .sensor_slave_info = &sensor_slave_info,
  /* sensor init params */
  .sensor_init_params = &sensor_init_params,
  /* sensor actuator name */
  .actuator_name = "dw9714",
  /* sensor output settings */
  .sensor_output = &sensor_output,
  /* sensor output register address */
  .output_reg_addr = &output_reg_addr,
  /* sensor exposure gain register address */
  .exp_gain_info = &exp_gain_info,
  /* sensor aec info */
  .aec_info = &aec_info,
  /* sensor snapshot exposure wait frames info */
  .snapshot_exp_wait_frames = 1,
  /* number of frames to skip after start stream */
  .sensor_num_frame_skip = 1,  // 1  peter
  /* sensor exposure table size */
  .exposure_table_size = 20,
  /* sensor lens info */
  .default_lens_info = &default_lens_info,
  /* csi lane params */
  .csi_lane_params = &csi_lane_params,
  /* csi cid params */
  .csi_cid_params = gc5024_cid_cfg,
  /* csi csid params array size */
  .csi_cid_params_size = ARRAY_SIZE(gc5024_cid_cfg),
  /* init settings */
  .init_settings_array = &init_settings_array,
  /* start settings */
  .start_settings = &start_settings,
  /* stop settings */
  .stop_settings = &stop_settings,
  /* group on settings */
  .groupon_settings = &groupon_settings,
  /* group off settings */
  .groupoff_settings = &groupoff_settings,
  /* resolution cfg table */
  .sensor_res_cfg_table = &gc5024_res_table,
  /* res settings */
  .res_settings_array = &res_settings_array,
  /* out info array */
  .out_info_array = &out_info_array,
  /* crop params array */
  .crop_params_array = &crop_params_array,
  /* csi params array */
  .csi_params_array = &csi_params_array,
  /* sensor port info array */
  .sensor_stream_info_array = &gc5024_stream_info_array,
  /* exposure funtion table */
  .exposure_func_table = &gc5024_expsoure_tbl,
  /* chromatix array */
  .chromatix_array = &gc5024_lib_chromatix_array,
};

/*===========================================================================
 * FUNCTION    - SKUAA_ST_gc5024_open_lib -
 *
 * DESCRIPTION:
 *==========================================================================*/
void *gc5024_open_lib(void)
{
 // CDBG("gc5024_open_lib is called");
  return &sensor_lib_ptr;
}
