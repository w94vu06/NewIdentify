/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.cpp
 * Author: user
 *
 * Created on 2015年12月18日, 下午 2:18
 */

#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <cstring>
#include <complex>
#include <list>
#include <limits>
#include <jni.h>
#include <android/log.h>

using namespace std;

#define PI							(3.14159265)
#define POST_LEN_MAX                256                 // the max length of the parameters from post
#define MaxPacketSize				96
#define ECGCellSize					136		            // MaxPacketSize/3*2*2+8, unit: byte
#define EEPROM_ECG_SIZE				12032
#define FileFrameSize				(ECGCellSize*188)	// 188 (ECG cell number per frame): EEPROM_ECG_SIZE/2/(MaxPacketSize/3)
#define ECG_FRAME_SIZE			    (EEPROM_ECG_SIZE/2)	// ECG資料一個Frame中的資料點數目
#define DS_DOWNSAMPLE_R				4					// The downsampling ratio for the Dual-Slope QRS Detection
#define DS_FRAME_SIZE				(ECG_FRAME_SIZE/16/DS_DOWNSAMPLE_R)	// The frame size of the Dual-Slope QRS Detection
#define MaxBufferSize				2016                // (MaxBufferSize*RawRatio)須被(MaxPacketSize/3)整除
#define RawRatio					2
#define ECGADC_MAX					4096		        // The max value of ECG data from ADC
#define ECGADC_MAX_F				(4096.0)	        // The max value of ECG data from ADC (float-point)
#define ECGADC_MIDDLE				2048		        // The middle value of ECG data from ADC
#define ECGADC_MIDDLE_F				(2048.0)	        // The middle value of ECG data from ADC (float-point)
#define ECG_ANALY_SIZE				3000                // The number of ECG record used in the parameter mode.
#define ECG_ANALY_REWIND			3050                // The number of ECG records rewinded from
#define POWERFFT_ANALY_SIZE			256		            // The number of ECG record used in power FFT (must be 2^N).
#define POWERFFT_SIZE_LOG			8		            // Binary log of POWERFFT_ANALY_SIZE.
#define POWERFFT_SAMPLE_STEP		7		            // FFT的sample step.
#define NOTCH_SPAN					2	                // notch filter的階數
#define NOTCH_PREPAD_SIZE			1000				// Notch filter超前處理的心電圖資料點數(為了消除Frame一開始的震盪)
#define DS_SLOPE_SIZE				16					// The max width of a slope of the two slopes beside the R point in the Dual-Slope QRSDetection (=63/DS_DOWNSAMPLE_R+1)
#define DS_SLOPE_MIN				6					// The min width of a slope of the two slopes beside the R point in the Dual-Slope QRSDetection (=27/DS_DOWNSAMPLE_R)
#define DS_HIGH_SEARCH_SIZE			10					// The max width of the search areas of the two sides of the R point for the R height (unit: the same as DS_SLOPE_SIZE)
#define DS_THETA_DIFF_0				(3840.0)			// The 0 level of the threshold value of Sdiff_max (Multiplied by the sampling frequency)
#define DS_THETA_DIFF_1				(4352.0)			// The 1 level of the threshold value of Sdiff_max (Multiplied by the sampling frequency)
#define DS_THETA_DIFF_2				(7680.0)			// The 2 level of the threshold value of Sdiff_max (Multiplied by the sampling frequency)
#define DS_S_AVE_COMP_0				(12800.0)			// The first threshold of the average of Sdiff_max for the choice of the level of Theta_diff (Multiplied by the sampling frequency) (COMP: compare)
#define DS_S_AVE_COMP_1				(20480.0)			// The second threshold of the average of Sdiff_max for the choice of the level of Theta_diff (Multiplied by the sampling frequency)
#define DS_S_LR_MIN_MIN				(1536.0)			// The min of the samller one of the left and right slopes of R point (Multiplied by the sampling frequency)
#define DS_SDIFF_AVE_N				8					// The buffer size for Sdiff_max for average
#define DS_HIGH_AVE_RAT				(0.6)				// The ratio of the ECG peak average for the threshold of ECG peak high (Criterion 3)
//#define S_DIFFDIFF_HSPAN			((DS_SLOPE_SIZE - DS_SLOPE_MIN) / 2 + 1)	// The half span of the difference calculation of Sdiff_max of the Dual-Slope QRSDetection
#define S_DIFFDIFF_HSPAN			2					// The half span of the difference calculation of Sdiff_max of the Dual-Slope QRSDetection
#define DS_LAST_R_DIST_MIN			(DS_SLOPE_SIZE * 3)		// The min of the distance of the current R point candidate and the last R point (The R candidate whose distance is smaller than this will be dropped)
#define DS_FRA_ST_LEFTSH			(DS_SLOPE_SIZE + S_DIFFDIFF_HSPAN - 1 + 1)		// The distance of left-shift from the start of a Dual-Slope frame to locate the start index of R position labeling
#define BWR_FILTER_SIZE				800				// Baseline Wandering Removal 取平均的資料長度(單位: 1ms有一點)
#define BWR_S2_SIZE					200				// Baseline Wandering Removal 取平均(stage 2, 即第二次)的資料長度(單位: 1ms有一點)(0表示不做第二次平均)
#define ADC_TO_MV					(0.004545881212)	// The constant transferring A/D convertor output to voltage value (unit: mV).
#define FIR_COEFF_N					21					// FIR線性相位濾波器tap的個數
#define FIR_RECORD_STEP				1					// FIR線性相位濾波器每隔幾個取樣點起一個出來濾波
#define HRV_WIN_UP_LIMIT			250					// HRV中，超過此值的Heart rate會被忽略
#define HRV_WIN_LOW_LIMIT			28					// HRV中，低於此值的Heart rate會被忽略
#define HRV_AVGWIN_UPLIM			100					// HRV中，超出平均值多少百分比的Heart rate會被忽略
#define LD_DOWNSAMP_RATIO			4					// Down-sampling ratio of the time axis for lond-period display (LD: Long Display)
#define LD_FRAME_N_IN_PAGE			15					// The number of frames in a page in the Long Display area
#define ST_FRAME_FST_PAGE			2					// The index of the start frame of the first page. (The first frame index: 0)
//#define LD_VALUE_LOW_LIMIT			1792				// The lower limit of the ECG data which are transmitted to Internet Browser - (2048 - 256)
#define LD_VALUE_LOW_LIMIT			0				// The lower limit of the ECG data which are transmitted to Internet Browser - (2048 - 256)
#define LD_VALUE_RANGE_LIMIT		4095					// The range limit of the ECG data which are transmitted to Internet Browser
//#define LD_VALUE_DOWNSAMP_R			(2.4)					// The ratio of down-sampling of the value of ECG data which are transmitted to Internet Browser
#define LD_VALUE_DOWNSAMP_R			(1.0)					// The ratio of down-sampling of the value of ECG data which are transmitted to Internet Browser
#define CHA_FNAME_LEN_MAX			4095				// The max length of the file name of the input .cha file
#define RR_N_MAX_4UID				10000				// The max number of the RR intervals in the RR array for User Identification

// Constants for ECG features for User Identification
#define EMBEDDING_DIMENSION 10
#define DELAY 1
#define RADIUS_START 0.05
#define RADIUS_END 1.0
#define RADIUS_STEP 0.1
#define C1A_THRESHOLD 0 // 0 milliseconds

// copied from our ECG web page
#define ECGDgmWidth                 500                 // The width of the ECG display region

#define HR_LEADSEL_MAX				160					// The max heart rate for Good Heart Rate for Lead Selection
//#define HR_LEADSEL_MAX			250					// The max heart rate for Good Heart Rate for Lead Selection (250: Lead Selection
// turned off)
#define HR_LEADSEL_MIN				20					// The min heart rate for Good Heart Rate for Lead Selection
#define HR_GOOD_THRESH				5					// The threshold value of the number of the Good Heart rates for Lead Selection
#define HR_GOOD_RATIO_TRSH			(0.85)				// The threshold value of the ratio of the Good HRs to the Total HRs for Lead Selection

#define HRV_HR_N_MAX				600					// The max size of the Heart Rate array for HRV analysis

#define MACC 4

#define BEAT_SAMPLE_RATE	100
#define BEAT_MS_PER_SAMPLE	( (double) 1000/ (double) BEAT_SAMPLE_RATE)

#define BEAT_MS10		((int) (10/BEAT_MS_PER_SAMPLE + 0.5))
#define BEAT_MS20		((int) (20/BEAT_MS_PER_SAMPLE + 0.5))
#define BEAT_MS40		((int) (40/BEAT_MS_PER_SAMPLE + 0.5))
#define BEAT_MS50		((int) (50/BEAT_MS_PER_SAMPLE + 0.5))
#define BEAT_MS60		((int) (60/BEAT_MS_PER_SAMPLE + 0.5))
#define BEAT_MS70		((int) (70/BEAT_MS_PER_SAMPLE + 0.5))
#define BEAT_MS80		((int) (80/BEAT_MS_PER_SAMPLE + 0.5))
#define BEAT_MS90		((int) (90/BEAT_MS_PER_SAMPLE + 0.5))
#define BEAT_MS100	((int) (100/BEAT_MS_PER_SAMPLE + 0.5))
#define BEAT_MS110	((int) (110/BEAT_MS_PER_SAMPLE + 0.5))
#define BEAT_MS120	((int) (120/BEAT_MS_PER_SAMPLE + 0.5))		// Added by HMC
#define BEAT_MS130	((int) (130/BEAT_MS_PER_SAMPLE + 0.5))
#define BEAT_MS140	((int) (140/BEAT_MS_PER_SAMPLE + 0.5))
#define BEAT_MS150	((int) (150/BEAT_MS_PER_SAMPLE + 0.5))
#define BEAT_MS250	((int) (250/BEAT_MS_PER_SAMPLE + 0.5))
#define BEAT_MS280	((int) (280/BEAT_MS_PER_SAMPLE + 0.5))
#define BEAT_MS300	((int) (300/BEAT_MS_PER_SAMPLE + 0.5))
#define BEAT_MS350	((int) (350/BEAT_MS_PER_SAMPLE + 0.5))
#define BEAT_MS400	((int) (400/BEAT_MS_PER_SAMPLE + 0.5))
#define BEAT_MS1000	BEAT_SAMPLE_RATE

#define BEATLGTH	BEAT_MS1000
#define MAXTYPES 8
#define FIDMARK BEAT_MS400

// For QRS detection
#define RECORDS_PER_SAMP	1	// 實際的取樣頻率是QRS detection計算取樣頻率的多少倍
#define SAMPLE_RATE	1000	/* Sample rate in Hz. */
#define MS_PER_SAMPLE	1   // Shoud be ( (double) 1000/ (double) SAMPLE_RATE)
#define MS10	(10/ MS_PER_SAMPLE)
#define MS25	(25/MS_PER_SAMPLE)
#define MS30	(30/MS_PER_SAMPLE)
#define MS80	(80/MS_PER_SAMPLE)
#define MS95	(95/MS_PER_SAMPLE)
#define MS100	(100/MS_PER_SAMPLE)
#define MS125	(125/MS_PER_SAMPLE)
#define MS150	(150/MS_PER_SAMPLE)
#define MS160	(160/MS_PER_SAMPLE)
#define MS175	(175/MS_PER_SAMPLE)
#define MS195	(195/MS_PER_SAMPLE)
#define MS200	(200/MS_PER_SAMPLE)
#define MS220	(220/MS_PER_SAMPLE)
#define MS250	(250/MS_PER_SAMPLE)
#define MS300	(300/MS_PER_SAMPLE)
#define MS360	(360/MS_PER_SAMPLE)
#define MS450	(450/MS_PER_SAMPLE)
#define MS1000	SAMPLE_RATE
#define MS1500	(1500/MS_PER_SAMPLE)
#define DERIV_LENGTH	MS10
#define LPBUFFER_LGTH (2*MS25)
#define HPBUFFER_LGTH MS125
#define WINDOW_WIDTH	MS80			// Moving window integration width.
#define	FILTER_DELAY (int) (((double) DERIV_LENGTH/2) + ((double) LPBUFFER_LGTH/2 - 1) + (((double) HPBUFFER_LGTH-1)/2) + PRE_BLANK)  // filter delays plus 200 ms blanking delay
#define DER_DELAY	WINDOW_WIDTH + FILTER_DELAY + MS100
#define PRE_BLANK	MS195
#define MIN_PEAK_AMP	7 // Prevents detections of peaks smaller than 150 uV.

#define FIX_R_MISS_PRD		1660		// The time period for the algorithm to fix R-point missing (unit: MS_PER_SAMPLE ms)
#define FIX_R_MISS_SBCMX	202			// The max of m_RD2_sbcount for the algorithm to fix R-point missing (unit: MS_PER_SAMPLE ms)
#define FIX_R_MISS_RTRSHR	(0.65)		// The ratio of the new threshold (in FIX_R_MISS_PRD) to the old one (m_RD2_det_thresh) for the algorithm to
// fix R-point missing

#define HR_OUT_ST_SEC		(16.2)		// The startint time (sec.) of Heart Rate output

#define ECG_BUFFER_LENGTH	5000	// Should be long enough for a beat
// plus extra space to accommodate
// the maximum detection delay.
#define BEAT_QUE_LENGTH	10			// Length of que for beats awaiting
// classification.  Because of
// detection delays, Multiple beats
// can occur before there is enough data
// to classify the first beat in the que.

// ECG annotation codes

#define	NOTQRS	0	/* not-QRS (not a getann/putann code) */
#define NORMAL	1	/* normal beat */
#define	LBBB	2	/* left bundle branch block beat */
#define	RBBB	3	/* right bundle branch block beat */
#define	ABERR	4	/* aberrated atrial premature beat */
#define	PVC	5	/* premature ventricular contraction */
#define	FUSION	6	/* fusion of ventricular and normal beat */
#define	NPC	7	/* nodal (junctional) premature beat */
#define	APC	8	/* atrial premature contraction */
#define	SVPB	9	/* premature or ectopic supraventricular beat */
#define	VESC	10	/* ventricular escape beat */
#define	NESC	11	/* nodal (junctional) escape beat */
#define	PACE	12	/* paced beat */
#define	UNKNOWN	13	/* unclassifiable beat */
#define	NOISE	14	/* signal quality change */
#define ARFCT	16	/* isolated QRS-like artifact */
#define STCH	18	/* ST change */
#define TCH	19	/* T-wave change */
#define SYSTOLE	20	/* systole */
#define DIASTOLE 21	/* diastole */
#define	NOTE	22	/* comment annotation */
#define MEASURE 23	/* measurement annotation */
#define	BBB	25	/* left or right bundle branch block */
#define	PACESP	26	/* non-conducted pacer spike */
#define RHYTHM	28	/* rhythm change */
#define	LEARN	30	/* learning */
#define	FLWAV	31	/* ventricular flutter wave */
#define	VFON	32	/* start of ventricular flutter/fibrillation */
#define	VFOFF	33	/* end of ventricular flutter/fibrillation */
#define	AESC	34	/* atrial escape beat */
#define SVESC	35	/* supraventricular escape beat */
#define	NAPC	37	/* non-conducted P-wave (blocked APB) */
#define	PFUS	38	/* fusion of paced and normal beat */
#define PQ	39	/* PQ junction (beginning of QRS) */
#define	JPT	40	/* J point (end of QRS) */
#define RONT	41	/* R-on-T premature ventricular contraction */

/* ... annotation codes between RONT+1 and ACMAX inclusive are user-defined */

#define	ACMAX	49	/* value of largest valid annot code (must be < 50) */

// Match definitions

#define MATCH_LENGTH	BEAT_MS300	// Number of points used for beat matching.
#define MATCH_LIMIT	1.2			// Match limit used testing whether two
// beat types might be combined.
#define COMBINE_LIMIT	0.8		// Limit used for deciding whether two types
// can be combined.
#define MATCH_START	(FIDMARK-(MATCH_LENGTH/2))	// Starting point for beat matching
#define MATCH_END	(FIDMARK+(MATCH_LENGTH/2))		// End point for beat matching.
#define MAXPREV	8	// Number of preceeding beats used as beat features.
#define MAX_SHIFT	BEAT_MS40

// RhythmChk definitions

// Define RR interval types.
#define QQ	0	// Unknown-Unknown interval.
#define NN	1	// Normal-Normal interval.
#define NV	2	// Normal-PVC interval.
#define VN	3	// PVC-Normal interval.
#define VV	4	// PVC-PVC interval.
#define RBB_LENGTH	8
#define LEARNING	0
#define READY	1
#define BRADY_LIMIT	MS1500

// AnalBeat definitions

#define ISO_LENGTH1  BEAT_MS50
#define ISO_LENGTH2	BEAT_MS80
#define ISO_LIMIT	12

#define AF_ISO_LIMIT	8
#define AF_ISO_LENGTH	BEAT_MS110				// The length of isoelectric region finding for Atrl. Fib. (AF) detection

// Classify definitions

// Detection Rule Parameters.
#define MATCH_LIMIT_CLAS 1.3					// Threshold for template matching
// without amplitude sensitivity.
#define MATCH_WITH_AMP_LIMIT	2.5	// Threshold for matching index that
// is amplitude sensitive.
#define PVC_MATCH_WITH_AMP_LIMIT 0.9 // Amplitude sensitive limit for
//matching premature beats
#define BL_SHIFT_LIMIT	100			// Threshold for assuming a baseline shift.
#define NEW_TYPE_NOISE_THRESHOLD	18	// Above this noise level, do not create
// new beat types.
#define NEW_TYPE_HF_NOISE_LIMIT 75	// Above this noise level, do not crate
// new beat types.

#define MATCH_NOISE_THRESHOLD	0.7	// Match threshold below which noise
// indications are ignored.
// TempClass classification rule parameters.
#define R2_DI_THRESHOLD 1.0		// Rule 2 dominant similarity index threshold
#define R3_WIDTH_THRESHOLD	BEAT_MS90		// Rule 3 width threshold.
#define R7_DI_THRESHOLD	1.2		// Rule 7 dominant similarity index threshold
#define R8_DI_THRESHOLD 1.5		// Rule 8 dominant similarity index threshold
#define R9_DI_THRESHOLD	2.0		// Rule 9 dominant similarity index threshold
#define R10_BC_LIM	3				// Rule 10 beat count limit.
#define R10_DI_THRESHOLD	2.5	// Rule 10 dominant similarity index threshold
#define R11_MIN_WIDTH BEAT_MS110			// Rule 11 minimum width threshold.
#define R11_WIDTH_BREAK	BEAT_MS140			// Rule 11 width break.
#define R11_WIDTH_DIFF1	BEAT_MS40			// Rule 11 width difference threshold 1
#define R11_WIDTH_DIFF2	BEAT_MS60			// Rule 11 width difference threshold 2
#define R11_HF_THRESHOLD	45		// Rule 11 high frequency noise threshold.
#define R11_MA_THRESHOLD	14		// Rule 11 motion artifact threshold.
#define R11_BC_LIM	1				// Rule 11 beat count limit.
#define R15_DI_THRESHOLD	3.5	// Rule 15 dominant similarity index threshold
#define R15_WIDTH_THRESHOLD BEAT_MS100	// Rule 15 width threshold.
#define R16_WIDTH_THRESHOLD BEAT_MS100	// Rule 16 width threshold.
#define R17_WIDTH_DELTA	BEAT_MS20			// Rule 17 difference threshold.
#define R18_DI_THRESHOLD	1.5	// Rule 18 dominant similarity index threshold.
#define R19_HF_THRESHOLD	75		// Rule 19 high frequency noise threshold.
// Dominant monitor constants.
#define DM_BUFFER_LENGTH	180
#define IRREG_RR_LIMIT	60

// NoiseChk definitions

#define NB_LENGTH	MS1500
#define NS_LENGTH	MS50

typedef std::complex<double> MyComplex;

char ECGFilePath[256];
char ECGDirUrl[256];
float ECGAnaData2[ECG_ANALY_SIZE];      // 存放供分析的心電圖波形區段資料(Lead II)
float ECGFilt[ECG_FRAME_SIZE];         // 存放濾波後的心電圖波形區段資料
MyComplex PowerFFTRlt[POWERFFT_ANALY_SIZE];  // This is the in-place FFT buffer
float powerNoiseHz; // The calculated power line noise frequency (in Hz)
bool bPNoiseGot = false;	// Power line noise frequency已經算出
float notchNcoeff[3];		// notch濾波器的係數
float notchDcoeff[2];		// notch濾波器的係數
float ECGdelayBuffer[MaxBufferSize*RawRatio];  // 心電圖資料的delay buffer (供無限脈衝響應filter使用)
float ECGdelayBuffer1[MaxBufferSize*RawRatio];  // 心電圖資料的delay buffer (供無限脈衝響應filter使用)
int notchDelayHead;  // 指向ECGdelayBuffer頭端的index
int notchDelayTail;  // 指向ECGdelayBuffer尾端的index

unsigned char ECGFrameOri[FileFrameSize];  // 儲存由心電圖資料檔中讀出的10秒心電圖區間資料 - 原始資料

unsigned short ECGFrame0[ECG_FRAME_SIZE];  // 儲存由心電圖資料檔中讀出的10秒心電圖區間資料(Lead I)
unsigned short ECGFrame1[ECG_FRAME_SIZE];  // 儲存由心電圖資料檔中讀出的10秒心電圖區間資料(Lead II)
unsigned short ECGFrame2[ECG_FRAME_SIZE];  // 儲存由心電圖資料檔中讀出的10秒心電圖區間資料(Lead III)

int BWRFrameSize;		// BWRFrame的實際大小(單位: 1ms有一資料點)
int BWRpageEndIdx;		// 最後一頁的結束位置(For BWRFrame)
unsigned short BWRFrame0[ECG_FRAME_SIZE+BWR_FILTER_SIZE*2+BWR_S2_SIZE];  // 心電圖區間資料For Baseline Wandering Removal (BWR) (Lead I)
unsigned short BWRFrame1[ECG_FRAME_SIZE+BWR_FILTER_SIZE*2+BWR_S2_SIZE];  // 心電圖區間資料For Baseline Wandering Removal (BWR) (Lead II)
unsigned short BWRFrame2[ECG_FRAME_SIZE+BWR_FILTER_SIZE*2+BWR_S2_SIZE];  // 心電圖區間資料For Baseline Wandering Removal (BWR) (Lead III)

unsigned short NchPreFrame0[ECG_FRAME_SIZE];  // 前一個Frame的心電圖區間資料(For Notch filter) (Lead I)
unsigned short NchPreFrame1[ECG_FRAME_SIZE];  // 前一個Frame的心電圖區間資料(For Notch filter) (Lead II)
unsigned short NchFrame0[ECG_FRAME_SIZE + NOTCH_PREPAD_SIZE];  // 心電圖區間資料For Notch filter (Lead I)
unsigned short NchFrame1[ECG_FRAME_SIZE + NOTCH_PREPAD_SIZE];  // 心電圖區間資料For Notch filter (Lead II)

unsigned short DSpreFrame0[ECG_FRAME_SIZE];  // Temporary storing the previous filtered ECG Frame for the Dual-Slope QRS Detection (Lead I)
unsigned short DSpreFrame1[ECG_FRAME_SIZE];  // Temporary storing the previous filtered ECG Frame for the Dual-Slope QRS Detection (Lead II)

unsigned short DSdownFrame[DS_FRAME_SIZE + DS_SLOPE_SIZE * 2];  // The downsampled ECG buffer including two previous slope widths for the Dual-Slope QRS Detection (Lead I or Lead II)

float DS_Sdiff[DS_FRAME_SIZE];  // The Sdiff_max buffer for the Dual-Slope QRS Detection
float DS_SdiffPre[DS_FRAME_SIZE];  // The previous frame for the Sdiff_max buffer of the Dual-Slope QRS Detection

float DS_S_LRmin[DS_FRAME_SIZE];  // The buffer of the smaller one of the left and right slopes of R point for the Dual-Slope QRS Detection
float DS_S_LRminPre[DS_FRAME_SIZE];  // The previous frame of the smaller one of the left and right slopes of R point for the Dual-Slope QRS Detection

float DS_High[DS_FRAME_SIZE];  // The ECG buffer for the Dual-Slope QRS Detection
float DS_HighPre[DS_FRAME_SIZE];  // The previous frame for the ECG buffer of the Dual-Slope QRS Detection

float SdiffDiffRstLast[DS_FRAME_SIZE];	// The last result of the calculation of the difference of Sdiff_max including the span of the differential filter
float SdiffAveBuf[DS_SDIFF_AVE_N];				// Storing DS_SDIFF_AVE_N Sdiff_max(s) for average
int SdiffAveIdx = 0;							// The index of the current Sdiff_max
float HighAveBuf[DS_SDIFF_AVE_N];				// Storing DS_SDIFF_AVE_N ECG peak(s) for average
int HighAveIdx = 0;							// The index of the current ECG peak
float fCurThetaDiff = 0;					// The value of the current Theta_diff
int lastRdistCnt = 0;						// The counter for the distance of the current R point candidate and the last R point

bool bDS_Sdiff1stFrame = true;		// true: This is the first DS_Sdiff[] frame
bool bDS_SdiffDiff1stF = true;		// true: This is the first SdiffDiffRst[] frame
bool bDS_1stRfnd = true;			// true: This is the first R found in the Dual-Slope QRS Detection
bool bDS_GloRfndForCla = false;		// The R found flag for BeatClassify

std::list<int> DS_RPosList;	// The list of R position (Unit: The DS_DOWNSAMPLE_R downsampled index)
std::list<int>::iterator iterDS_RPosList;

std::list<int> DS_GloRList0;	// The list of R position in the global time
std::list<int>::iterator iterDS_GloRList0;

std::list<int> DS_GloRList1;	// The list of R position in the global time
std::list<int>::iterator iterDS_GloRList1;

float FIRCoeff[FIR_COEFF_N];  // 儲存FIR線性相位濾波器的tap係數
int FIRCoeffInd;

int RD2_8to18Cnt;			// The counter to count 8 sec. to 18 sec. in QRSDet()
unsigned short RD2b8st;		// Whether the RD2_8to18Cnt counter starts

void decryFileName( char* lpszPlain, char* lpszEncry);  // 將檔名解密
unsigned char fNChar2Enc( unsigned char cFName );       // 將檔名解密會用到，把用於檔名的ASCII code轉成加密後的數值
void PowerFFT();                                        // 計算心電圖片段(長度為ECGFFT_ANALY_SIZE)的Fast Fourier Transform
void CalPNoiseFreq();                                   // 計算Power line noise頻率
void CalNotchCoeff();                                   // 由powerNoiseHz計算notch filter的參數
void notchFrame();       // 對由檔案中讀進來的心電圖資料進行notch濾波
void nonNotchFrame();    // 不進行notch濾波的對照組
void incFilteredIndex(int* pFilteredIndex);             // 將參數加一(為了由PC軟體移植過來方便)
void decFilteredIndex(int* pFilteredIndex);             // 將參數減一(為了由PC軟體移植過來方便)
void BWRFrame();										// Baseline Wandering Removal function (對一個Frame)
void FIRFrame(float * ECGRaw, float * ECGFilt);	// FIR Lowpass Filter (對一個Frame)
void DSFrameFindR();	// The Dual-Slope QRS Detection in a DSdownFrame[] frame

class CQRSDet
{
public:
    long m_RD2_y1;
    long m_RD2_y2;
    int m_RD2_lpdata[LPBUFFER_LGTH];
    int m_RD2_lpptr;
    long m_RD2_y;
    int m_RD2_hpdata[HPBUFFER_LGTH];
    int m_RD2_hpptr;
    int m_RD2_derBuff1[DERIV_LENGTH];
    int m_RD2_derI1;
    int m_RD2_derBuff2[DERIV_LENGTH];
    int m_RD2_derI2;
    long m_RD2_mwisum;
    int m_RD2_mwidata[WINDOW_WIDTH];
    int m_RD2_mwiptr;

    double m_RD2_TH;
    int * m_RD2_DDBuffer;	/* Buffer holding derivative data. */
    int m_RD2_DDPtr;	/* The ptr to the buffer holding derivative data. */
    int m_RD2_Dly;
    int m_RD2_MEMMOVELEN;

    int m_RD2_det_thresh;
    int m_RD2_qpkcnt;
    int m_RD2_qrsbuf[8];
    int m_RD2_noise[8];
    int m_RD2_rrbuf[8];
    int m_RD2_rsetBuff[8];
    int m_RD2_rsetCount;
    int m_RD2_nmean;
    int m_RD2_qmean;
    int m_RD2_rrmean;
    int m_RD2_count;
    int m_RD2_sbpeak;
    int m_RD2_sbloc;
    int m_RD2_sbcount;
    int m_RD2_realSbcount;
    int m_RD2_maxder;
    int m_RD2_lastmax;
    int m_RD2_initBlank;
    int m_RD2_initMax;
    int m_RD2_preBlankCnt;
    int m_RD2_tempPeak;
    int m_RD2_pk_max;
    int m_RD2_pk_timeSinceMax;
    int m_RD2_pk_lastDatum;
    short m_RD2_First1s;	// The first one second

public:
    CQRSDet(void);
    ~CQRSDet(void);

    int QRSInit();

    int QRSFilter(int datum,int init);
    int RD2lpfilt( int datum ,int init);
    int RD2hpfilt( int datum, int init );
    int RD2deriv1(int x, int init);
    int RD2deriv2(int x, int init);
    int RD2mvwint(int datum, int init);
    int QRSDet( int datum, int init );
    int RD2Peak( int datum, int init );
    int RD2mean(int *array, int datnum);
    int RD2thresh(int qmean, int nmean);
    int RD2BLSCheck(int *dBuf,int dbPtr,int *maxder);

};

class CBeatClassify
{

// 屬性
public:
    int ECGBuffer[ECG_BUFFER_LENGTH];
    int ECGBufferIndex;  // Circular data buffer.
    int BeatBuffer[BEATLGTH] ;
    int BeatQue[BEAT_QUE_LENGTH];  // Buffer of detection delays.
    int BeatQueCount;
    int RRCount;
    int InitBeatFlag;

    // Match variables

    int BeatTemplates[MAXTYPES][BEATLGTH] ;
    int BeatCounts[MAXTYPES] ;
    int BeatWidths[MAXTYPES] ;
    int BeatClassifications[MAXTYPES] ;
    int BeatBegins[MAXTYPES] ;
    int BeatEnds[MAXTYPES] ;
    int BeatsSinceLastMatch[MAXTYPES] ;
    int BeatAmps[MAXTYPES] ;
    int BeatCenters[MAXTYPES] ;
    double MIs[MAXTYPES][8] ;
    int TypeCount;

    // RhythmChk variables

    // Global variables.
    int RRBuffer[RBB_LENGTH];
    int RRTypes[RBB_LENGTH];
    int BeatCount;
    int ClassifyState ;
    int BigeminyFlag ;

    // PostClas variables

    // Records of post classifications.
    int PostClass[MAXTYPES][8];
    int PCInitCount ;
    int PCRhythm[MAXTYPES][8] ;

    // Classify variables

    // Local Global variables
    int DomType ;
    int RecentRRs[8];
    int RecentTypes[8] ;
    int NewDom;
    int	DomRhythm ;
    int DMBeatTypes[DM_BUFFER_LENGTH];
    int DMBeatClasses[DM_BUFFER_LENGTH] ;
    int DMBeatRhythms[DM_BUFFER_LENGTH] ;
    int DMNormCounts[8];
    int DMBeatCounts[8];
    int DMIrregCount;

    // NoiseChk variables

    int NoiseBuffer[NB_LENGTH];
    int NBPtr ;
    int NoiseEstimate ;

    // Added variables
    bool ST_elevation;	// Whether ST Elevation occurred
    bool bAF_fnd;		// Whether Atrl. Fib. (AF) found
    CQRSDet m_QRSDetForAna;



public:
    CBeatClassify(void);
    ~CBeatClassify(void);

    // Internal function prototypes.

    void DownSampleBeat(int *beatOut, int *beatIn) ;

    // External function prototypes.

    //int QRSDet( int datum, int init ) ;
    int NoiseCheck(int datum, int delay, int RR, int beatBegin, int beatEnd) ;
    int Classify(int *newBeat,int rr, int noiseLevel, int *beatMatch, int *fidAdj, int init) ;
    int GetDominantType(void) ;
    int GetBeatEnd(int type) ;
    int GetBeatBegin(int type) ;
    int gcd(int x, int y) ;

    /******************************************************************************
		ResetBDAC() resets static variables required for beat detection and
		classification.
	*******************************************************************************/

    void ResetBDAC(void);

    /*****************************************************************************
	Syntax:
		int BeatDetectAndClassify(int ecgSample, int *beatType, *beatMatch) ;

	Description:
		BeatDetectAndClassify() implements a beat detector and classifier.
		ECG samples are passed into BeatDetectAndClassify() one sample at a
		time.  BeatDetectAndClassify has been designed for a sample rate of
		200 Hz.  When a beat has been detected and classified the detection
		delay is returned and the beat classification is returned through the
		pointer *beatType.  For use in debugging, the number of the template
	   that the beat was matched to is returned in via *beatMatch.

	Returns
		BeatDetectAndClassify() returns 0 if no new beat has been detected and
		classified.  If a beat has been classified, BeatDetectAndClassify returns
		the number of samples since the approximate location of the R-wave.

	****************************************************************************/

    int BeatDetectAndClassify(int ecgSample, int *beatType, int *beatMatch);

    // Match functions

    int NewBeatType(int *beat) ;
    void BestMorphMatch(int *newBeat,int *matchType,double *matchIndex, double *mi2, int *shiftAdj) ;
    void UpdateBeatType(int matchType,int *newBeat, double mi2, int shiftAdj) ;
    int GetTypesCount(void) ;
    int GetBeatTypeCount(int type) ;
    int IsTypeIsolated(int type) ;
    void SetBeatClass(int type, int beatClass) ;
    int GetBeatClass(int type) ;
    //int GetDominantType(void) ;
    int GetBeatWidth(int type) ;
    int GetPolarity(int type) ;
    int GetRhythmIndex(int type) ;
    void ResetMatch(void) ;
    void ClearLastNewType(void) ;
    //int GetBeatBegin(int type) ;
    //int GetBeatEnd(int type) ;
    int GetBeatAmp(int type) ;
    int MinimumBeatVariation(int type) ;
    int GetBeatCenter(int type) ;
    int WideBeatVariation(int type) ;
    double DomCompare2(int *newBeat, int domType) ;
    double DomCompare(int newType, int domType) ;
    // Local prototypes.
    int NoiseCheck(int *beat) ;
    double CompareBeats(int *beat1, int *beat2, int *shiftAdj) ;
    double CompareBeats2(int *beat1, int *beat2, int *shiftAdj) ;
    void UpdateBeat(int *aveBeat, int *newBeat, int shift) ;
    void BeatCopy(int srcBeat, int destBeat) ;
    // External prototypes.
    void AnalyzeBeat(int *beat, int *onset, int *offset, int *isoLevel,
                     int *beatBegin, int *beatEnd, int *amp) ;
    void AdjustDomData(int oldType, int newType) ;
    void CombineDomData(int oldType, int newType) ;

    // RhythmChk functions

    void ResetRhythmChk(void) ;
    int RhythmChk(int rr) ;
    int IsBigeminy(void) ;
    // Local prototypes.
    int RRMatch(int rr0,int rr1) ;
    int RRShort(int rr0,int rr1) ;
    int RRShort2(int *rrIntervals, int *rrTypes) ;
    int RRMatch2(int rr0,int rr1) ;

    // AnalBeat functions

    // Local prototypes.
    int IsoCheck(int *data, int isoLength);

    int AFisoCheck(int *data, int isoLength);		// Added by HMC for AF detection

    // PostClas functions

    void ResetPostClassify() ;
    void PostClassify(int *recentTypes, int domType, int *recentRRs, int width, double mi2,
                      int rhythmClass) ;
    int CheckPostClass(int type) ;
    int CheckPCRhythm(int type) ;

    // Classify functions

    // Local prototypes.
    int HFNoiseCheck(int *beat) ;
    int TempClass(int rhythmClass, int morphType, int beatWidth, int domWidth,
                  int domType, int hfNoise, int noiseLevel, int blShift, double domIndex) ;
    int DomMonitor(int morphType, int rhythmClass, int beatWidth, int rr, int reset) ;
    int GetNewDominantType(void);
    int GetDomRhythm(void) ;
    int GetRunCount(void) ;

    // NoiseChk functions

    /************************************************************************
		GetNoiseEstimate() allows external access the present noise estimate.
		this function is only used for debugging.
	*************************************************************************/
    int GetNoiseEstimate();
};

CQRSDet QRSDetForRFnd;		// For R-point finding
int detDlyForRFnd;			// Detection delay for QRS finding

// Variables for ECG feature calculation for User Identification
double phase_space[RR_N_MAX_4UID - EMBEDDING_DIMENSION * DELAY][EMBEDDING_DIMENSION];

extern "C" void fasper(float * x, float * y, unsigned long n, float ofac, float hifac, float * wk1, float * wk2,
                       unsigned long nwk, unsigned long * nout, unsigned long * jmax, float * prob);

double Cal_HRV_PI(double rrIntervals[], int numIntervals);
double Cal_HRV_GI(double rrIntervals[], int numIntervals);
double Cal_HRV_SI(double rrIntervals[], int numPoints);
double compute_std_dev(double *data, int n);
double Cal_HRV_CVI(double rr_intervals[], int n);
double euclidean_distance(double* a, double* b, int len);
double calculate_correlation_sum(double radius, int RR_Data_Len);
double linear_regression(double* x, double* y, int len);
double Cal_HRV_CD(double rr_intervals[], int RR_Data_Len);
double Cal_HRV_C1a(double *rr_intervals, int size);
double Cal_HRV_ShanEn(double rr_intervals[], int size, double * pMedianRR);

extern "C" JNIEXPORT jint JNICALL
Java_com_example_newidentify_MainActivity_stringFromJNI(JNIEnv* env, jobject /* this */, jstring str, jstring path){
    char chaFileName[256];
    strcpy(chaFileName, env ->GetStringUTFChars(str, JNI_FALSE));

//    strcpy(ECGFilePath, "/home/user/RemoteECG/ECG_Files/LabeledEcgFiles/20170421/");

    strcpy(ECGFilePath, "/storage/emulated/0/ECGFiles/test/");

//    strcpy(ECGFilePath, env ->GetStringUTFChars(path, JNI_FALSE));
    strcpy(ECGDirUrl, "../ECGFiles/");

    unsigned short * pECGFrameOri = (unsigned short *)ECGFrameOri;

    // notch filter的係數
    notchNcoeff[0] = 0.981150444;
    notchNcoeff[1] = -1.824501224;
    notchNcoeff[2] = 0.981150444;

    notchDcoeff[0] = 1.824501224;
    notchDcoeff[1] = -0.9623008882;

    notchDelayHead = MaxBufferSize*RawRatio - 1;
    notchDelayTail = MaxBufferSize*RawRatio - 1;

    // Initialize notch filter (to depress the starting noise)
    notchDelayHead--;
    if(notchDelayHead < 0)
        notchDelayHead = MaxBufferSize*RawRatio - 1;
    if(notchDelayHead == notchDelayTail)
    {	notchDelayTail--;
        if(notchDelayTail < 0)
            notchDelayTail = MaxBufferSize*RawRatio - 1;
    }
    ECGdelayBuffer[notchDelayHead] = 27787.410;
    ECGdelayBuffer1[notchDelayHead] = 27787.410;
    notchDelayHead--;
    if(notchDelayHead < 0)
        notchDelayHead = MaxBufferSize*RawRatio - 1;
    if(notchDelayHead == notchDelayTail)
    {	notchDelayTail--;
        if(notchDelayTail < 0)
            notchDelayTail = MaxBufferSize*RawRatio - 1;
    }
    ECGdelayBuffer[notchDelayHead] = 28199.822;
    ECGdelayBuffer1[notchDelayHead] = 28199.822;
    notchDelayHead--;
    if(notchDelayHead < 0)
        notchDelayHead = MaxBufferSize*RawRatio - 1;
    if(notchDelayHead == notchDelayTail)
    {	notchDelayTail--;
        if(notchDelayTail < 0)
            notchDelayTail = MaxBufferSize*RawRatio - 1;
    }
    ECGdelayBuffer[notchDelayHead] = 28483.166;
    ECGdelayBuffer1[notchDelayHead] = 28483.166;

    //strcpy(ECGFilePath, "");

    char encFtstr[22];        // The string representing the recording time (one portion of the ECG data file name)
    char fileTimeStr[22];        // The string representing the recording time (one portion of the ECG data file name)
    char inCHAFileName[CHA_FNAME_LEN_MAX];        // The file name of the input .cha file
    int inPageN;      // The index of the requested page in the ECG data file
    int filePageN;    // 實際在讀取ECG data file時所用的Page index (=inPageN-1)
    int intFrameSize = FileFrameSize;
    int leadN;                  // What lead is requested by the user
    long long int anaSt;                  // The start index of ECG data for analysis (selected from mouse-click by user)
    char devID[7];        // 儲存Device ID備用
    int bNoiseFilt;       // 是否要過濾電源線雜訊
    int bAmpl;            // 是否處於"放大"模式
    std::list<double> HRList;	// Store all Heart Rates
    std::list<double>::iterator iterHRList;
    std::list<double> HRNNList;	// 儲存Normal beats的Heart rates
    std::list<double>::iterator iterHRNNList;
    std::list<double> RRSDList;	// 儲存successive differences of RR intervals
    std::list<double>::iterator iterRRSDList;

    // Variables for ECG feature calculation for User Identification
    std::list<double> RRList4UID;	// RR interval list for User Identification
    std::list<double>::iterator iterRRList4UID;
    double RRarray4UID[RR_N_MAX_4UID];
    double lastMarkLoc = 0;			// 此檔案中上一個R point的時間(Normal beat, 單位: 秒)

    bool bAbnorFound;	// 分析過程中是否發現心臟異常
    bool bFirNorBfnd;	// 是否已發現第一個Normal beat
    bool bFirNNfnd;		// True: The first NN has been calculated.
    bool bFirRfnd;		// True: The first R point has been found.
    bool bFirRRfnd;		// True: The first RR interval has been calculated.

    int printResult;

    bAbnorFound = false;
    bFirNorBfnd = false;
    bFirNNfnd = false;
    bFirRfnd = false;
    bFirRRfnd = false;

    //if( argc < 2 )
    //	return EXIT_FAILURE;

    //strcpy ( inCHAFileName, argv[1] );		// Get the input file name from the command line argument

    strcpy ( inCHAFileName, chaFileName );		// Get the input file name from the command line argument

    // For debug
    //strcpy ( inCHAFileName, "20230319185544_032852" );
    //strcpy ( inCHAFileName, "20230319183235_029924" );

    // For debug
    inPageN = 1;
    leadN = 1;
    anaSt = 0;
    strcpy(devID, "857a2a");
    bNoiseFilt = 0;
    bAmpl = 0;

    filePageN = inPageN-1;

    // set the file name to read
    char lpszFileName[1024];
    strcpy(lpszFileName, "");
    strcat(lpszFileName, ECGFilePath);
    strcat(lpszFileName, inCHAFileName);
    __android_log_print(ANDROID_LOG_ERROR, "native-log", "%s",lpszFileName);
//    strcat(lpszFileName, ".cha");

    // For debug
    //strcpy(lpszFileName, "20170414132226_857a2a.CHA");

    // 讀取檔案大小
    intmax_t fileSize;          // 儲存 file size
    intmax_t maxPageN;      // 此心電圖檔的最大頁數
    struct stat     statbuf;
    // Get the file's information.
    if (stat(lpszFileName, &statbuf) == -1)
    {	printf("<br/>The information of %s cannot be retrieved.<br/>", lpszFileName);
        __android_log_print(ANDROID_LOG_ERROR, "native-log", "1");
        return EXIT_FAILURE;
    }
    // Get the size of the ECG file.
    fileSize = (intmax_t)(statbuf.st_size);
    // Calculate the max page number
    maxPageN =  fileSize / FileFrameSize;
    if(fileSize % FileFrameSize)
        maxPageN++;

    int intMaxPageN = (int)maxPageN;

    // 讀取檔案中的心電圖資料

    int errCode;
    int fi, fi2;
    unsigned char tempDateTime[8];
    unsigned char DateTimeComp1[8];		// 儲存心電圖記錄檔的記錄時間
    unsigned short ECGnodeID[3];		// 處理(組合)後的心電圖儀node ID (24bits,存在三個unsigned short的low 8 bits中)
    unsigned short ECG_OUI[3];			// 處理(組合)後的心電圖儀OUI ID (24bits,存在三個unsigned short的low 8 bits中)

    unsigned int nBytesReceived;
    bool bResult;
    int loadOffset = 0;

    unsigned short nodeIDori[6];					// 接收心電圖儀傳來的node ID (24bits,存在四個unsigned short的high 6 bits中)
    unsigned short OUIori[6];					// 接收心電圖儀傳來的OUI ID (24bits,存在四個unsigned short的high 6 bits中)

    unsigned short BWRpreFrame0[ECG_FRAME_SIZE];  // ECGFrame0前十秒的心電圖區間資料(For m_BWRFrame0) (Lead I)
    unsigned short BWRpreFrame1[ECG_FRAME_SIZE];  // ECGFrame1前十秒的心電圖區間資料(For m_BWRFrame1) (Lead II)
    unsigned short BWRpreFrame2[ECG_FRAME_SIZE];  // ECGFrame2前十秒的心電圖區間資料(For m_BWRFrame2) (Lead III)
    unsigned short BWRpostFrame0[ECG_FRAME_SIZE];  // ECGFrame0後十秒的心電圖區間資料(For m_BWRFrame0) (Lead I)
    unsigned short BWRpostFrame1[ECG_FRAME_SIZE];  // ECGFrame1後十秒的心電圖區間資料(For m_BWRFrame1) (Lead II)
    unsigned short BWRpostFrame2[ECG_FRAME_SIZE];  // ECGFrame2後十秒的心電圖區間資料(For m_BWRFrame2) (Lead III)

    unsigned short lastECGvalue0;	// 保存一個心電圖資料檔中的最後一個cell的第3個Lead I資料點(避免最後一個資料點失真), 用於填檔案最後一頁提前的結束點之後的資料點
    unsigned short lastECGvalue1;	// 保存一個心電圖資料檔中的最後一個cell的第3個Lead II資料點(避免最後一個資料點失真), 用於填檔案最後一頁提前的結束點之後的資料點

    int preFrameAddSize = BWR_FILTER_SIZE + BWR_S2_SIZE/2;	// 前一個Frame尾端多少Size加到此Frame前(單位: unsigned short), 為了前後Frame執行BWR後的連續性
    int postFrameAddSize = BWR_FILTER_SIZE + BWR_S2_SIZE/2;	// 後一個Frame前端多少Size加到此Frame後(單位: unsigned short), 為了前後Frame執行BWR後的連續性

    // 初始化NchPreFrame
    for(fi = 0; fi < ECG_FRAME_SIZE; fi++)
    {
        NchPreFrame0[fi] = ECGADC_MAX;
        NchPreFrame1[fi] = ECGADC_MAX;
    }

    // 將FIR濾波器tap係數初始化
    float nInd = -10.0;
    float nom = 10;	// 指定FIR低通濾波器的截止頻率(100 Hz)
    float myX;
    float winF = 0;
    float winFSum = 0;
    for(FIRCoeffInd = 0; FIRCoeffInd < FIR_COEFF_N; FIRCoeffInd++)
    {
        myX = 2 * PI * nInd / nom;
        winF = 0.54 - 0.46 * cos(2 * PI * FIRCoeffInd / FIR_COEFF_N);  // Hamming window

        if(myX == 0.0)
            FIRCoeff[FIRCoeffInd] = 2.0*1.11*1.0*winF/nom;
        else
            FIRCoeff[FIRCoeffInd] = 2.0*1.11*sin(myX)*winF/myX/nom;

        nInd += 1.0;
        winFSum += winF;
    }
    // Normalize
    for(FIRCoeffInd = 0; FIRCoeffInd < FIR_COEFF_N; FIRCoeffInd++)
        FIRCoeff[FIRCoeffInd] = 0.5 * FIRCoeff[FIRCoeffInd]/(winFSum/FIR_COEFF_N);

    FILE * pReadFile = NULL;	// Input ECG data file
    FILE * pOut1 = NULL;		// Output processed ECG data file
    FILE * pOut2 = NULL;		// Output processed ECG data file for results of Beat Classification
    FILE * pOut3 = NULL;		// Output processed down-sampling text file for long-period display
    FILE * pOut4 = NULL;		// Output the processed text file for results of QRS complex (R-point) finding

    strcpy(inCHAFileName, strtok(inCHAFileName, "."));

    // set the name of the output file
    char lpszOutFileName[1024];
    strcpy(lpszOutFileName, "");
    strcat(lpszOutFileName, ECGFilePath);

    strcat(lpszOutFileName, "o_");
    strcat(lpszOutFileName, inCHAFileName);
    strcat(lpszOutFileName, ".cha");

    // For debug
    //strcpy(lpszOutFileName, "o_20170414132226_857a2a.CHA");

    // set the name of the output file for results of analysis
    char lpszAnaRstFileName[1024];
    strcpy(lpszAnaRstFileName, "");
    strcat(lpszAnaRstFileName, ECGFilePath);

    strcat(lpszAnaRstFileName, "r_");
    strcat(lpszAnaRstFileName, inCHAFileName);
    strcat(lpszAnaRstFileName, ".txt");

    // For debug
    //strcpy(lpszAnaRstFileName, "r_20170414132226_857a2a.txt");

    // set the name of the output file for results of QRS complex (R-point) finding
    char lpszQRSfndFileName[1024];
    strcpy(lpszQRSfndFileName, "");
    strcat(lpszQRSfndFileName, ECGFilePath);

    strcat(lpszQRSfndFileName, "q_");
    strcat(lpszQRSfndFileName, inCHAFileName);
    strcat(lpszQRSfndFileName, ".txt");

    // For debug
    //strcpy(lpszQRSfndFileName, "q_20170414132226_857a2a.txt");

    // set the name of the output down-sampling text file for long-period display
    char lpszLongDispName[1024];
    strcpy(lpszLongDispName, "");
    strcat(lpszLongDispName, ECGFilePath);

    strcat(lpszLongDispName, "s_");
    strcat(lpszLongDispName, inCHAFileName);
    strcat(lpszLongDispName, ".txt");

    // For debug
    //strcpy(lpszLongDispName, "s_20170414132226_857a2a.CHA");

    bool bLastAFfound;		// Atrl. Fib. (AF) found in the last beat

    bLastAFfound = false;

    QRSDetForRFnd.QRSInit();
    QRSDetForRFnd.QRSDet(0,1) ;	// Reset the qrs detector

    float HRV_HR[HRV_HR_N_MAX];		// The array storing Heart Rate of all beats for NRV analysis
    float HRV_Time[HRV_HR_N_MAX];	// The array storing the time of the Heart Rate of all beats for
    // NRV analysis
    int HRV_HRIdx = 0;
    int lastBeatType = PVC;
    int HRV_50n = 0;		// The number of pairs of successive NN (R-R) intervals that differ by more than 50 ms
    int HRV_tot = 0;		// The total number of NN (R-R) intervals
    double lastRR = 0.0;
    double lastRR_RMSSD = 0.0;	// Stores the last RR interval for the RMSSD (Root mean square of the successive differences) calculation

    // open file
    pReadFile = fopen(lpszFileName, "r+b");
    if(pReadFile == NULL)
    {	printf("<br/>%s open failed<br/>", lpszFileName);
        __android_log_print(ANDROID_LOG_ERROR, "native-log", "2");
        return EXIT_FAILURE;
    }

    float RR4LeadSel = 0.0;		// The calculated RR interval for Lead Selection for QRS Detection
    float HR4LeadSel = 0.0;		// The calculated Heart Rate for Lead Selection for QRS Detection
    int HRGoodN = 0;			// The number of the Heart Rates which have been labeled "Good" (HR_LEADSEL_MIN < HR4LeadSel < HR_LEADSEL_MAX)
    double HRTot4LeadSel = 0.0;		// The total number of the Heart Rates for Lead Selection
    double HRGood4LeadSel = 0.0;	// The double type version of HRGoodN
    double HRGood2Tot = 0.0;	// The ratio of HRGood4LeadSel to HRTot4LeadSel
    double LastR4LeadSel = 0.0;	// The last R position for the calculation of RR4LeadSel
    bool b1stR4LeadSel = true;	// true: The current R is the first R for the calculation of RR4LeadSel
    bool bSelLeadII = true;		// true: Currently we select Lead II for QRS Detection
    int TryLeadSelN = 0;		// The number of times we try the Lead Selection for QRS Detection
    double HRG2TLeadII = 0.0;	// The ratio of HRGood4LeadSel to HRTot4LeadSel for Lead II

    do
    {
        HRGoodN = 0;
        b1stR4LeadSel = true;
        LastR4LeadSel = 0.0;
        HRTot4LeadSel = 0.0;

        HRList.clear();
        HRNNList.clear();
        RRSDList.clear();
        RRList4UID.clear();
        for(fi = 0; fi < RR_N_MAX_4UID; fi++)
            RRarray4UID[fi] = 0.0;

        lastMarkLoc = 0;
        bAbnorFound = false;
        bFirNorBfnd = false;
        bFirNNfnd = false;
        bFirRfnd = false;
        bFirRRfnd = false;
        HRV_HRIdx = 0;
        lastBeatType = PVC;
        HRV_50n = 0;
        HRV_tot = 0;
        lastRR = 0.0;
        lastRR_RMSSD = 0.0;

        // Reset the Beat Classifier and the QRS detection
        CBeatClassify aBeatClassifyForAna;
        aBeatClassifyForAna.m_QRSDetForAna.QRSInit();
        QRSDetForRFnd.QRSInit();

        bDS_Sdiff1stFrame = true;		// true: This is the first DS_Sdiff[] frame
        bDS_SdiffDiff1stF = true;		// true: This is the first SdiffDiffRst[] frame

        // Initialize SdiffAveBuf[]
        for (fi = 0; fi < DS_SDIFF_AVE_N; fi++)
            SdiffAveBuf[fi] = 0;

        SdiffAveIdx = 0;

        // Initialize HighAveBuf[]
        for (fi = 0; fi < DS_SDIFF_AVE_N; fi++)
            HighAveBuf[fi] = 0;

        HighAveIdx = 0;

        // Initialize fCurThetaDiff
        float fDSdownSampR = DS_DOWNSAMPLE_R;
        float fFreqSamp = 1000.0 / fDSdownSampR;
        fCurThetaDiff = DS_THETA_DIFF_0 / fFreqSamp;

        lastRdistCnt = 0;

        DS_GloRList0.clear();
        DS_GloRList1.clear();

        // Clear the output file
        if (pOut1 != NULL)
        {
            fclose(pOut1);
            pOut1 = NULL;
        }
        pOut1 = fopen(lpszOutFileName, "wb");
        if (pOut1 == NULL)
        {
            printf("<br/>%s open failed<br/>", lpszOutFileName);
            __android_log_print(ANDROID_LOG_ERROR, "native-log", "3");
            return EXIT_FAILURE;
        }

        // Clear the output file for results of analysis
        if (pOut2 != NULL)
        {
            fclose(pOut2);
            pOut2 = NULL;
        }
        pOut2 = fopen(lpszAnaRstFileName, "w");
        if (pOut2 == NULL)
        {
            printf("<br/>%s open failed<br/>", lpszAnaRstFileName);
            __android_log_print(ANDROID_LOG_ERROR, "native-log", "4");
            return EXIT_FAILURE;
        }

        // Clear the output file for results of QRS finding
        if (pOut4 != NULL)
        {
            fclose(pOut4);
            pOut4 = NULL;
        }
        pOut4 = fopen(lpszQRSfndFileName, "w");
        if (pOut4 == NULL)
        {
            printf("<br/>%s open failed<br/>", lpszQRSfndFileName);
            __android_log_print(ANDROID_LOG_ERROR, "native-log", "5");
            return EXIT_FAILURE;
        }

        // Clear the output down-sampling text file
        if (pOut3 != NULL)
        {
            fclose(pOut3);
            pOut3 = NULL;
        }
        pOut3 = fopen(lpszLongDispName, "w");
        if (pOut3 == NULL)
        {
            printf("<br/>%s open failed<br/>", lpszLongDispName);
            __android_log_print(ANDROID_LOG_ERROR, "native-log", "6");
            return EXIT_FAILURE;
        }


        for(filePageN = 0; filePageN < maxPageN; filePageN++)
        {
            DS_GloRList0.clear();
            DS_GloRList1.clear();
            iterDS_GloRList0 = DS_GloRList0.begin();
            iterDS_GloRList1 = DS_GloRList1.begin();

            int retValue;
            retValue = fseek(pReadFile, filePageN*intFrameSize, SEEK_SET);
            if(retValue)
            {   printf("<br/>%s seek failed<br/>", lpszFileName);
                __android_log_print(ANDROID_LOG_ERROR, "native-log", "7");
                return EXIT_FAILURE;
            }

            // Store the original file data for output next time
            int nOriLastPageRead = FileFrameSize;	// 最後一頁心電圖資料的長度
            nBytesReceived = fread(pECGFrameOri, 1, FileFrameSize, pReadFile);
            if(nBytesReceived != FileFrameSize && filePageN == maxPageN-1)
            {	// 這是最後一頁
                nOriLastPageRead = nBytesReceived;
            }

            retValue = fseek(pReadFile, filePageN*intFrameSize, SEEK_SET);
            if(retValue)
            {   printf("<br/>%s seek failed<br/>", lpszFileName);
                __android_log_print(ANDROID_LOG_ERROR, "native-log", "8");
                return EXIT_FAILURE;
            }

            int dataCellSize;

            long long Comp1EndIdx = ECG_FRAME_SIZE;

            BWRpageEndIdx = ECG_FRAME_SIZE;

            dataCellSize = MaxPacketSize/3;

            float msPerPix;
            bool firstFail = false;
            int totalPix, linePix;
            short Comp1EndX;   // 心電圖資料檔結束點的x座標
            short Comp1EndY;   // 心電圖資料檔結束點的y座標
            int anaStartMax1 = ECG_FRAME_SIZE - ECG_ANALY_REWIND;   // 心電圖資料分析起始點的最大index
            short anaStartXMax1;   // 心電圖資料分析起始點的最大x座標
            short anaStartYMax1;   // 心電圖資料分析起始點的最大y座標

            // 由fileSize計算anaStartXMax1和anaStartYMax1
            msPerPix = (float)ECG_FRAME_SIZE / 2.0 / (float)ECGDgmWidth;  // Pixels per second (x axis)
            linePix = ECGDgmWidth;
            long long pageLength;
            {   // 非最後一頁
                pageLength = FileFrameSize;
                Comp1EndIdx = pageLength / ECGCellSize * dataCellSize;
                if(Comp1EndIdx <= ECG_ANALY_REWIND)
                    anaStartMax1 = 0;
                else
                    anaStartMax1 = Comp1EndIdx - ECG_ANALY_REWIND;
                totalPix = (int)( (float)anaStartMax1 / msPerPix );
                anaStartXMax1 = totalPix % linePix;
                anaStartYMax1 = totalPix / linePix;
            }

            loadOffset = 0;
            for(fi = 0; fi < FileFrameSize/ECGCellSize; fi++)
            {
                nBytesReceived = fread(ECGFrame0+loadOffset, 1, dataCellSize * 2, pReadFile);
                if(nBytesReceived != dataCellSize * 2)
                {	if( firstFail == false && filePageN == maxPageN-1 )
                    {	msPerPix = (float)ECG_FRAME_SIZE / 2.0 / (float)ECGDgmWidth;  // Pixels per second (x axis)
                        BWRpageEndIdx = fi * dataCellSize;
                        linePix = ECGDgmWidth;
                        if(BWRpageEndIdx <= ECG_ANALY_REWIND)
                            anaStartMax1 = 0;
                        else
                            anaStartMax1 = BWRpageEndIdx - ECG_ANALY_REWIND;
                        totalPix = (int)( (float)anaStartMax1 / msPerPix );
                        anaStartXMax1 = totalPix % linePix;
                        anaStartYMax1 = totalPix / linePix;
                        //printf("<br/>BWRpageEndIdx: %lld, anaStartMax1: %d, totalPix = %d.<br/>",
                        //    BWRpageEndIdx, anaStartMax1, totalPix);
                        firstFail = true;
                    }
                    for(fi2 = 0; fi2 < dataCellSize; fi2++)
                        ECGFrame0[loadOffset+fi2] = lastECGvalue0;  // avoiding nagetive saturation
                }

                nBytesReceived = fread(ECGFrame1+loadOffset, 1, dataCellSize * 2, pReadFile);
                if(nBytesReceived != dataCellSize * 2)
                {	for(fi2 = 0; fi2 < dataCellSize; fi2++)
                        ECGFrame1[loadOffset+fi2] = lastECGvalue1;  // avoiding nagetive saturation
                }

                // Lead III = Lead II - lead I
                unsigned short LeadIRec, LeadIIRec;
                for(fi2 = 0; fi2 < dataCellSize; fi2++)
                {	if(filePageN == 0 && fi == 1 && fi2 < 6)
                        nodeIDori[fi2] = ECGFrame0[loadOffset+fi2] & 0xF000;
                    else if(filePageN == 0 && fi == 1 && fi2 > 6 && fi2 < 13)
                        OUIori[fi2-7] = ECGFrame0[loadOffset+fi2] & 0xF000;

                    ECGFrame0[loadOffset+fi2] = ECGFrame0[loadOffset+fi2] & 0xFFF;
                    ECGFrame1[loadOffset+fi2] = ECGFrame1[loadOffset+fi2] & 0xFFF;

                    if( firstFail == false && fi2 == 2 )  // 保存一個心電圖資料檔中的最後一個cell的第3個資料點
                    {
                        lastECGvalue0 = ECGFrame0[loadOffset+fi2];
                        lastECGvalue1 = ECGFrame1[loadOffset+fi2];
                    }

                    LeadIRec = ECGFrame0[loadOffset+fi2];
                    LeadIIRec = ECGFrame1[loadOffset+fi2];
                    ECGFrame2[loadOffset+fi2] = ECGADC_MAX + LeadIIRec - LeadIRec;

                    // Because Lead III was added ECGADC_MAX (avoiding nagetive saturation), so Lead I and Lead II must be added ECGADC_MIDDLE
                    ECGFrame0[loadOffset+fi2] += ECGADC_MIDDLE;
                    ECGFrame1[loadOffset+fi2] += ECGADC_MIDDLE;
                }

                nBytesReceived = fread(tempDateTime, 1, 8, pReadFile);

                if(fi == 0)
                    for(fi2 = 0; fi2 < 8; fi2++)
                        DateTimeComp1[fi2] = tempDateTime[fi2];

                loadOffset += dataCellSize;
            }

            unsigned short tempNodeID01;					// 處理(組合)心電圖儀node ID時會用到的暫時變數
            unsigned short tempNodeID02;					// 處理(組合)心電圖儀node ID時會用到的暫時變數

            // 組合出心電圖儀的EUI node ID (24 bits)
            tempNodeID01 = nodeIDori[0];
            tempNodeID01 = tempNodeID01 >> 8;
            tempNodeID02 = nodeIDori[1];
            tempNodeID02 = tempNodeID02 >> 12;
            ECGnodeID[0] = tempNodeID01 | tempNodeID02;
            tempNodeID01 = nodeIDori[2];
            tempNodeID01 = tempNodeID01 >> 8;
            tempNodeID02 = nodeIDori[3];
            tempNodeID02 = tempNodeID02 >> 12;
            ECGnodeID[1] = tempNodeID01 | tempNodeID02;
            tempNodeID01 = nodeIDori[4];
            tempNodeID01 = tempNodeID01 >> 8;
            tempNodeID02 = nodeIDori[5];
            tempNodeID02 = tempNodeID02 >> 12;
            ECGnodeID[2] = tempNodeID01 | tempNodeID02;

            // 組合出心電圖儀的OUI ID (24 bits)
            tempNodeID01 = OUIori[0];
            tempNodeID01 = tempNodeID01 >> 8;
            tempNodeID02 = OUIori[1];
            tempNodeID02 = tempNodeID02 >> 12;
            ECG_OUI[0] = tempNodeID01 | tempNodeID02;
            tempNodeID01 = OUIori[2];
            tempNodeID01 = tempNodeID01 >> 8;
            tempNodeID02 = OUIori[3];
            tempNodeID02 = tempNodeID02 >> 12;
            ECG_OUI[1] = tempNodeID01 | tempNodeID02;
            tempNodeID01 = OUIori[4];
            tempNodeID01 = tempNodeID01 >> 8;
            tempNodeID02 = OUIori[5];
            tempNodeID02 = tempNodeID02 >> 12;
            ECG_OUI[2] = tempNodeID01 | tempNodeID02;

            // 組合出此心電圖區段的紀錄時間字串
            unsigned char yr, mth, day, hr, min, sec, tempByte;
            yr = DateTimeComp1[6];
            mth = DateTimeComp1[5];
            day = DateTimeComp1[4];
            hr = DateTimeComp1[2];
            min = DateTimeComp1[1];
            sec = DateTimeComp1[0];

            unsigned char DataTime_str[21];

            DataTime_str[0] = '2';
            DataTime_str[1] = '0';

            DataTime_str[2] = (yr >> 4) + '0';
            DataTime_str[3] = (yr & 0xF) + '0';
            DataTime_str[4] = '.';

            tempByte = mth & 0x1F;
            DataTime_str[5] = (tempByte >> 4) + '0';
            DataTime_str[6] = (tempByte & 0xF) + '0';
            DataTime_str[7] = '.';

            tempByte = day & 0x3F;
            DataTime_str[8] = (tempByte >> 4) + '0';
            DataTime_str[9] = (tempByte & 0xF) + '0';

            DataTime_str[10] = ',';
            DataTime_str[11] = ' ';

            DataTime_str[14] = ':';
            DataTime_str[17] = ':';

            tempByte = hr & 0x3F;
            DataTime_str[12] = (tempByte >> 4) + '0';
            DataTime_str[13] = (tempByte & 0xF) + '0';

            tempByte = min & 0x7F;
            DataTime_str[15] = (tempByte >> 4) + '0';
            DataTime_str[16] = (tempByte & 0xF) + '0';

            tempByte = sec & 0x7F;
            DataTime_str[18] = (tempByte >> 4) + '0';
            DataTime_str[19] = (tempByte & 0xF) + '0';

            DataTime_str[20] = '\0';

            // 準備BWRFrame

            bool BWRfirstPage;	// 目前是第一頁, 沒有前一頁
            BWRFrameSize = 0;
            BWRfirstPage = false;
            if(filePageN > 0)
            {	// 不是第一頁, 填入BWRpreFrame
                retValue = fseek(pReadFile, (filePageN-1)*intFrameSize, SEEK_SET);
                if(retValue)
                {   printf("<br/>%s seek failed<br/>", lpszFileName);
                    __android_log_print(ANDROID_LOG_ERROR, "native-log", "9");
                    return EXIT_FAILURE;
                }

                loadOffset = 0;

                for(fi = 0; fi < FileFrameSize/ECGCellSize; fi++)
                {
                    nBytesReceived = fread(BWRpreFrame0+loadOffset, 1, dataCellSize * 2, pReadFile);
                    if(nBytesReceived != dataCellSize * 2)
                    {	for(fi2 = 0; fi2 < dataCellSize; fi2++)
                            BWRpreFrame0[loadOffset+fi2] = ECGADC_MIDDLE;  // avoiding nagetive saturation
                    }

                    nBytesReceived = fread(BWRpreFrame1+loadOffset, 1, dataCellSize * 2, pReadFile);
                    if(nBytesReceived != dataCellSize * 2)
                    {	for(fi2 = 0; fi2 < dataCellSize; fi2++)
                            BWRpreFrame1[loadOffset+fi2] = ECGADC_MIDDLE;  // avoiding nagetive saturation
                    }

                    // Lead III = Lead II - lead I
                    unsigned short LeadIRec, LeadIIRec;
                    for(fi2 = 0; fi2 < dataCellSize; fi2++)
                    {	BWRpreFrame0[loadOffset+fi2] = BWRpreFrame0[loadOffset+fi2] & 0xFFF;
                        BWRpreFrame1[loadOffset+fi2] = BWRpreFrame1[loadOffset+fi2] & 0xFFF;

                        LeadIRec = BWRpreFrame0[loadOffset+fi2];
                        LeadIIRec = BWRpreFrame1[loadOffset+fi2];
                        BWRpreFrame2[loadOffset+fi2] = ECGADC_MAX + LeadIIRec - LeadIRec;

                        // Because Lead III was added ECGADC_MAX (avoiding nagetive saturation), so Lead I and Lead II must be added ECGADC_MIDDLE
                        BWRpreFrame0[loadOffset+fi2] += ECGADC_MIDDLE;
                        BWRpreFrame1[loadOffset+fi2] += ECGADC_MIDDLE;
                    }

                    nBytesReceived = fread(tempDateTime, 1, 8, pReadFile);

                    loadOffset += dataCellSize;
                }

                // 將BWRpreFrame尾端資料填入BWRFrame
                for(fi = 0; fi < preFrameAddSize; fi++)
                {	BWRFrame0[fi] = BWRpreFrame0[ ECG_FRAME_SIZE - preFrameAddSize + fi ];
                    BWRFrame1[fi] = BWRpreFrame1[ ECG_FRAME_SIZE - preFrameAddSize + fi ];
                    BWRFrame2[fi] = BWRpreFrame2[ ECG_FRAME_SIZE - preFrameAddSize + fi ];
                }
                BWRFrameSize += preFrameAddSize;

                // 將ECGFrame資料填入BWRFrame
                int BWRendIdx = ECG_FRAME_SIZE;

                if(BWRpageEndIdx < BWRendIdx)
                    BWRendIdx = BWRpageEndIdx;

                for(fi = 0; fi < BWRendIdx; fi++)
                {	BWRFrame0[ preFrameAddSize + fi ] = ECGFrame0[fi];
                    BWRFrame1[ preFrameAddSize + fi ] = ECGFrame1[fi];
                    BWRFrame2[ preFrameAddSize + fi ] = ECGFrame2[fi];
                }
                BWRFrameSize += BWRendIdx;

                if(filePageN < maxPageN-1)
                {	// 不是最後一頁, 將後一頁資料填入BWRpostFrame

                    int BWRnextPageEnd = ECG_FRAME_SIZE;

                    retValue = fseek(pReadFile, (filePageN+1)*intFrameSize, SEEK_SET);
                    if(retValue)
                    {   printf("<br/>%s seek failed<br/>", lpszFileName);
                        __android_log_print(ANDROID_LOG_ERROR, "native-log", "10");
                        return EXIT_FAILURE;
                    }

                    loadOffset = 0;
                    firstFail = false;
                    for(fi = 0; fi < FileFrameSize/ECGCellSize; fi++)
                    {
                        nBytesReceived = fread(BWRpostFrame0+loadOffset, 1, dataCellSize * 2, pReadFile);
                        if(nBytesReceived != dataCellSize * 2)
                        {	if( firstFail == false && filePageN+1 == maxPageN-1 )
                            {	BWRnextPageEnd = fi * dataCellSize;
                                firstFail = true;
                            }
                            for(fi2 = 0; fi2 < dataCellSize; fi2++)
                                BWRpostFrame0[loadOffset+fi2] = lastECGvalue0;  // avoiding nagetive saturation
                        }

                        nBytesReceived = fread(BWRpostFrame1+loadOffset, 1, dataCellSize * 2, pReadFile);
                        if(nBytesReceived != dataCellSize * 2)
                        {	for(fi2 = 0; fi2 < dataCellSize; fi2++)
                                BWRpostFrame1[loadOffset+fi2] = lastECGvalue1;  // avoiding nagetive saturation
                        }

                        // Lead III = Lead II - lead I
                        unsigned short LeadIRec, LeadIIRec;
                        for(fi2 = 0; fi2 < dataCellSize; fi2++)
                        {	BWRpostFrame0[loadOffset+fi2] = BWRpostFrame0[loadOffset+fi2] & 0xFFF;
                            BWRpostFrame1[loadOffset+fi2] = BWRpostFrame1[loadOffset+fi2] & 0xFFF;

                            if( firstFail == false && fi2 == 2 )  // 保存一個心電圖資料檔中的最後一個cell的第3個資料點
                            {
                                lastECGvalue0 = BWRpostFrame0[loadOffset+fi2];
                                lastECGvalue1 = BWRpostFrame1[loadOffset+fi2];
                            }

                            LeadIRec = BWRpostFrame0[loadOffset+fi2];
                            LeadIIRec = BWRpostFrame1[loadOffset+fi2];
                            BWRpostFrame2[loadOffset+fi2] = ECGADC_MAX + LeadIIRec - LeadIRec;

                            // Because Lead III was added ECGADC_MAX (avoiding nagetive saturation), so Lead I and Lead II must be added ECGADC_MIDDLE
                            BWRpostFrame0[loadOffset+fi2] += ECGADC_MIDDLE;
                            BWRpostFrame1[loadOffset+fi2] += ECGADC_MIDDLE;
                        }

                        nBytesReceived = fread(tempDateTime, 1, 8, pReadFile);

                        loadOffset += dataCellSize;
                    }

                    BWRendIdx = postFrameAddSize;

                    //if(BWRnextPageEnd < BWRendIdx)
                    //	BWRendIdx = BWRnextPageEnd;

                    // 將BWRpostFrame資料填入BWRFrame
                    for(fi = 0; fi < BWRendIdx; fi++)
                    {	BWRFrame0[ preFrameAddSize + ECG_FRAME_SIZE + fi ] = BWRpostFrame0[fi];
                        BWRFrame1[ preFrameAddSize + ECG_FRAME_SIZE + fi ] = BWRpostFrame1[fi];
                        BWRFrame2[ preFrameAddSize + ECG_FRAME_SIZE + fi ] = BWRpostFrame2[fi];
                    }
                    BWRFrameSize += BWRendIdx;
                }
                else
                {
                    // 最後一頁, 將ECGADC_MIDDLE填入BWRFrame尾端

                    for(fi = 0; fi < postFrameAddSize; fi++)
                    {	BWRFrame0[ preFrameAddSize + ECG_FRAME_SIZE + fi ] = ECGADC_MIDDLE;
                        BWRFrame1[ preFrameAddSize + ECG_FRAME_SIZE + fi ] = ECGADC_MIDDLE;
                        BWRFrame2[ preFrameAddSize + ECG_FRAME_SIZE + fi ] = ECGADC_MIDDLE;
                    }

                    BWRFrameSize += postFrameAddSize;
                }
            }
            else
            {	// 目前是第一頁，不用讀前一頁

                // 將BWRpreFrame尾端資料填入BWRFrame
                for(fi = 0; fi <= preFrameAddSize; fi++)
                {	BWRFrame0[fi] = ECGADC_MIDDLE;
                    BWRFrame1[fi] = ECGADC_MIDDLE;
                    BWRFrame2[fi] = ECGADC_MIDDLE;
                }
                BWRFrameSize += preFrameAddSize;

                // 將ECGFrame資料填入BWRFrame

                BWRfirstPage = true;
                int BWRendIdx = ECG_FRAME_SIZE;

                if(BWRpageEndIdx < BWRendIdx)
                    BWRendIdx = BWRpageEndIdx;

                for(fi = 1; fi < BWRendIdx; fi++)
                {	BWRFrame0[ preFrameAddSize + fi ] = ECGFrame0[fi];
                    BWRFrame1[ preFrameAddSize + fi ] = ECGFrame1[fi];
                    BWRFrame2[ preFrameAddSize + fi ] = ECGFrame2[fi];
                }
                BWRFrameSize += BWRendIdx;

                if(filePageN < maxPageN-1)
                {	// 不是最後一頁, 將後一頁資料填入m_BWRpostFrame

                    int BWRnextPageEnd = ECG_FRAME_SIZE;

                    retValue = fseek(pReadFile, (filePageN+1)*intFrameSize, SEEK_SET);
                    if(retValue)
                    {   printf("<br/>%s seek failed<br/>", lpszFileName);
                        __android_log_print(ANDROID_LOG_ERROR, "native-log", "11");
                        return EXIT_FAILURE;
                    }

                    loadOffset = 0;
                    firstFail = false;
                    for(fi = 0; fi < FileFrameSize/ECGCellSize; fi++)
                    {
                        nBytesReceived = fread(BWRpostFrame0+loadOffset, 1, dataCellSize * 2, pReadFile);
                        if(nBytesReceived != dataCellSize * 2)
                        {	if( firstFail == false && filePageN+1 == maxPageN-1 )
                            {	BWRnextPageEnd = fi * dataCellSize;
                                firstFail = true;
                            }
                            for(fi2 = 0; fi2 < dataCellSize; fi2++)
                                BWRpostFrame0[loadOffset+fi2] = lastECGvalue0;  // avoiding nagetive saturation
                        }

                        nBytesReceived = fread(BWRpostFrame1+loadOffset, 1, dataCellSize * 2, pReadFile);
                        if(nBytesReceived != dataCellSize * 2)
                        {	for(fi2 = 0; fi2 < dataCellSize; fi2++)
                                BWRpostFrame1[loadOffset+fi2] = lastECGvalue1;  // avoiding nagetive saturation
                        }

                        // Lead III = Lead II - lead I
                        unsigned short LeadIRec, LeadIIRec;
                        for(fi2 = 0; fi2 < dataCellSize; fi2++)
                        {	BWRpostFrame0[loadOffset+fi2] = BWRpostFrame0[loadOffset+fi2] & 0xFFF;
                            BWRpostFrame1[loadOffset+fi2] = BWRpostFrame1[loadOffset+fi2] & 0xFFF;

                            if( firstFail == false && fi2 == 2 )  // 保存一個心電圖資料檔中的最後一個cell的第3個資料點
                            {
                                lastECGvalue0 = BWRpostFrame0[loadOffset+fi2];
                                lastECGvalue1 = BWRpostFrame1[loadOffset+fi2];
                            }

                            LeadIRec = BWRpostFrame0[loadOffset+fi2];
                            LeadIIRec = BWRpostFrame1[loadOffset+fi2];
                            BWRpostFrame2[loadOffset+fi2] = ECGADC_MAX + LeadIIRec - LeadIRec;

                            // Because Lead III was added ECGADC_MAX (avoiding nagetive saturation), so Lead I and Lead II must be added ECGADC_MIDDLE
                            BWRpostFrame0[loadOffset+fi2] += ECGADC_MIDDLE;
                            BWRpostFrame1[loadOffset+fi2] += ECGADC_MIDDLE;
                        }

                        nBytesReceived = fread(tempDateTime, 1, 8, pReadFile);

                        loadOffset += dataCellSize;
                    }

                    BWRendIdx = postFrameAddSize;

                    //if(BWRnextPageEnd < BWRendIdx)
                    //	BWRendIdx = BWRnextPageEnd;

                    // 將BWRpostFrame資料填入BWRFrame
                    for(fi = 0; fi < BWRendIdx; fi++)
                    {	BWRFrame0[ preFrameAddSize + ECG_FRAME_SIZE + fi ] = BWRpostFrame0[fi];
                        BWRFrame1[ preFrameAddSize + ECG_FRAME_SIZE + fi ] = BWRpostFrame1[fi];
                        BWRFrame2[ preFrameAddSize + ECG_FRAME_SIZE + fi ] = BWRpostFrame2[fi];
                    }
                    BWRFrameSize += BWRendIdx;
                }
                else
                {
                    // 最後一頁, 將ECGADC_MIDDLE填入BWRFrame尾端

                    for(fi = 0; fi < postFrameAddSize; fi++)
                    {	BWRFrame0[ preFrameAddSize + ECG_FRAME_SIZE + fi ] = ECGADC_MIDDLE;
                        BWRFrame1[ preFrameAddSize + ECG_FRAME_SIZE + fi ] = ECGADC_MIDDLE;
                        BWRFrame2[ preFrameAddSize + ECG_FRAME_SIZE + fi ] = ECGADC_MIDDLE;
                    }

                    BWRFrameSize += postFrameAddSize;
                }
            }

            BWRFrame();		// Baseline Wandering Removal

            // 過濾電源雜訊

            // 將上一個Frame資料(NchPreFrame)尾端填入NchFrame
            for(fi = 0; fi < NOTCH_PREPAD_SIZE; fi++)
            {	NchFrame0[fi] = NchPreFrame0[ ECG_FRAME_SIZE - NOTCH_PREPAD_SIZE + fi ];
                NchFrame1[fi] = NchPreFrame1[ ECG_FRAME_SIZE - NOTCH_PREPAD_SIZE + fi ];
            }

            // 將ECGFrame資料填入NchFrame
            for(fi = 0; fi < ECG_FRAME_SIZE; fi++)
            {	NchFrame0[ NOTCH_PREPAD_SIZE + fi ] = ECGFrame0[fi];
                NchFrame1[ NOTCH_PREPAD_SIZE + fi ] = ECGFrame1[fi];
            }

            // 濾除電源線雜訊

            //if(filePageN == 0)
            {	// 第一頁，計算濾波器係數
                // 每一頁都計算濾波器係數
                for(fi = 0; fi < ECG_ANALY_SIZE; fi++)
                    ECGAnaData2[fi] = ECGFrame1[anaStartMax1+fi];

                // 找電源線雜訊
                PowerFFT();
                CalPNoiseFreq();
                if( !bPNoiseGot )
                {	// 再從Lead I中找電源雜訊
                    for(fi = 0; fi < ECG_ANALY_SIZE; fi++)
                        ECGAnaData2[fi] = ECGFrame0[anaStartMax1+fi];

                    // 找電源線雜訊
                    PowerFFT();
                    CalPNoiseFreq();
                }
                if(bPNoiseGot)
                {   CalNotchCoeff();
                    notchFrame();
                }
                else
                    nonNotchFrame();
            }
            //else
            //{
            //	if(bPNoiseGot)
            //		notchFrame();
            //	else
            //		nonNotchFrame();
            //}

            // 保存這次的Frame data
            for(fi = 0; fi < ECG_FRAME_SIZE; fi++)
            {   NchPreFrame0[fi] = ECGFrame0[fi];    // Lead I
                NchPreFrame1[fi] = ECGFrame1[fi];    // Lead II
            }

            // 取得Notch filter處理過的資料
            for(fi = 0; fi < ECG_FRAME_SIZE; fi++)
            {	ECGFrame0[fi] = NchFrame0[fi + NOTCH_PREPAD_SIZE];    // Lead I
                ECGFrame1[fi] = NchFrame1[fi + NOTCH_PREPAD_SIZE];    // Lead II
            }

            if (filePageN > 0)
            {	// Not the first frame
                for (fi = 0; fi < ECG_FRAME_SIZE / ( DS_FRAME_SIZE * DS_DOWNSAMPLE_R ); fi++)
                {
                    if (fi == 0)
                    {
                        int DSdownFrameIdx = 0;
                        for (fi2 = 0; fi2 < DS_SLOPE_SIZE * 2 * DS_DOWNSAMPLE_R; fi2++)
                        {
                            if (fi2 % DS_DOWNSAMPLE_R == 0)
                            {
                                if (DSdownFrameIdx < DS_FRAME_SIZE + DS_SLOPE_SIZE * 2)
                                {
                                    if(bSelLeadII)
                                        DSdownFrame[DSdownFrameIdx] = DSpreFrame1[ECG_FRAME_SIZE - DS_SLOPE_SIZE * 2 * DS_DOWNSAMPLE_R + fi2];    // Lead II
                                    else
                                        DSdownFrame[DSdownFrameIdx] = DSpreFrame0[ECG_FRAME_SIZE - DS_SLOPE_SIZE * 2 * DS_DOWNSAMPLE_R + fi2];    // Lead I
                                    DSdownFrameIdx++;
                                }
                            }
                        }
                        for (fi2 = 0; fi2 < DS_FRAME_SIZE * DS_DOWNSAMPLE_R; fi2++)
                        {
                            if (fi2 % DS_DOWNSAMPLE_R == 0)
                            {
                                if (DSdownFrameIdx < DS_FRAME_SIZE + DS_SLOPE_SIZE * 2)
                                {
                                    if (bSelLeadII)
                                        DSdownFrame[DSdownFrameIdx] = ECGFrame1[fi2];    // Lead II
                                    else
                                        DSdownFrame[DSdownFrameIdx] = ECGFrame0[fi2];    // Lead I
                                    DSdownFrameIdx++;
                                }
                            }
                        }
                    }
                    else
                    {
                        int DSdownFrameIdx = 0;
                        for (fi2 = 0; fi2 < (DS_FRAME_SIZE + DS_SLOPE_SIZE * 2) * DS_DOWNSAMPLE_R; fi2++)
                        {
                            if (fi2 % DS_DOWNSAMPLE_R == 0)
                            {
                                if (DSdownFrameIdx < DS_FRAME_SIZE + DS_SLOPE_SIZE * 2)
                                {
                                    if (bSelLeadII)
                                        DSdownFrame[DSdownFrameIdx] = ECGFrame1[fi2 + fi * DS_FRAME_SIZE * DS_DOWNSAMPLE_R - DS_SLOPE_SIZE * 2 * DS_DOWNSAMPLE_R];    // Lead II
                                    else
                                        DSdownFrame[DSdownFrameIdx] = ECGFrame0[fi2 + fi * DS_FRAME_SIZE * DS_DOWNSAMPLE_R - DS_SLOPE_SIZE * 2 * DS_DOWNSAMPLE_R];    // Lead I
                                    DSdownFrameIdx++;
                                }
                            }
                        }
                    }

                    DSFrameFindR();

                    // Translate the R position from the local downsampled time to the global time
                    int tempGloRPos = 0;
                    for (iterDS_RPosList = DS_RPosList.begin(); iterDS_RPosList != DS_RPosList.end(); ++iterDS_RPosList)
                    {
                        tempGloRPos = filePageN * ECG_FRAME_SIZE + (fi * DS_FRAME_SIZE - DS_FRA_ST_LEFTSH + (*iterDS_RPosList)) * DS_DOWNSAMPLE_R;

                        // We will call QRS Detection function twice. So we use two R lists.
                        DS_GloRList0.push_back(tempGloRPos);
                        DS_GloRList1.push_back(tempGloRPos);
                    }
                }
            }
            else
            {
                // The first frame
                for (fi = 3; fi < ECG_FRAME_SIZE / ( DS_FRAME_SIZE * DS_DOWNSAMPLE_R ); fi++)
                {
                    int DSdownFrameIdx = 0;
                    for (fi2 = 0; fi2 < (DS_FRAME_SIZE + DS_SLOPE_SIZE * 2) * DS_DOWNSAMPLE_R; fi2++)
                    {
                        if (fi2 % DS_DOWNSAMPLE_R == 0)
                        {
                            if (DSdownFrameIdx < DS_FRAME_SIZE + DS_SLOPE_SIZE * 2)
                            {
                                if (bSelLeadII)
                                    DSdownFrame[DSdownFrameIdx] = ECGFrame1[fi2 + fi * DS_FRAME_SIZE * DS_DOWNSAMPLE_R - DS_SLOPE_SIZE * 2 * DS_DOWNSAMPLE_R];    // Lead II
                                else
                                    DSdownFrame[DSdownFrameIdx] = ECGFrame0[fi2 + fi * DS_FRAME_SIZE * DS_DOWNSAMPLE_R - DS_SLOPE_SIZE * 2 * DS_DOWNSAMPLE_R];    // Lead I
                                DSdownFrameIdx++;
                            }
                        }
                    }

                    DSFrameFindR();

                    // Translate the R position from the local downsampled time to the global time
                    int tempGloRPos = 0;
                    for (iterDS_RPosList = DS_RPosList.begin(); iterDS_RPosList != DS_RPosList.end(); ++iterDS_RPosList)
                    {
                        tempGloRPos = filePageN * ECG_FRAME_SIZE + (fi * DS_FRAME_SIZE - DS_FRA_ST_LEFTSH + (*iterDS_RPosList)) * DS_DOWNSAMPLE_R;

                        // We will call QRS Detection function twice. So we use two R lists.
                        DS_GloRList0.push_back(tempGloRPos);
                        DS_GloRList1.push_back(tempGloRPos);
                    }
                }
            }

            // Fill the DSpreFrame[]
            for (fi = 0; fi < ECG_FRAME_SIZE; fi++)
            {
                DSpreFrame0[fi] = ECGFrame0[fi];    // Lead I
                DSpreFrame1[fi] = ECGFrame1[fi];    // Lead II
            }

            // output processed ECG data to the output file
            unsigned int nBytesWritten;
            loadOffset = 0;
            int prcedFrameIdx0 = 0;
            int prcedFrameIdx1 = 0;

            int nOutCell = nOriLastPageRead / ECGCellSize;
            int nLastCellSize = nOriLastPageRead % ECGCellSize;

            for(fi = 0; fi < nOutCell; fi++)
            {	// Lead I data
                for(fi2 = 0; fi2 < dataCellSize; fi2++)
                {	// 將處理過的心電圖資料寫入檔案
                    ECGFrame0[prcedFrameIdx0] = ECGFrame0[prcedFrameIdx0] & 0xFFF;
                    pECGFrameOri[loadOffset+fi2] = pECGFrameOri[loadOffset+fi2] & 0xF000;
                    pECGFrameOri[loadOffset+fi2] = pECGFrameOri[loadOffset+fi2] | ECGFrame0[prcedFrameIdx0];
                    prcedFrameIdx0++;
                }

                // Lead II data
                for(fi2 = 0; fi2 < dataCellSize; fi2++)
                {	// 將處理過的心電圖資料寫入檔案
                    ECGFrame1[prcedFrameIdx1] = ECGFrame1[prcedFrameIdx1] & 0xFFF;
                    pECGFrameOri[loadOffset+dataCellSize+fi2] = pECGFrameOri[loadOffset+dataCellSize+fi2] & 0xF000;
                    pECGFrameOri[loadOffset+dataCellSize+fi2] = pECGFrameOri[loadOffset+dataCellSize+fi2] | ECGFrame1[prcedFrameIdx1];
                    prcedFrameIdx1++;
                }

                nBytesWritten = fwrite ( pECGFrameOri + loadOffset, 1, ECGCellSize, pOut1 );
                if(nBytesWritten != ECGCellSize)
                {   // 關閉檔案
                    if(pOut1 != NULL)
                    {	fclose(pOut1);
                        pOut1 = NULL;
                    }
                    __android_log_print(ANDROID_LOG_ERROR, "native-log", "12");
                    return EXIT_FAILURE;
                }

                loadOffset += dataCellSize * 2 + 4;
            }

            // output processed ECG data to the down-sampling text file

            int LdDownSampRatio = LD_DOWNSAMP_RATIO;
            int LdSampInCell = dataCellSize / LdDownSampRatio;
            int LdSampIndex = 0;
            int LdTempSampValue = 0;
            float LdTempSampFloat = 0;
            float LdTempAVL = 0;
            float LdTempAVF = 0;
            float LdTempAVR = 0;
            int LdThisPage = inPageN-1;
            int LdFrameMin;
            int LdFrameMax;

            if( inPageN > 0 )
            {	LdFrameMin = ST_FRAME_FST_PAGE + LdThisPage * LD_FRAME_N_IN_PAGE;
                LdFrameMax = ST_FRAME_FST_PAGE + ( LdThisPage + 1 ) * LD_FRAME_N_IN_PAGE;	// Currently not used.
            }
            else
            {	LdFrameMin = 0;
                LdFrameMax = LD_FRAME_N_IN_PAGE;
            }

            //if( filePageN >= LdFrameMin && filePageN < LdFrameMax )
            if( filePageN >= LdFrameMin )
            {
                // For debug
                //printf( "%d<br>", filePageN );

                for(fi = 0; fi < nOutCell; fi++)
                {	for(fi2 = 0; fi2 < LdSampInCell; fi2++)
                    {
                        LdTempSampValue = ECGFrame0[LdSampIndex];	// Lead I

                        if( LdTempSampValue < LD_VALUE_LOW_LIMIT )
                            LdTempSampValue = LD_VALUE_LOW_LIMIT;

                        LdTempSampValue -= LD_VALUE_LOW_LIMIT;
                        float fLdTempSampValue = LdTempSampValue;
                        fLdTempSampValue = fLdTempSampValue / LD_VALUE_DOWNSAMP_R;
                        LdTempSampValue = fLdTempSampValue + 0.5;

                        if( LdTempSampValue > LD_VALUE_RANGE_LIMIT )
                            LdTempSampValue = LD_VALUE_RANGE_LIMIT;

                        printResult = fprintf( pOut3, "%03x", LdTempSampValue );
                        if(printResult < 0)
                        {    // 關閉檔案
                            if(pOut3 != NULL)
                            {	fclose(pOut3);
                                pOut3 = NULL;
                            }
                            __android_log_print(ANDROID_LOG_ERROR, "native-log", "13");
                            return EXIT_FAILURE;
                        }

                        LdTempSampValue = ECGFrame1[LdSampIndex];	// Lead II

                        if( LdTempSampValue < LD_VALUE_LOW_LIMIT )
                            LdTempSampValue = LD_VALUE_LOW_LIMIT;

                        LdTempSampValue -= LD_VALUE_LOW_LIMIT;
                        fLdTempSampValue = LdTempSampValue;
                        fLdTempSampValue = fLdTempSampValue / LD_VALUE_DOWNSAMP_R;
                        LdTempSampValue = fLdTempSampValue + 0.5;

                        if( LdTempSampValue > LD_VALUE_RANGE_LIMIT )
                            LdTempSampValue = LD_VALUE_RANGE_LIMIT;

                        printResult = fprintf( pOut3, "%03x", LdTempSampValue );
                        if(printResult < 0)
                        {    // 關閉檔案
                            if(pOut3 != NULL)
                            {	fclose(pOut3);
                                pOut3 = NULL;
                            }
                            __android_log_print(ANDROID_LOG_ERROR, "native-log", "14");
                            return EXIT_FAILURE;
                        }

                        LdSampIndex += LdDownSampRatio;
                    }
                }
            }

            // 準備進行QRS detection和分析

            if (bSelLeadII)
                for (fi = 0; fi < ECG_FRAME_SIZE; fi++)
                    ECGFilt[fi] = (float)(ECGFrame1[fi]);    // 對象是Lead II資料
            else
                for (fi = 0; fi < ECG_FRAME_SIZE; fi++)
                    ECGFilt[fi] = (float)(ECGFrame0[fi]);    // 對象是Lead I資料

            // 為電源線雜訊濾除準備心電圖資料
            //for(fi = 0; fi < ECG_ANALY_SIZE; fi++)
            //	ECGAnaData2[fi] = ECGFrame1[anaStartMax1+fi];

            // 濾除電源線雜訊
            //PowerFFT();
            //CalPNoiseFreq();
            //if(bPNoiseGot)
            //{   CalNotchCoeff();
            //	notchFrame(ECGFilt, ECGFilt);
            //}
            //else
            //	nonNotchFrame(ECGFilt, ECGFilt);

            // 開始分析
            int thisBeatType, thisBeatMatch, beatClasRet, lastBeatMatch;
            unsigned short tempSample;
            int intECGtemp;
            double markLoc;  // Beat type mark location

            lastBeatMatch = ACMAX + 1;	// 超過心跳種類碼最大值，表示一開始要更新心跳種類碼

            int allFrameSize = nOutCell * (MaxPacketSize/3);
            for(fi = 0; fi < allFrameSize; fi++)
            {	//if(fi % 5 == 4)
                {
                    // For debug
                    //m_debugInt = fi;

                    int curIdxTestR = filePageN * ECG_FRAME_SIZE + fi - (DS_FRA_ST_LEFTSH * DS_DOWNSAMPLE_R);
                    if (curIdxTestR < 0)
                        continue;

                    // Adjust the magnitude of ECG signal value to fit 'short' type
                    //shECGtemp = (short)( (tempFind - findMin) / (findMax - findMin) * 4096.0 );
                    intECGtemp = (int)(ECGFilt[fi]);

                    // QRS finding and output
                    //detDlyForRFnd = QRSDetForRFnd.QRSDet(intECGtemp, 0);

                    if (DS_GloRList0.size() == 0)
                        continue;

                    iterDS_GloRList0 = DS_GloRList0.begin();
                    while (curIdxTestR > (*iterDS_GloRList0))
                    {
                        ++iterDS_GloRList0;
                        if (iterDS_GloRList0 == DS_GloRList0.end())
                            break;
                    }

                    if (iterDS_GloRList0 != DS_GloRList0.end())
                    {
                        //if (detDlyForRFnd)
                        if (curIdxTestR == (*iterDS_GloRList0))
                        {
                            //markLoc = ( filePageN * ECG_FRAME_SIZE + fi - detDlyForRFnd * 5 ) / 1000.0;
                            markLoc = ( curIdxTestR ) / 1000.0;
                            if( markLoc > 0.0 )
                            {	printResult = fprintf( pOut4, "QRS location: %f\n", markLoc );
                                if(printResult < 0)
                                {    // 關閉檔案
                                    if(pOut4 != NULL)
                                    {	fclose(pOut4);
                                        pOut4 = NULL;
                                    }
                                    __android_log_print(ANDROID_LOG_ERROR, "native-log", "15");
                                    return EXIT_FAILURE;
                                }

                                if (!b1stR4LeadSel)
                                {
                                    // After the 1st R found
                                    // Calculate the number of the Heart Rates which have been labeled "Good" (HR_LEADSEL_MIN < HR4LeadSel < HR_LEADSEL_MAX)
                                    RR4LeadSel = markLoc - LastR4LeadSel;
                                    HR4LeadSel = 60.0 / RR4LeadSel;
                                    if (HR4LeadSel > HR_LEADSEL_MIN && HR4LeadSel < HR_LEADSEL_MAX)
                                        HRGoodN++;
                                    HRTot4LeadSel += 1.0;
                                }

                                b1stR4LeadSel = false;
                                LastR4LeadSel = markLoc;
                            }
                        }
                    }

                    iterDS_GloRList1 = DS_GloRList1.begin();
                    while (curIdxTestR > (*iterDS_GloRList1))
                    {
                        ++iterDS_GloRList1;
                        if (iterDS_GloRList1 == DS_GloRList1.end())
                            break;
                    }

                    if (iterDS_GloRList1 == DS_GloRList1.end())
                    {
                        bDS_GloRfndForCla = false;
                    }
                    else
                    {
                        if (curIdxTestR == (*iterDS_GloRList1))
                            bDS_GloRfndForCla = true;
                        else
                            bDS_GloRfndForCla = false;
                    }

                    // Beat Classification
                    beatClasRet = aBeatClassifyForAna.BeatDetectAndClassify(intECGtemp, &thisBeatType, &thisBeatMatch);
                    if(beatClasRet)
                    {
                        markLoc = ( curIdxTestR - beatClasRet ) / 1000.0;
                        if( markLoc > 0.0 )
                        {	printResult = fprintf( pOut2, "A beat classified: " );
                            //printResult = printf( "A beat classified: " );
                            if(printResult < 0)
                            {    // 關閉檔案
                                if(pOut2 != NULL)
                                {	fclose(pOut2);
                                    pOut2 = NULL;
                                }
                                __android_log_print(ANDROID_LOG_ERROR, "native-log", "16");
                                return EXIT_FAILURE;
                            }

                            if(bFirRfnd)
                            {	double tempRR_RMSSD = (markLoc - lastMarkLoc) * 1000.0;
                                double tempHR = 60.0 / (markLoc - lastMarkLoc);
                                double tempHRtime = (markLoc + lastMarkLoc) / 2.0;
                                //double tempRR_flt4UID = markLoc - lastMarkLoc;
                                double tempRR_flt4UID = (markLoc - lastMarkLoc) * 1000.0;

                                // Store Heart Rate for HRV Time domain analysis
                                if( tempHR > HRV_WIN_LOW_LIMIT && tempHR < HRV_WIN_UP_LIMIT )
                                {	HRList.push_back(tempHR);
                                    RRList4UID.push_back(tempRR_flt4UID);
                                    // Store Heart Rate for HRV Frequency domain analysis
                                    if( HRV_HRIdx < HRV_HR_N_MAX )
                                    {	HRV_HR[HRV_HRIdx] = (float)tempHR;
                                        HRV_Time[HRV_HRIdx] = (float)tempHRtime;
                                        HRV_HRIdx++;
                                    }
                                }

                                if( bFirRRfnd )
                                {
                                    RRSDList.push_back( fabs(lastRR_RMSSD - tempRR_RMSSD) );
                                }

                                lastRR_RMSSD = tempRR_RMSSD;
                                bFirRRfnd = true;
                            }

                            bFirRfnd = true;

                            if( thisBeatType == NORMAL )
                            {
                                if( lastBeatType == NORMAL )
                                {
                                    // Calculate NN50
                                    double tempRR = (markLoc - lastMarkLoc) * 1000.0;

                                    if( bFirNNfnd )
                                    {	if( fabs(lastRR - tempRR) > 50 )
                                            HRV_50n++;
                                        HRV_tot++;
                                    }

                                    lastRR = tempRR;

                                    double tempHRNN = 60.0 / (markLoc - lastMarkLoc);
                                    if( tempHRNN > HRV_WIN_LOW_LIMIT && tempHRNN < HRV_WIN_UP_LIMIT )
                                        HRNNList.push_back(tempHRNN);

                                    bFirNNfnd = true;
                                }

                                if (aBeatClassifyForAna.ST_elevation)
                                {
                                    printResult = fprintf(pOut2, "ST_ele, R location: %f\n", markLoc);
                                    //printResult = fprintf( pOut2, "R location: %f\n", markLoc );
                                    if (printResult < 0)
                                    {    // 關閉檔案
                                        if (pOut2 != NULL)
                                        {
                                            fclose(pOut2);
                                            pOut2 = NULL;
                                        }
                                        __android_log_print(ANDROID_LOG_ERROR, "native-log", "17");
                                        return EXIT_FAILURE;
                                    }
                                }
                                    //else if( aBeatClassifyForAna.bAF_fnd && bLastAFfound )
                                else if( aBeatClassifyForAna.bAF_fnd )
                                {
                                    printResult = fprintf( pOut2, "AF_fnd, R location: %f\n", markLoc );
                                    //printResult = fprintf( pOut2, "R location: %f\n", markLoc );
                                    if(printResult < 0)
                                    {    // 關閉檔案
                                        if(pOut2 != NULL)
                                        {	fclose(pOut2);
                                            pOut2 = NULL;
                                        }
                                        __android_log_print(ANDROID_LOG_ERROR, "native-log", "18");
                                        return EXIT_FAILURE;
                                    }
                                }
                                else
                                {
                                    printResult = fprintf( pOut2, "Normal, R location: %f\n", markLoc );
                                    //printResult = fprintf( pOut2, "R location: %f\n", markLoc );
                                    if(printResult < 0)
                                    {    // 關閉檔案
                                        if(pOut2 != NULL)
                                        {	fclose(pOut2);
                                            pOut2 = NULL;
                                        }
                                        __android_log_print(ANDROID_LOG_ERROR, "native-log", "19");
                                        return EXIT_FAILURE;
                                    }
                                }

                                if( bFirNorBfnd == false )
                                    bFirNorBfnd = true;
                            }
                            else
                            {
                                bAbnorFound = true;
                                //bFirNorBfnd = false;

                                if(thisBeatType == PVC)
                                {
                                    printResult = fprintf( pOut2, "PVCfnd, R location: %f\n", markLoc );
                                    //printResult = fprintf( pOut2, "R location: %f\n", markLoc );
                                    if(printResult < 0)
                                    {    // 關閉檔案
                                        if(pOut2 != NULL)
                                        {	fclose(pOut2);
                                            pOut2 = NULL;
                                        }
                                        __android_log_print(ANDROID_LOG_ERROR, "native-log", "20");
                                        return EXIT_FAILURE;
                                    }

                                    //break;
                                }
                                    //else if( aBeatClassifyForAna.bAF_fnd && bLastAFfound )
                                else if( aBeatClassifyForAna.bAF_fnd )
                                {
                                    printResult = fprintf( pOut2, "AF_fnd, R location: %f\n", markLoc );
                                    //printResult = fprintf( pOut2, "R location: %f\n", markLoc );
                                    if(printResult < 0)
                                    {    // 關閉檔案
                                        if(pOut2 != NULL)
                                        {	fclose(pOut2);
                                            pOut2 = NULL;
                                        }
                                        __android_log_print(ANDROID_LOG_ERROR, "native-log", "21");
                                        return EXIT_FAILURE;
                                    }
                                }
                                else
                                {
                                    printResult = fprintf( pOut2, "%06d, R location: %f\n", thisBeatType, markLoc );
                                    //printResult = fprintf( pOut2, "R location: %f\n", markLoc );
                                    if(printResult < 0)
                                    {    // 關閉檔案
                                        if(pOut2 != NULL)
                                        {	fclose(pOut2);
                                            pOut2 = NULL;
                                        }
                                        __android_log_print(ANDROID_LOG_ERROR, "native-log", "22");
                                        return EXIT_FAILURE;
                                    }
                                }
                            }

                            lastMarkLoc = markLoc;
                            lastBeatType = thisBeatType;
                            bLastAFfound = aBeatClassifyForAna.bAF_fnd;
                        }
                    }
                }
            }
        }

        // Calculate the ratio of "Good" to "Total" HR number for Lead Selection
        HRGood4LeadSel = (double)HRGoodN;
        if (HRTot4LeadSel != 0.0)
            HRGood2Tot = HRGood4LeadSel / HRTot4LeadSel;
        else
            HRGood2Tot = 0.0;

        // For debug
        //printResult = fprintf(pOut2, "Good HR ratio: %lf\n", HRGood2Tot);
        //if (printResult < 0)
        //{    // 關閉檔案
        //	if (pOut2 != NULL)
        //	{
        //		fclose(pOut2);
        //		pOut2 = NULL;
        //	}
        //	return EXIT_FAILURE;
        //}

        // Invert bSelLeadII (Select the other Lead)
        if (bSelLeadII)
        {	bSelLeadII = false;
            HRG2TLeadII = HRGood2Tot;
        }
        else
        {	bSelLeadII = true;
            if(HRGood2Tot > HRG2TLeadII && HRGoodN > HR_GOOD_THRESH)
                break;
        }

        TryLeadSelN++;
        if (TryLeadSelN >= 3)
            //if (TryLeadSelN >= 2)
            break;
    }
    while (HRGoodN < HR_GOOD_THRESH || HRGood2Tot < HR_GOOD_RATIO_TRSH);
    //while (1);

    // 統計HRV

    // Calculate the frequency-domain parameters of HRV

    int HRV_Size = HRV_HRIdx;
    float ofac, hifac;
    unsigned long  nout;
    unsigned long ndim, nfreq, nfreqt;

    ofac = 4.0;
    hifac = 2.0;

    nout = 0.5*ofac*hifac*HRV_Size;

    nfreqt = ofac*hifac*HRV_Size*MACC;
    nfreq = 64;
    while (nfreq < nfreqt) nfreq <<= 1;
    ndim = nfreq << 1;

    float * wk1 = new float[ndim];
    float * wk2 = new float[ndim];
    unsigned long nwk = ndim;
    unsigned long jmax;
    float prob;

    fasper(HRV_HR-1, HRV_Time-1, HRV_Size, ofac, hifac, wk1-1, wk2-1, nwk, &nout, &jmax, &prob);

    float HRV_FreqLo[3];
    float HRV_FreqHi[3];

    HRV_FreqLo[0] = 0.0;
    HRV_FreqLo[1] = 0.04;
    HRV_FreqLo[2] = 0.15;
    HRV_FreqHi[0] = 0.04;
    HRV_FreqHi[1] = 0.15;
    HRV_FreqHi[2] = 0.4;

    double HRV_TtlPwr = 0.0;
    double HRV_Pwr[3];

    // For debug
    //printf("HRV_Size: %d, nout: %ld.\n", HRV_Size, nout);

    for( fi = 0; wk1[fi] <= HRV_FreqHi[2] && fi < nout; fi++ )
    {	HRV_TtlPwr += wk2[fi];
        for ( fi2 = 0; fi2 < 3; fi2++ ) {
            if (wk1[fi] > HRV_FreqLo[fi2] && wk1[fi] <= HRV_FreqHi[fi2])
                HRV_Pwr[fi2] += wk2[fi];
        }

        // For debug
        //printf("wk1[%d]: %.3f, wk2[%d]: %.3f.\n", fi, wk1[fi], fi, wk2[fi]);
    }

    double HRV_VLF, HRV_LF, HRV_HF;

    HRV_VLF = HRV_Pwr[0] / HRV_TtlPwr;
    HRV_LF = HRV_Pwr[1] / HRV_TtlPwr;
    HRV_HF = HRV_Pwr[2] / HRV_TtlPwr;

    delete [] wk1;
    delete [] wk2;

    // 計算得到的Heart rate數目, 最大值和最小值
    double nHRsize = 0.0;
    double HRfileMax = 0.0;
    double HRfileMin = 500.0;	// 一個不合理的Heart rate大數值，用於計算最小值
    for( iterHRList = HRList.begin(); iterHRList != HRList.end(); ++iterHRList )
    {
        nHRsize += 1.0;
        if( HRfileMax < (*iterHRList) )
            HRfileMax = (*iterHRList);
        if( HRfileMin > (*iterHRList) )
            HRfileMin = (*iterHRList);
    }

    // 計算得到的RR interval數目
    double nRRsize = 0.0;
    int RRidx4UID = 0;
    for( iterRRList4UID = RRList4UID.begin(); iterRRList4UID != RRList4UID.end(); ++iterRRList4UID )
    {
        if(RRidx4UID < RR_N_MAX_4UID)
        {	RRarray4UID[RRidx4UID] = (*iterRRList4UID);
            RRidx4UID++;
            nRRsize += 1.0;
        }
    }

    if( nHRsize > 0.0 )
    {
        // HRList裡面不是空的

        // Calculate the HR average

        double HRavg;
        HRavg = 0.0;

        for( iterHRList = HRList.begin(); iterHRList != HRList.end(); ++iterHRList )
        {
            HRavg += *iterHRList;
        }
        HRavg = HRavg / nHRsize;

        double HRstdDev;		// Standard deviation
        double HRmsAvg;

        HRstdDev = 0.0;
        HRmsAvg = 0.0;

        double nHRNNsize = 0.0;
        for( iterHRNNList = HRNNList.begin(); iterHRNNList != HRNNList.end(); ++iterHRNNList )
        {
            HRmsAvg += 60000.0 / (*iterHRNNList);		// Unit: ms
            nHRNNsize += 1.0;
        }
        HRmsAvg = HRmsAvg / nHRNNsize;

        for( iterHRNNList = HRNNList.begin(); iterHRNNList != HRNNList.end(); ++iterHRNNList )
            HRstdDev += ( 60000.0 / (*iterHRNNList) - HRmsAvg ) * ( 60000.0 / (*iterHRNNList) - HRmsAvg );	// Unit: ms

        HRstdDev = sqrt( HRstdDev / nHRNNsize );

        double PNN50;
        if( HRV_tot != 0 )
            PNN50 = (double)HRV_50n / (double)HRV_tot;
        else
            PNN50 = 0.0;

        double HRV_RMSSD = 0.0;		// Root mean square of the successive differences of RR intervals
        if(RRSDList.size() > 0)
        {	double nRRSDsize = RRSDList.size();
            for( iterRRSDList = RRSDList.begin(); iterRRSDList != RRSDList.end(); ++iterRRSDList )
                HRV_RMSSD += (*iterRRSDList) * (*iterRRSDList);	// Unit: ms

            HRV_RMSSD = sqrt( HRV_RMSSD / nRRSDsize );
        }

        double HRV_PI = Cal_HRV_PI(RRarray4UID, (int)nRRsize );
        double HRV_GI = Cal_HRV_GI(RRarray4UID, (int)nRRsize );
        double HRV_SI = Cal_HRV_SI(RRarray4UID, (int)nRRsize );
        double HRV_CVI = Cal_HRV_CVI(RRarray4UID, (int)nRRsize );
        double HRV_CD = Cal_HRV_CD(RRarray4UID, (int)nRRsize );
        double HRV_C1a = Cal_HRV_C1a(RRarray4UID, (int)nRRsize );
        double HRV_MedianNN = 0.0;
        double HRV_ShanEn = Cal_HRV_ShanEn(RRarray4UID, (int)nRRsize, &HRV_MedianNN );

        printResult = fprintf( pOut2, "Max:%.3f,Min:%.3f,Average:%.3f, Standard Deviation:%.3f,VLF:%.3f,LF:%.3f,HF:%.3f,NN50:%d,PNN50:%.5f,RMSSD:%.3f,MedianNN:%.3f,PI:%.4f,GI:%.4f,SI:%.4f,CVI:%.4f,CD:%.4f,C1a:%.4f,ShanEn:%.4f\n",
                               HRfileMax, HRfileMin, HRavg, HRstdDev, HRV_VLF, HRV_LF, HRV_HF, HRV_50n,
                               PNN50, HRV_RMSSD, HRV_MedianNN, HRV_PI, HRV_GI, HRV_SI, HRV_CVI, HRV_CD, HRV_C1a,
                               HRV_ShanEn );

        if(printResult < 0)
        {   // 關閉檔案
            if(pOut2 != NULL)
            {	fclose(pOut2);
                pOut2 = NULL;
            }
            __android_log_print(ANDROID_LOG_ERROR, "native-log", "23");
            return EXIT_FAILURE;
        }
    }

    //printf("<br/>The ECG file proccessed!<br/>");

    // 關閉檔案

    if(pOut1 != NULL)
    {	fclose(pOut1);
        pOut1 = NULL;
    }

    if(pOut2 != NULL)
    {	fclose(pOut2);
        pOut2 = NULL;
    }

    if(pOut4 != NULL)
    {	fclose(pOut4);
        pOut4 = NULL;
    }

    if(pOut3 != NULL)
    {	fclose(pOut3);
        pOut3 = NULL;
    }

    if(pReadFile != NULL)
    {	fclose(pReadFile);
        pReadFile = NULL;
    }

    char procEndMsg[CHA_FNAME_LEN_MAX];   // The output message at the end of the process of this ECG data file

    strcpy(procEndMsg, inCHAFileName);
    strcat(procEndMsg, ".cha processed!\n");

    printf("%s", procEndMsg);

    return EXIT_SUCCESS;
}

double Cal_HRV_PI(double rrIntervals[], int numIntervals) {
    double sumBelowLI = 0.0, sumAllPoints = 0.0;
    int i;

    for (i = 0; i < numIntervals - 1; i++) {
        double x = rrIntervals[i];
        double y = rrIntervals[i + 1];
        if(y != x)
        {
            if (y < x)
            {	sumBelowLI += 1.0;
            }
            sumAllPoints += 1.0;
        }
    }

    if(sumAllPoints != 0.0)
        return sumBelowLI / sumAllPoints;
    else
        return 0.0;
}

double Cal_HRV_GI(double rrIntervals[], int numIntervals) {
    double sumAboveLI = 0.0, sumAllPoints = 0.0;
    int i;

    for (i = 0; i < numIntervals - 1; i++) {
        double x = rrIntervals[i];
        double y = rrIntervals[i + 1];
        double distance = sqrt(pow((y - x), 2.0) / 2.0);

        if(y != x)
        {
            if (y > x) {
                sumAboveLI += distance;
            }
            sumAllPoints += distance;
        }
    }

    if(sumAllPoints != 0.0)
        return sumAboveLI / sumAllPoints;
    else
        return 0.0;
}

double Cal_HRV_SI(double rrIntervals[], int numPoints) {
    int i;
    double sumAboveLI = 0.0;
    double sumAllPoints = 0.0;

    // Calculate the phase angle for each point
    for (i = 0; i < numPoints - 1; i++) {
        double x = rrIntervals[i];
        double y = rrIntervals[i + 1];
        double phaseAngle = atan(y / x);

        if(y != x)
        {
            // Check if the point is above the line of Identity (LI)
            if (y > x) {
                sumAboveLI += phaseAngle;
            }

            sumAllPoints += phaseAngle;
        }
    }

    // Calculate and return the Slope Index
    if(sumAllPoints != 0.0)
        return sumAboveLI / sumAllPoints;
    else
        return 0.0;
}

double compute_std_dev(double *data, int n) {
    double mean = 0.0, variance = 0.0;
    for (int i = 0; i < n; i++) {
        mean += data[i];
    }
    mean /= n;

    for (int i = 0; i < n; i++) {
        variance += (data[i] - mean) * (data[i] - mean);
    }
    variance /= n;

    return sqrt(variance);
}

double Cal_HRV_CVI(double rr_intervals[], int n) {
    double *diff = (double*)(malloc((n-1) * sizeof(double)));
    if (!diff) {
        perror("Unable to allocate memory for R-R interval differences");
        return 0.0;
    }

    for (int i = 0; i < n - 1; i++) {
        diff[i] = rr_intervals[i+1] - rr_intervals[i];
    }

    double sd_rr = compute_std_dev(rr_intervals, n);
    double sd_diff = compute_std_dev(diff, n-1);

    double SD1 = sqrt(0.5 * sd_diff * sd_diff);
    double SD2 = sqrt(2 * sd_rr * sd_rr - 0.5 * sd_diff * sd_diff);

    //printf("4*SD1 (Transverse variability): %.2lf\n", 4 * SD1);
    //printf("4*SD2 (Longitudinal variability): %.2lf\n", 4 * SD2);

    free(diff);

    double cvi = (1.0) * log(4 * SD1 * SD2);

    return cvi;
}



double euclidean_distance(double* a, double* b, int len) {
    double sum = 0;
    for (int i = 0; i < len; i++)
        sum += (a[i] - b[i]) * (a[i] - b[i]);
    return sqrt(sum);
}

double calculate_correlation_sum(double radius, int RR_Data_Len) {
    int count = 0;
    for (int i = 0; i < RR_Data_Len - EMBEDDING_DIMENSION * DELAY; i++)
        for (int j = i + 1; j < RR_Data_Len - EMBEDDING_DIMENSION * DELAY; j++)
            if (euclidean_distance(phase_space[i], phase_space[j], EMBEDDING_DIMENSION) < radius)
                count++;
    return (double)count / ((RR_Data_Len - EMBEDDING_DIMENSION * DELAY) * ((RR_Data_Len - EMBEDDING_DIMENSION * DELAY) - 1) / 2);
}

double linear_regression(double* x, double* y, int len) {
    double x_sum = 0, y_sum = 0, xy_sum = 0, x2_sum = 0;
    int realLen = 0;
    for (int i = 0; i < len; i++) {
        if( !isnan(y[i]) && !isinf(y[i]) )
        {
            x_sum += x[i];
            y_sum += y[i];
            xy_sum += x[i] * y[i];
            x2_sum += x[i] * x[i];
            realLen++;
        }
    }
    double slope = 0.0;
    if( (realLen * x2_sum - x_sum * x_sum) != 0.0 )
        slope = (realLen * xy_sum - x_sum * y_sum) / (realLen * x2_sum - x_sum * x_sum);

    return slope;
}

double Cal_HRV_CD(double rr_intervals[], int RR_Data_Len) {
    // Load your R-R interval series data into rr_data
    // This should be populated with your actual data

    // Normalize data
    double max_val = rr_intervals[0];
    for (int i = 1; i < RR_Data_Len; i++)
        if (rr_intervals[i] > max_val)
            max_val = rr_intervals[i];
    for (int i = 0; i < RR_Data_Len; i++)
        rr_intervals[i] /= max_val;

    // Construct phase space
    for (int i = 0; i < RR_Data_Len - EMBEDDING_DIMENSION * DELAY; i++)
        for (int j = 0; j < EMBEDDING_DIMENSION; j++)
            phase_space[i][j] = rr_intervals[i + j * DELAY];

    // Calculate correlation sum for a range of radii
    int num_radii = (int)((RADIUS_END - RADIUS_START) / RADIUS_STEP) + 1;
    double log_radii[num_radii];
    double log_correlation_sums[num_radii];
    double tempCorrSum = 1.0;
    for (int i = 0; i < num_radii; i++) {
        double radius = RADIUS_START + i * RADIUS_STEP;
        log_radii[i] = log(radius);
        tempCorrSum = calculate_correlation_sum(radius, RR_Data_Len);
        log_correlation_sums[i] = log(tempCorrSum);
    }

    // Calculate correlation dimension
    double correlation_dimension = linear_regression(log_radii, log_correlation_sums, num_radii);

    //printf("Correlation Dimension: %f\n", correlation_dimension);

    return correlation_dimension;
}

double Cal_HRV_C1a(double *rr_intervals, int size) {
    double *rr_diff = (double *)malloc((size-1) * sizeof(double));
    if (rr_diff == NULL){
        printf("Error allocating memory!\n");
        return 0.0;
    }

    for(int i = 0; i < size-1; i++){
        rr_diff[i] = rr_intervals[i] - rr_intervals[i+1]; // for accelerations
    }

    int count = 0;
    for(int i = 0; i < size-1; i++){
        if(rr_diff[i] > C1A_THRESHOLD){
            count++;
        }
    }

    double C1a = ((double)count/(size-1)) * 100;

    free(rr_diff);

    return C1a;
}

// Comparator function for the qsort function
int compare(const void *a, const void *b) {
    const double* x = (double*) a;
    const double* y = (double*) b;

    if (*x > *y)
        return 1;
    else if (*x < *y)
        return -1;

    return 0;
}

double calculate_quartile(double* intervals, int size, int quartile) {
    int point = (quartile * size) / 4;

    if(point > 0)
    {
        if(size % 4 > 0) {
            return intervals[point];
        } else {
            return (double) (intervals[point - 1] + intervals[point]) / 2;
        }
    }
    else
        return intervals[0];
}

void calculate_histogram(double* intervals, int size, int* histogram, int max_bin, double bin_width) {
    for(int i=0; i<=max_bin; i++) histogram[i] = 0;

    for(int i=0; i<size; i++) {
        int bin = (int) (intervals[i] / bin_width);
        if(bin <= max_bin) {
            histogram[bin]++;
        }
    }
}

double shannon_entropy(int* histogram, int max_bin, int size) {
    double entropy = 0.0;

    for(int i=0; i<=max_bin; i++) {
        if(histogram[i] != 0) {
            double probability = (double) histogram[i] / size;
            entropy += probability * log2(probability);
        }
    }

    return -entropy;
}

double Cal_HRV_ShanEn(double rr_intervals[], int size, double * pMedianRR) {
    // Duplicate RR interval array for sorting

    double *rrArray_4sort = (double *)malloc(size * sizeof(double));
    if (rrArray_4sort == NULL){
        printf("Error allocating memory!\n");
        return 0.0;
    }

    for(int i = 0; i < size; i++){
        rrArray_4sort[i] = rr_intervals[i]; // for accelerations
    }



    // Sort the RR intervals
    qsort(rrArray_4sort, size, sizeof(double), compare);

    double Q1 = calculate_quartile(rrArray_4sort, size, 1);
    double Q3 = calculate_quartile(rrArray_4sort, size, 3);
    (*pMedianRR) = calculate_quartile(rrArray_4sort, size, 2);
    //(*pMedianRR) *= 1000.0;
    double IQR = Q3 - Q1;
    double bin_width = 2 * IQR * pow(size, -1.0/3.0);

    int max_bin = (int) (rrArray_4sort[size - 1] / bin_width);
    int* histogram = (int*) malloc((max_bin + 1) * sizeof(int));
    calculate_histogram(rrArray_4sort, size, histogram, max_bin, bin_width);

    double entropy = shannon_entropy(histogram, max_bin, size);

    //printf("Shannon entropy: %lf\n", entropy);

    free(histogram);
    free(rrArray_4sort);

    return entropy;
}


void BWRFrame()
{
    int fi;
    long avgSum0 = 0;
    long avgSum1 = 0;
    long avgSum2 = 0;
    double avgCntDouble = 0.0;
    double avgValue0 = 0.0;
    double avgValue1 = 0.0;
    double avgValue2 = 0.0;
    double BWRfilterSize = BWR_FILTER_SIZE;
    int BWRcalSize = ECG_FRAME_SIZE + BWR_FILTER_SIZE * 2 + BWR_S2_SIZE;

    unsigned short BWRbaseline0[ECG_FRAME_SIZE+BWR_FILTER_SIZE*2+BWR_S2_SIZE];  // 將BWRFrame平均算出的baseline值(Lead I)
    unsigned short BWRbaseline1[ECG_FRAME_SIZE+BWR_FILTER_SIZE*2+BWR_S2_SIZE];  // 將BWRFrame平均算出的baseline值(Lead II)
    unsigned short BWRbaseline2[ECG_FRAME_SIZE+BWR_FILTER_SIZE*2+BWR_S2_SIZE];  // 將BWRFrame平均算出的baseline值(Lead III)
    unsigned short BWRbaseS20[ECG_FRAME_SIZE+BWR_FILTER_SIZE*2+BWR_S2_SIZE];  // 將BWRFrame平均算出的baseline值(stage 2) (Lead I)
    unsigned short BWRbaseS21[ECG_FRAME_SIZE+BWR_FILTER_SIZE*2+BWR_S2_SIZE];  // 將BWRFrame平均算出的baseline值(stage 2) (Lead II)
    unsigned short BWRbaseS22[ECG_FRAME_SIZE+BWR_FILTER_SIZE*2+BWR_S2_SIZE];  // 將BWRFrame平均算出的baseline值(stage 2) (Lead III)

    float BWR2FIRraw0[ECG_FRAME_SIZE+BWR_FILTER_SIZE*2+BWR_S2_SIZE];  // BWR後要進行FIR的buffer (Lead I)
    float BWR2FIRraw1[ECG_FRAME_SIZE+BWR_FILTER_SIZE*2+BWR_S2_SIZE];  // BWR後要進行FIR的buffer (Lead II)
    float BWR2FIRraw2[ECG_FRAME_SIZE+BWR_FILTER_SIZE*2+BWR_S2_SIZE];  // BWR後要進行FIR的buffer (Lead III)
    float BWR2FIRfilt0[ECG_FRAME_SIZE+BWR_FILTER_SIZE*2+BWR_S2_SIZE];  // BWR後要進行FIR的buffer (Lead I)
    float BWR2FIRfilt1[ECG_FRAME_SIZE+BWR_FILTER_SIZE*2+BWR_S2_SIZE];  // BWR後要進行FIR的buffer (Lead II)
    float BWR2FIRfilt2[ECG_FRAME_SIZE+BWR_FILTER_SIZE*2+BWR_S2_SIZE];  // BWR後要進行FIR的buffer (Lead III)

    // 初始化BWRbaseS2
    for(fi = 0; fi < BWRcalSize; fi++)
    {
        BWRbaseS20[fi] = ECGADC_MIDDLE;
        BWRbaseS21[fi] = ECGADC_MIDDLE;
        BWRbaseS22[fi] = ECGADC_MIDDLE;
    }
    // 初始化BWR2FIRraw
    for(fi = 0; fi < BWRcalSize; fi++)
    {
        BWR2FIRraw0[fi] = ECGADC_MIDDLE;
        BWR2FIRraw1[fi] = ECGADC_MIDDLE;
        BWR2FIRraw2[fi] = ECGADC_MIDDLE;
    }

    // 進行BWR

    // 計算平均, 填入BWRbaseline
    for(fi = 0; fi < BWRcalSize; fi++)
    {
        avgSum0 += BWRFrame0[fi];
        avgSum1 += BWRFrame1[fi];
        avgSum2 += BWRFrame2[fi];
        avgCntDouble += 1.0;
        if( fi >= BWR_FILTER_SIZE )
        {	avgSum0 -= BWRFrame0[fi - BWR_FILTER_SIZE];
            avgSum1 -= BWRFrame1[fi - BWR_FILTER_SIZE];
            avgSum2 -= BWRFrame2[fi - BWR_FILTER_SIZE];

            avgValue0 = avgSum0 / BWRfilterSize;
            avgValue1 = avgSum1 / BWRfilterSize;
            avgValue2 = avgSum2 / BWRfilterSize;
        }
        else
        {
            avgValue0 = avgSum0 / avgCntDouble;
            avgValue1 = avgSum1 / avgCntDouble;
            avgValue2 = avgSum2 / avgCntDouble;
        }

        BWRbaseline0[fi] = avgValue0;
        BWRbaseline1[fi] = avgValue1;
        BWRbaseline2[fi] = avgValue2;
    }

    if(BWR_S2_SIZE == 0)
    {	// 不做第二次平均
        for(fi = 0; fi < BWRcalSize; fi++)
        {	BWRbaseS20[fi] = BWRbaseline0[fi];
            BWRbaseS21[fi] = BWRbaseline1[fi];
            BWRbaseS22[fi] = BWRbaseline2[fi];
        }
    }
    else
    {
        // 計算平均(stage 2), 填入BWRbaseS2
        avgSum0 = 0;
        avgSum1 = 0;
        avgSum2 = 0;
        avgCntDouble = 0.0;
        BWRfilterSize = BWR_S2_SIZE;
        for(fi = 0; fi < BWRcalSize; fi++)
        {
            avgSum0 += BWRbaseline0[fi];
            avgSum1 += BWRbaseline1[fi];
            avgSum2 += BWRbaseline2[fi];
            avgCntDouble += 1.0;
            if( fi >= BWR_S2_SIZE )
            {	avgSum0 -= BWRbaseline0[fi - BWR_S2_SIZE];
                avgSum1 -= BWRbaseline1[fi - BWR_S2_SIZE];
                avgSum2 -= BWRbaseline2[fi - BWR_S2_SIZE];

                avgValue0 = avgSum0 / BWRfilterSize;
                avgValue1 = avgSum1 / BWRfilterSize;
                avgValue2 = avgSum2 / BWRfilterSize;
            }
            else
            {
                avgValue0 = avgSum0 / avgCntDouble;
                avgValue1 = avgSum1 / avgCntDouble;
                avgValue2 = avgSum2 / avgCntDouble;
            }

            if( fi - BWR_S2_SIZE / 2 >= 0 )
            {	BWRbaseS20[fi - BWR_S2_SIZE / 2] = avgValue0;
                BWRbaseS21[fi - BWR_S2_SIZE / 2] = avgValue1;
                BWRbaseS22[fi - BWR_S2_SIZE / 2] = avgValue2;
            }
        }
    }

    // 把ECGFrame減去BWRbaseS2, 再放入BWR2FIRraw

    int BWRbufferSize;

    BWRbufferSize = ECG_FRAME_SIZE + BWR_FILTER_SIZE + BWR_FILTER_SIZE / 2 + BWR_S2_SIZE;

    for(fi = 0; fi < BWRbufferSize; fi++)
    {
        BWR2FIRraw0[fi] = (float)( BWRFrame0[fi] - BWRbaseS20[fi + BWR_FILTER_SIZE / 2] + ECGADC_MIDDLE );
        BWR2FIRraw1[fi] = (float)( BWRFrame1[fi] - BWRbaseS21[fi + BWR_FILTER_SIZE / 2] + ECGADC_MIDDLE );
        BWR2FIRraw2[fi] = (float)( BWRFrame2[fi] - BWRbaseS22[fi + BWR_FILTER_SIZE / 2] + ECGADC_MIDDLE );
    }

    // 進行FIR lowpass filter
    FIRFrame(BWR2FIRraw0, BWR2FIRfilt0);
    FIRFrame(BWR2FIRraw1, BWR2FIRfilt1);
    FIRFrame(BWR2FIRraw2, BWR2FIRfilt2);

    // 輸出結果到ECGFrame

    int shiftIdx = BWR_FILTER_SIZE + BWR_S2_SIZE / 2;
    BWRbufferSize = ECG_FRAME_SIZE;

    int tempLimitChk0;
    int tempLimitChk1;
    int tempLimitChk2;
    for(fi = 0; fi < BWRbufferSize; fi++)
    {
        tempLimitChk0 = BWR2FIRfilt0[fi + shiftIdx];
        tempLimitChk1 = BWR2FIRfilt1[fi + shiftIdx];
        tempLimitChk2 = BWR2FIRfilt2[fi + shiftIdx];

        // 檢查結果是否超過心電圖輸出檔案格式容許的上下限
        if(tempLimitChk0 < 0)
            tempLimitChk0 = 0;
        if(tempLimitChk0 >= ECGADC_MAX)
            tempLimitChk0 = ECGADC_MAX-1;
        if(tempLimitChk1 < 0)
            tempLimitChk1 = 0;
        if(tempLimitChk1 >= ECGADC_MAX)
            tempLimitChk1 = ECGADC_MAX-1;
        if(tempLimitChk2 < 0)
            tempLimitChk2 = 0;
        if(tempLimitChk2 >= ECGADC_MAX)
            tempLimitChk2 = ECGADC_MAX-1;

        ECGFrame0[fi] = tempLimitChk0;
        ECGFrame1[fi] = tempLimitChk1;
        ECGFrame2[fi] = tempLimitChk2;
    }
}

void DSFrameFindR()		// The Dual-Slope QRS Detection in a DSdownFrame[] frame
{
    DS_RPosList.clear();

    int fi, fi2;

    float SL_max = 0.0;
    float SL_min = 0.0;
    float SR_max = 0.0;
    float SR_min = 0.0;
    float tempSlope = 0.0;

    float leftRHigh_max = 0.0;
    float leftRHigh_min = 0.0;
    float rightRHigh_max = 0.0;
    float rightRHigh_min = 0.0;
    float tempRHigh = 0.0;

    int curDSFidx = DS_SLOPE_SIZE;	// The current index of the DSdownFrame[]
    // Calculate Sdiff_max
    for (fi = 0; fi < DS_FRAME_SIZE; fi++)
    {
        // Initialize SL_max, SL_min, SR_max, SR_min.
        tempSlope = ((float)(DSdownFrame[curDSFidx]) - (float)(DSdownFrame[curDSFidx - DS_SLOPE_MIN])) / (float)DS_SLOPE_MIN;
        SL_max = tempSlope;
        SL_min = tempSlope;
        tempSlope = ((float)(DSdownFrame[curDSFidx]) - (float)(DSdownFrame[curDSFidx + DS_SLOPE_MIN])) / (float)DS_SLOPE_MIN;
        SR_max = tempSlope;
        SR_min = tempSlope;

        // Initialize leftRHigh_max, leftRHigh_min, rightRHigh_max, rightRHigh_min.
        tempRHigh = (float)(DSdownFrame[curDSFidx]) - (float)(DSdownFrame[curDSFidx - DS_SLOPE_MIN]);
        leftRHigh_max = tempRHigh;
        leftRHigh_min = tempRHigh;
        tempRHigh = (float)(DSdownFrame[curDSFidx]) - (float)(DSdownFrame[curDSFidx + DS_SLOPE_MIN]);
        rightRHigh_max = tempRHigh;
        rightRHigh_min = tempRHigh;

        for (fi2 = DS_SLOPE_MIN; fi2 <= DS_SLOPE_SIZE; fi2++)
        {
            if (fi2 != 0)
                tempSlope = ((float)(DSdownFrame[curDSFidx]) - (float)(DSdownFrame[curDSFidx - fi2])) / (float)fi2;
            else
                tempSlope = 0;
            if (tempSlope > SL_max)
                SL_max = tempSlope;
            if (tempSlope < SL_min)
                SL_min = tempSlope;

            if (fi2 != 0)
                tempSlope = ((float)(DSdownFrame[curDSFidx]) - (float)(DSdownFrame[curDSFidx + fi2])) / (float)fi2;
            else
                tempSlope = 0;
            if (tempSlope > SR_max)
                SR_max = tempSlope;
            if (tempSlope < SR_min)
                SR_min = tempSlope;
        }

        for (fi2 = DS_SLOPE_MIN; fi2 <= DS_HIGH_SEARCH_SIZE; fi2++)
        {
            tempRHigh = (float)(DSdownFrame[curDSFidx]) - (float)(DSdownFrame[curDSFidx - fi2]);
            if (tempRHigh > leftRHigh_max)
                leftRHigh_max = tempRHigh;
            if (tempRHigh < leftRHigh_min)
                leftRHigh_min = tempRHigh;

            tempRHigh = (float)(DSdownFrame[curDSFidx]) - (float)(DSdownFrame[curDSFidx + fi2]);
            if (tempRHigh > rightRHigh_max)
                rightRHigh_max = tempRHigh;
            if (tempRHigh < rightRHigh_min)
                rightRHigh_min = tempRHigh;
        }

        float Sdiff_max = 0.0;
        if (SL_max + SR_max > (-1.0) * SL_min + (-1.0) * SR_min)
            Sdiff_max = SL_max + SR_max;
        else
            Sdiff_max = (-1.0) * SL_min + (-1.0) * SR_min;

        DS_Sdiff[fi] = Sdiff_max;

        // For Criterion 2 check

        float fS_LRminTest = 0.0;
        if (SL_max + SR_max > (-1.0) * SL_min + (-1.0) * SR_min)
        {
            if (SL_max > SR_max)
                fS_LRminTest = SR_max;
            else
                fS_LRminTest = SL_max;
        }
        else
        {
            if ((-1.0) * SL_min > (-1.0) * SR_min)
                fS_LRminTest = (-1.0) * SR_min;
            else
                fS_LRminTest = (-1.0) * SL_min;
        }

        DS_S_LRmin[fi] = fS_LRminTest;

        // For Criterion 3 check

        float leftRHighMax = 0.0;
        if (leftRHigh_max > (-1.0) * leftRHigh_min)
            leftRHighMax = leftRHigh_max;
        else
            leftRHighMax = (-1.0) * leftRHigh_min;

        float rightRHighMax = 0.0;
        if (rightRHigh_max > (-1.0) * rightRHigh_min)
            rightRHighMax = rightRHigh_max;
        else
            rightRHighMax = (-1.0) * rightRHigh_min;

        float twoDirRHighMax = 0.0;
        if (rightRHighMax > leftRHighMax)
            twoDirRHighMax = rightRHighMax;
        else
            twoDirRHighMax = leftRHighMax;

        DS_High[fi] = twoDirRHighMax;

        //if(DSdownFrame[curDSFidx] - ECGADC_MIDDLE > 0)
        //	DS_High[fi] = DSdownFrame[curDSFidx] - ECGADC_MIDDLE;
        //else
        //	DS_High[fi] = (-1.0) * (DSdownFrame[curDSFidx] - ECGADC_MIDDLE);

        curDSFidx++;
    }

    int SDDCenterSpan = S_DIFFDIFF_HSPAN / 2;		// The span of the central part of the difference calculation of Sdiff_max

    float SdiffDiffBuf[DS_FRAME_SIZE + S_DIFFDIFF_HSPAN * 2 - 1];	// The buffer to calculate the difference of Sdiff_max including the span of the differential filter
    float S_LRminBuf[DS_FRAME_SIZE + S_DIFFDIFF_HSPAN * 2 - 1];	// The buffer of DS_S_LRmin[] including the starting SdiffDiff pad
    float DS_HighBuf[DS_FRAME_SIZE + S_DIFFDIFF_HSPAN * 2 - 1];	// The buffer of DS_High[] including the starting SdiffDiff pad
    float SdiffDiffRst[DS_FRAME_SIZE];	// The results of the calculation of the difference of Sdiff_max including the span of the differential filter
    float SdiffDiffRstBig[DS_FRAME_SIZE + 2];	// The buffer to detect the zero crossing of SdiffDiffRst[] including the span of the detection filter
    float tempSdiffDiff = 0.0;
    float weightCnt = 0.0;		// For counting the total weight of the difference calculation of Sdiff_max
    float SDDcoefSide = 1.0;	// The side coefficient of the difference calculation of Sdiff_max
    float SDDcoefCenter = 2.0;	// The central coefficient of the difference calculation of Sdiff_max

    // Initialize SdiffDiffRst[]
    for (fi = 0; fi < DS_FRAME_SIZE; fi++)
        SdiffDiffRst[fi] = 0.0;

    if (!bDS_Sdiff1stFrame)
    {
        // Not the first frame of Sdiff_max. Fill SdiffDiffBuf[].

        for (fi = 0; fi < S_DIFFDIFF_HSPAN * 2 - 1; fi++)
        {
            SdiffDiffBuf[fi] = DS_SdiffPre[DS_FRAME_SIZE - S_DIFFDIFF_HSPAN * 2 + 1 + fi];

            // For Criterion 2
            S_LRminBuf[fi] = DS_S_LRminPre[DS_FRAME_SIZE - S_DIFFDIFF_HSPAN * 2 + 1 + fi];

            // For Criterion 3
            DS_HighBuf[fi] = DS_HighPre[DS_FRAME_SIZE - S_DIFFDIFF_HSPAN * 2 + 1 + fi];

        }
        for (fi = 0; fi < DS_FRAME_SIZE; fi++)
        {
            SdiffDiffBuf[fi + S_DIFFDIFF_HSPAN * 2 - 1] = DS_Sdiff[fi];

            // For Criterion 2
            S_LRminBuf[fi + S_DIFFDIFF_HSPAN * 2 - 1] = DS_S_LRmin[fi];

            // For Criterion 3
            DS_HighBuf[fi + S_DIFFDIFF_HSPAN * 2 - 1] = DS_High[fi];
        }

        for (fi = 0; fi < DS_FRAME_SIZE; fi++)
        {
            // Calculate the difference of Sdiff_max

            weightCnt = 0.0;
            tempSdiffDiff = 0.0;
            for (fi2 = 0; fi2 < S_DIFFDIFF_HSPAN - SDDCenterSpan; fi2++)
            {
                tempSdiffDiff -= SdiffDiffBuf[fi + fi2] * SDDcoefSide;
                weightCnt += SDDcoefSide;
            }
            for (fi2 = 0; fi2 < SDDCenterSpan; fi2++)
            {
                tempSdiffDiff -= SdiffDiffBuf[fi + fi2 + (S_DIFFDIFF_HSPAN - SDDCenterSpan)] * SDDcoefCenter;
                weightCnt += SDDcoefCenter;
            }
            for (fi2 = 0; fi2 < SDDCenterSpan; fi2++)
            {
                tempSdiffDiff += SdiffDiffBuf[fi + fi2 + S_DIFFDIFF_HSPAN] * SDDcoefCenter;
            }
            for (fi2 = 0; fi2 < S_DIFFDIFF_HSPAN - SDDCenterSpan; fi2++)
            {
                tempSdiffDiff += SdiffDiffBuf[fi + fi2 + S_DIFFDIFF_HSPAN + SDDCenterSpan] * SDDcoefSide;
            }
            if (weightCnt != 0)
                tempSdiffDiff /= weightCnt;
            else
                tempSdiffDiff = 0.0;

            SdiffDiffRst[fi] = tempSdiffDiff;
        }
    }
    else
    {
        // The first frame of Sdiff_max. Fill SdiffDiffBuf[].

        for (fi = 0; fi < DS_FRAME_SIZE; fi++)
        {
            SdiffDiffBuf[fi] = DS_Sdiff[fi];

            // For Criterion 2
            S_LRminBuf[fi] = DS_S_LRmin[fi];

            // For Criterion 3
            DS_HighBuf[fi] = DS_High[fi];
        }

        for (fi = 0; fi < DS_FRAME_SIZE - (S_DIFFDIFF_HSPAN * 2 - 1); fi++)
        {
            // Calculate the difference of Sdiff_max

            weightCnt = 0.0;
            tempSdiffDiff = 0.0;
            for (fi2 = 0; fi2 < S_DIFFDIFF_HSPAN - SDDCenterSpan; fi2++)
            {
                tempSdiffDiff -= SdiffDiffBuf[fi + fi2] * SDDcoefSide;
                weightCnt += SDDcoefSide;
            }
            for (fi2 = 0; fi2 < SDDCenterSpan; fi2++)
            {
                tempSdiffDiff -= SdiffDiffBuf[fi + fi2 + (S_DIFFDIFF_HSPAN - SDDCenterSpan)] * SDDcoefCenter;
                weightCnt += SDDcoefCenter;
            }
            for (fi2 = 0; fi2 < SDDCenterSpan; fi2++)
            {
                tempSdiffDiff += SdiffDiffBuf[fi + fi2 + S_DIFFDIFF_HSPAN] * SDDcoefCenter;
            }
            for (fi2 = 0; fi2 < S_DIFFDIFF_HSPAN - SDDCenterSpan; fi2++)
            {
                tempSdiffDiff += SdiffDiffBuf[fi + fi2 + S_DIFFDIFF_HSPAN + SDDCenterSpan] * SDDcoefSide;
            }
            if (weightCnt != 0)
                tempSdiffDiff /= weightCnt;
            else
                tempSdiffDiff = 0.0;

            if(fi == 0)
                for (fi2 = 0; fi2 < S_DIFFDIFF_HSPAN * 2; fi2++)
                    SdiffDiffRst[fi2] = tempSdiffDiff;
            else
                SdiffDiffRst[fi + S_DIFFDIFF_HSPAN * 2 - 1] = tempSdiffDiff;
        }
    }

    //if (!bDS_Sdiff1stFrame)
    {
        if (!bDS_SdiffDiff1stF)
        {
            // Fill SdiffDiffRstBig[]

            for (fi = 0; fi < 2; fi++)
            {
                SdiffDiffRstBig[fi] = SdiffDiffRstLast[DS_FRAME_SIZE - 2 + fi];
            }
            for (fi = 0; fi < DS_FRAME_SIZE; fi++)
            {
                SdiffDiffRstBig[fi + 2] = SdiffDiffRst[fi];
            }

            // Detect the local max of Sdiff_max (i.e. the zero crossing of the SdiffDiffRst[])
            unsigned char SDPlastTurn = 0;	// Avoid double counting a Sdiff_max peak
            for (fi = 1; fi < DS_FRAME_SIZE + 1; fi++)
            {
                lastRdistCnt++;

                if (SdiffDiffRstBig[fi - 1] > 0 && SdiffDiffRstBig[fi + 1] < 0)
                {
                    if (SDPlastTurn == 0)  // Avoid double counting a Sdiff_max peak
                    {
                        // This is the zero-crossing (peak) point we want

                        // Calculate the min threshold value of Sdiff_max
                        float fDSdownSampR = DS_DOWNSAMPLE_R;
                        float fFreqSamp = 1000.0 / fDSdownSampR;
                        float fSdiffAve = 0.0;
                        float fTempSdiffAve = 0.0;
                        float fHighAve = 0.0;
                        float fTempHighAve = 0.0;
                        float fS_LRminMin = DS_S_LR_MIN_MIN / fFreqSamp;		// The min of the samller one of the left and right slopes of R point

                        if (SdiffDiffBuf[fi - 1 - 1 + S_DIFFDIFF_HSPAN] > fCurThetaDiff)
                        {
                            // This is the R point candidate. Than we check Criterion 1

                            SdiffAveIdx++;
                            if (SdiffAveIdx > DS_SDIFF_AVE_N)
                            {
                                SdiffAveIdx = DS_SDIFF_AVE_N + 1;	// Avoid increasing infinitely

                                // Calculate the average of Sdiff_max
                                fSdiffAve = 0;
                                for (fi2 = 0; fi2 < DS_SDIFF_AVE_N; fi2++)
                                {
                                    fTempSdiffAve = SdiffAveBuf[fi2] / DS_SDIFF_AVE_N;
                                    fSdiffAve += fTempSdiffAve;
                                }

                                // Calculate the average of ECG peaks
                                fHighAve = 0;
                                for (fi2 = 0; fi2 < DS_SDIFF_AVE_N; fi2++)
                                {
                                    fTempHighAve = HighAveBuf[fi2] / DS_SDIFF_AVE_N;
                                    fHighAve += fTempHighAve;
                                }

                                float fSAveComp0 = DS_S_AVE_COMP_0 / fFreqSamp;
                                float fSAveComp1 = DS_S_AVE_COMP_1 / fFreqSamp;
                                if (fSdiffAve < fSAveComp0)
                                {
                                    fCurThetaDiff = DS_THETA_DIFF_0 / fFreqSamp;
                                }
                                else if (fSdiffAve >= fSAveComp0 && fSdiffAve < fSAveComp1)
                                {
                                    fCurThetaDiff = DS_THETA_DIFF_1 / fFreqSamp;
                                }
                                else
                                {
                                    fCurThetaDiff = DS_THETA_DIFF_2 / fFreqSamp;
                                }

                                // Check Criterion 2

                                if (S_LRminBuf[fi - 1 - 1 + S_DIFFDIFF_HSPAN] > fS_LRminMin)
                                {
                                    // Criterion 2 passed. Check Criterion 3
                                    if (DS_HighBuf[fi - 1 - 1 + S_DIFFDIFF_HSPAN] > fHighAve * DS_HIGH_AVE_RAT)
                                        //if (true)
                                    {
                                        // Criterion 3 passed

                                        if (lastRdistCnt > DS_LAST_R_DIST_MIN)
                                        {
                                            DS_RPosList.push_back(fi - 1);

                                            // Update the Sdiff_max average buffer
                                            for (fi2 = DS_SDIFF_AVE_N - 1; fi2 > 0; fi2--)
                                                SdiffAveBuf[fi2] = SdiffAveBuf[fi2 - 1];
                                            SdiffAveBuf[0] = SdiffDiffBuf[fi - 1 - 1 + S_DIFFDIFF_HSPAN];

                                            // Calculate the average of Sdiff_max
                                            fSdiffAve = 0;
                                            for (fi2 = 0; fi2 < DS_SDIFF_AVE_N; fi2++)
                                            {
                                                fTempSdiffAve = SdiffAveBuf[fi2] / DS_SDIFF_AVE_N;
                                                fSdiffAve += fTempSdiffAve;
                                            }

                                            // Update the ECG peak average buffer
                                            for (fi2 = DS_SDIFF_AVE_N - 1; fi2 > 0; fi2--)
                                                HighAveBuf[fi2] = HighAveBuf[fi2 - 1];
                                            HighAveBuf[0] = DS_HighBuf[fi - 1 - 1 + S_DIFFDIFF_HSPAN];

                                            // Calculate the average of ECG peaks
                                            fHighAve = 0;
                                            for (fi2 = 0; fi2 < DS_SDIFF_AVE_N; fi2++)
                                            {
                                                fTempHighAve = HighAveBuf[fi2] / DS_SDIFF_AVE_N;
                                                fHighAve += fTempHighAve;
                                            }

                                            float fSAveComp0 = DS_S_AVE_COMP_0 / fFreqSamp;
                                            float fSAveComp1 = DS_S_AVE_COMP_1 / fFreqSamp;
                                            if (fSdiffAve < fSAveComp0)
                                            {
                                                fCurThetaDiff = DS_THETA_DIFF_0 / fFreqSamp;
                                            }
                                            else if (fSdiffAve >= fSAveComp0 && fSdiffAve < fSAveComp1)
                                            {
                                                fCurThetaDiff = DS_THETA_DIFF_1 / fFreqSamp;
                                            }
                                            else
                                            {
                                                fCurThetaDiff = DS_THETA_DIFF_2 / fFreqSamp;
                                            }

                                            lastRdistCnt = 0;
                                        }
                                    }
                                }

                            }
                            else
                            {
                                if (lastRdistCnt > DS_LAST_R_DIST_MIN)
                                {
                                    // Update the Sdiff_max average buffer
                                    for (fi2 = DS_SDIFF_AVE_N - 1; fi2 > 0; fi2--)
                                        SdiffAveBuf[fi2] = SdiffAveBuf[fi2 - 1];
                                    SdiffAveBuf[0] = SdiffDiffBuf[fi - 1 - 1 + S_DIFFDIFF_HSPAN];

                                    // Calculate the average of Sdiff_max
                                    fSdiffAve = 0;
                                    for (fi2 = 0; fi2 < DS_SDIFF_AVE_N; fi2++)
                                    {
                                        fTempSdiffAve = SdiffAveBuf[fi2] / DS_SDIFF_AVE_N;
                                        fSdiffAve += fTempSdiffAve;
                                    }

                                    // Update the ECG peak average buffer
                                    for (fi2 = DS_SDIFF_AVE_N - 1; fi2 > 0; fi2--)
                                        HighAveBuf[fi2] = HighAveBuf[fi2 - 1];
                                    HighAveBuf[0] = DS_HighBuf[fi - 1 - 1 + S_DIFFDIFF_HSPAN];

                                    // Calculate the average of ECG peaks
                                    fHighAve = 0;
                                    for (fi2 = 0; fi2 < DS_SDIFF_AVE_N; fi2++)
                                    {
                                        fTempHighAve = HighAveBuf[fi2] / DS_SDIFF_AVE_N;
                                        fHighAve += fTempHighAve;
                                    }

                                    lastRdistCnt = 0;
                                }
                            }
                        }

                        SDPlastTurn = 1;
                        continue;
                    }
                }
                SDPlastTurn = 0;
            }
        }

        bDS_SdiffDiff1stF = false;
    }

    bDS_Sdiff1stFrame = false;

    // Fill DS_SdiffPre[]
    for (fi = 0; fi < DS_FRAME_SIZE; fi++)
    {
        DS_SdiffPre[fi] = DS_Sdiff[fi];
    }

    // Fill DS_S_LRminPre[]
    for (fi = 0; fi < DS_FRAME_SIZE; fi++)
    {
        DS_S_LRminPre[fi] = DS_S_LRmin[fi];
    }

    // Fill DS_HighPre[]
    for (fi = 0; fi < DS_FRAME_SIZE; fi++)
    {
        DS_HighPre[fi] = DS_High[fi];
    }

    // Fill SdiffDiffRstLast[]
    for (fi = 0; fi < DS_FRAME_SIZE; fi++)
    {
        SdiffDiffRstLast[fi] = SdiffDiffRst[fi];
    }
}

void decryFileName( char* lpszPlain, char* lpszEncry)
{
    int fi;
    unsigned char cPlain;
    unsigned char key[13];         // 加密的key，ASCII值必須小於64
    unsigned char cDecry;
    unsigned char cEncry;
    unsigned char strUnscat[21];

    // 還原字串中字元的次序，是解密的一部分
    lpszPlain[0] = lpszEncry[0];
    lpszPlain[1] = lpszEncry[1];
    lpszPlain[2] = lpszEncry[2];
    lpszPlain[3] = lpszEncry[3];
    lpszPlain[4] = lpszEncry[4];
    lpszPlain[5] = lpszEncry[5];
    lpszPlain[6] = lpszEncry[6];
    lpszPlain[7] = lpszEncry[7];
    lpszPlain[8] = lpszEncry[8];
    lpszPlain[9] = lpszEncry[19];
    lpszPlain[10] = lpszEncry[17];
    lpszPlain[11] = lpszEncry[11];
    lpszPlain[12] = lpszEncry[12];
    lpszPlain[13] = lpszEncry[18];
    lpszPlain[14] = lpszEncry[14];
    lpszPlain[15] = lpszEncry[15];
    lpszPlain[16] = lpszEncry[16];
    lpszPlain[17] = lpszEncry[10];
    lpszPlain[18] = lpszEncry[13];
    lpszPlain[19] = lpszEncry[9];
    lpszPlain[20] = lpszEncry[20];

    lpszPlain[21] = '\0';
}

unsigned char fNChar2Enc( unsigned char cFName )
{
    unsigned char cRet;
    if(cFName == '_')
        cRet = 62;
    else if(cFName == '-')
        cRet = 61;
    else if(cFName >= 'a' && cFName <= 'z')
        cRet = cFName - 'a' + 36;
    else if(cFName >= 'A' && cFName <= 'Z')
        cRet = cFName - 'A' + 10;
    else
        cRet = cFName - '0';

    return cRet;
}

void notchFrame()
{
    float FIRProcessed[ECG_FRAME_SIZE + NOTCH_PREPAD_SIZE];
    float FIRProcessed1[ECG_FRAME_SIZE + NOTCH_PREPAD_SIZE];

    int fi;
    int fi2;
    int tempDelayHead;
    float sum, sum_1;
    float sum2, sum2_1;

    float tempCoeff = 0;
    int tempDestStart = 0;
    int tempSourceStart = 0;
    float avg = 0;
    float avg1 = 0;
    float avgN = ECG_FRAME_SIZE + NOTCH_PREPAD_SIZE;
    int iAvgN = (int)avgN;
    int FIRMax = iAvgN;
    float tempF;
    bool isEnd = false;

    for(fi = 0; fi < iAvgN; fi++)
    {
        sum = (float)(NchFrame0[fi]);
        sum_1 = (float)(NchFrame1[fi]);

        tempDelayHead = notchDelayHead;
        fi2 = 0;
        while(fi2 < NOTCH_SPAN)
        {	sum += notchDcoeff[fi2] * ECGdelayBuffer[tempDelayHead];
            sum_1 += notchDcoeff[fi2] * ECGdelayBuffer1[tempDelayHead];
            tempDelayHead++;

            if(tempDelayHead >= MaxBufferSize*RawRatio)
                tempDelayHead = 0;
            fi2 = fi2+1;
        }

        sum2 = notchNcoeff[0] * sum;
        sum2_1 = notchNcoeff[0] * sum_1;

        tempDelayHead = notchDelayHead;

        fi2 = 0;
        while(fi2 < NOTCH_SPAN)
        {	sum2 += notchNcoeff[fi2+1] * ECGdelayBuffer[tempDelayHead];
            sum2_1 += notchNcoeff[fi2+1] * ECGdelayBuffer1[tempDelayHead];
            tempDelayHead++;

            if(tempDelayHead >= MaxBufferSize*RawRatio)
                tempDelayHead = 0;
            fi2 = fi2+1;
        }

        FIRProcessed[fi] = sum2;
        FIRProcessed1[fi] = sum2_1;

        notchDelayHead--;
        if(notchDelayHead < 0)
            notchDelayHead = MaxBufferSize*RawRatio - 1;
        if(notchDelayHead == notchDelayTail)
        {	notchDelayTail--;
            if(notchDelayTail < 0)
                notchDelayTail = MaxBufferSize*RawRatio - 1;
        }
        ECGdelayBuffer[notchDelayHead] = sum;
        ECGdelayBuffer1[notchDelayHead] = sum_1;
    }

    //for(fi = 0; fi < FIRMax; fi++)
    //{		avg += FIRProcessed[fi] / (float)FIRMax;
    //		avg1 += FIRProcessed1[fi] / (float)FIRMax;
    //}

    float tempAvged = 0.0;
    float tempAvged1 = 0.0;

    for(fi = 0; fi < iAvgN; fi++)
    {	//tempAvged = FIRProcessed[fi] - avg + ECGADC_MAX_F;
        //tempAvged1 = FIRProcessed1[fi] - avg + ECGADC_MAX_F;
        tempAvged = FIRProcessed[fi];
        tempAvged1 = FIRProcessed1[fi];

        if(tempAvged < 0)
            tempAvged = 0;
        if(tempAvged1 < 0)
            tempAvged1 = 0;

        if(tempAvged >= ECGADC_MAX)
            tempAvged = ECGADC_MAX-1;
        if(tempAvged1 >= ECGADC_MAX)
            tempAvged1 = ECGADC_MAX-1;

        NchFrame0[fi] = (unsigned short)tempAvged;
        NchFrame1[fi] = (unsigned short)tempAvged1;
    }
}

void nonNotchFrame()
{
    float FIRProcessed[ECG_FRAME_SIZE + NOTCH_PREPAD_SIZE];
    float FIRProcessed1[ECG_FRAME_SIZE + NOTCH_PREPAD_SIZE];

    int fi;
    float sum, sum_1;

    float avg = 0;
    float avg1 = 0;
    float avgN = ECG_FRAME_SIZE + NOTCH_PREPAD_SIZE;
    int iAvgN = (int)avgN;
    int FIRMax = iAvgN;
    bool isEnd = false;
    int endIdx = iAvgN;

    for(fi = 0; fi < iAvgN; fi++)
    {
        sum = (float)(NchFrame0[fi]);
        sum_1 = (float)(NchFrame1[fi]);
        FIRProcessed[fi] = sum;
        FIRProcessed1[fi] = sum_1;
    }

    //for(fi = 0; fi < FIRMax; fi++)
    //{	avg += FIRProcessed[fi] / (float)FIRMax;
    //	avg1 += FIRProcessed1[fi] / (float)FIRMax;
    //}

    float tempAvged = 0.0;
    float tempAvged1 = 0.0;
    for(fi = 0; fi < iAvgN; fi++)
    {	//tempAvged = FIRProcessed[fi] - avg + ECGADC_MAX_F;
        //tempAvged1 = FIRProcessed1[fi] - avg + ECGADC_MAX_F;
        tempAvged = FIRProcessed[fi];
        tempAvged1 = FIRProcessed1[fi];

        if(tempAvged < 0)
            tempAvged = 0;
        if(tempAvged1 < 0)
            tempAvged1 = 0;

        NchFrame0[fi] = (unsigned short)tempAvged;
        NchFrame1[fi] = (unsigned short)tempAvged1;
    }
}

void PowerFFT()
{
    int Points = POWERFFT_ANALY_SIZE;
    int fi, fi2, l;
    double ECGori[POWERFFT_ANALY_SIZE];  // This is where the original samples will be stored
    unsigned short tempECGValueUS;
    float POWERFFT_tempF;
    MyComplex WExp[POWERFFT_SIZE_LOG+1][POWERFFT_ANALY_SIZE];  // Store the precomputed complex exponentials for each stage
    int aBitRev[POWERFFT_ANALY_SIZE];  // Bit-reversed mapping for ECGori[]
    int endIdx = ECG_ANALY_SIZE;

    // Set the start point
    int ECGAnalyStart = 0;
    int ECGReadTempHead = ECGAnalyStart;
    int debugInt = ECGReadTempHead;

    // Prepare data points for FFT
    POWERFFT_tempF = ECGAnaData2[ECGReadTempHead];
    ECGori[0] = (double)POWERFFT_tempF;
    incFilteredIndex(&ECGReadTempHead);
    for(fi = 1; fi < POWERFFT_ANALY_SIZE; fi++) {
        for(fi2 = 0; fi2 < POWERFFT_SAMPLE_STEP; fi2++)
        {	POWERFFT_tempF = ECGAnaData2[ECGReadTempHead];
            incFilteredIndex(&ECGReadTempHead);
        }
        ECGori[fi] = (double)POWERFFT_tempF;
    }

    // Precompute complex exponentials for each stage
    int _2_l = 2;
    for (l = 1; l <= POWERFFT_SIZE_LOG; l++)
    {
        for (fi = 0; fi < Points; fi++ )
        {
            double re =  cos(2. * PI * fi / _2_l);
            double im = (-1.0) * sin(2. * PI * fi / _2_l);
            WExp[l][fi] = MyComplex( re, im );
        }
        _2_l *= 2;
    }

    // prepare bit-reversed mapping
    int rev = 0;
    int halfPoints = Points/2;
    for (fi = 0; fi < Points - 1; fi++)
    {
        aBitRev[fi] = rev;
        int mask = halfPoints;
        // add 1 backwards
        while (rev >= mask)
        {
            rev -= mask; // turn off this bit
            mask >>= 1;
        }
        rev += mask;
    }
    aBitRev[Points-1] = Points-1;

    // Fill the FFT buffer with samples from ECGori[] in bit-reversed order
    for (fi = 0; fi < Points; fi++)
        PowerFFTRlt[aBitRev[fi]] = ECGori[fi];

    ///////////// FFT calculation /////////////////////////////////////////
    //
    //               0   1   2   3   4   5   6   7
    //  level   1
    //  step    1                                     0
    //  increm  2                                   W
    //  j = 0        <--->   <--->   <--->   <--->   1
    //  level   2
    //  step    2
    //  increm  4                                     0
    //  j = 0        <------->       <------->      W      1
    //  j = 1            <------->       <------->   2   W
    //  level   3                                         2
    //  step    4
    //  increm  8                                     0
    //  j = 0        <--------------->              W      1
    //  j = 1            <--------------->           3   W      2
    //  j = 2                <--------------->            3   W      3
    //  j = 3                    <--------------->             3   W
    //                                                              3
    //

    // step = 2 ^ (level-1)
    // increm = 2 ^ level;
    int step = 1;
    int level;
    for (level = 1; level <= POWERFFT_SIZE_LOG; level++)
    {
        int increm = step * 2;
        for (int j = 0; j < step; j++)
        {
            // U = exp ( - 2 PI j / 2 ^ level )
            MyComplex U = WExp[level][j];
            for (int i = j; i < Points; i += increm)
            {
                // in-place butterfly
                // Xnew[i]      = X[i] + U * X[i+step]
                // Xnew[i+step] = X[i] - U * X[i+step]
                MyComplex T = U;
                T *= PowerFFTRlt[i+step];
                PowerFFTRlt[i+step] = PowerFFTRlt[i];
                PowerFFTRlt[i+step] -= T;
                PowerFFTRlt[i] += T;
            }
        }
        step *= 2;
    }
}

void CalPNoiseFreq()
{
    // 計算Power line noise頻率
    int fi;
    double freqStep = ( 1000.0 / (double)POWERFFT_SAMPLE_STEP / 2.0 ) / ( (double)POWERFFT_ANALY_SIZE / 2.0 );
    int startShift = (int)(46.5 / freqStep);
    int FFTEnd = POWERFFT_ANALY_SIZE/2;
    double tempD;
    double powerFFTMax;
    int noiseIdx;
    float curNoiseHz;

    noiseIdx = POWERFFT_ANALY_SIZE - startShift;
    powerFFTMax = std::abs(PowerFFTRlt[POWERFFT_ANALY_SIZE - startShift]);
    for(fi = POWERFFT_ANALY_SIZE - 1 - startShift; fi >= FFTEnd; fi--)
    {	tempD = std::abs(PowerFFTRlt[fi]);
        if(powerFFTMax < tempD)
        {	powerFFTMax = tempD;
            noiseIdx = fi;
        }
    }

    if(powerFFTMax >= 200.0)
    {	bPNoiseGot = true;
        powerNoiseHz = (float)( freqStep * (POWERFFT_ANALY_SIZE - 1 - noiseIdx + 1) );
    }
}

void CalNotchCoeff()
{
    float beta;
    float theta;
    float ctheta;

    beta  = powerNoiseHz * PI / 10000.0;
    theta = powerNoiseHz * 2.0 * PI / 1000.0;
    ctheta = cos(theta);

    notchNcoeff[0] = 1.0 - beta;
    notchNcoeff[1] = -2.0 * (1.0 - beta) * ctheta;
    notchNcoeff[2] = notchNcoeff[0];

    notchDcoeff[0] = -1.0 * notchNcoeff[1];
    notchDcoeff[1] = -1.0 * (1.0 - 2.0 * beta);
}

void incFilteredIndex(int* pFilteredIndex)
{
    (*pFilteredIndex)++;
    if((*pFilteredIndex) == ECG_ANALY_SIZE)
        (*pFilteredIndex) = 0;
}

void decFilteredIndex(int* pFilteredIndex)
{
    (*pFilteredIndex)--;
    if((*pFilteredIndex) == -1)
        (*pFilteredIndex) = ECG_ANALY_SIZE-1;
}

void FIRFrame(float * ECGRaw, float * ECGFilt)
{
    float FIRProcessed0[ECG_FRAME_SIZE+BWR_FILTER_SIZE*2+BWR_S2_SIZE];

    unsigned long fi, fi2;
    float convSum0;
    float tempCoeff = 0;
    int tempDestStart = 0;
    int tempSourceStart = 0;
    int FIROffset = FIR_RECORD_STEP * ( FIR_COEFF_N - 1 );
    float avgN = ECG_FRAME_SIZE + BWR_FILTER_SIZE * 2 + BWR_S2_SIZE - FIROffset;
    int iAvgN = (int)avgN;
    int FIRMax = iAvgN;
    int endIdx = iAvgN;

    for(fi = 0; fi < iAvgN; fi++)
    {
        convSum0 = 0;

        tempSourceStart = fi;
        for(fi2 = 0; fi2 < FIR_COEFF_N; fi2++)
        {	tempCoeff = FIRCoeff[fi2];

            convSum0 += tempCoeff * ECGRaw[tempSourceStart];

            tempSourceStart += FIR_RECORD_STEP;
        }

        FIRProcessed0[fi] = convSum0;
    }

    float tempAvged0;

    int ECGFiltSt = FIROffset / 2;

    for(fi = 0; fi < iAvgN; fi++)
    {	tempAvged0 = FIRProcessed0[fi];

        if(tempAvged0 < 0)
            tempAvged0 = 0;

        if(tempAvged0 >= ECGADC_MAX)
            tempAvged0 = ECGADC_MAX-1;

        ECGFilt[fi + ECGFiltSt] = tempAvged0;
    }
}


CQRSDet::CQRSDet(void)
{
    m_RD2_DDBuffer = new int[DER_DELAY];
    m_RD2_First1s = 1;
}

CQRSDet::~CQRSDet(void)
{
    delete [] m_RD2_DDBuffer;
}

int CQRSDet::QRSInit()
{
    m_RD2_y1 = 0;
    m_RD2_y2 = 0;
    m_RD2_lpptr = 0;
    m_RD2_y = 0;
    m_RD2_hpptr = 0;
    m_RD2_derI1 = 0;
    m_RD2_derI2 = 0;
    m_RD2_mwisum = 0;
    m_RD2_mwiptr = 0;
    m_RD2_TH = .3125;
    m_RD2_Dly = 0;
    m_RD2_MEMMOVELEN = 7*sizeof(int);
    m_RD2_qpkcnt = 0;
    m_RD2_rsetCount = 0;
    m_RD2_sbpeak = 0;
    m_RD2_sbcount = MS1500;
    m_RD2_pk_max = 0;
    m_RD2_pk_timeSinceMax = 0;

    // Initialize filters.
    QRSDet(0,1);

    return 0;
}

int CQRSDet::QRSFilter(int datum,int init)
{
    int fdatum ;

    if(init)
    {
        RD2hpfilt( 0, 1 ) ;		// Initialize filters.
        RD2lpfilt( 0, 1 ) ;
        RD2mvwint( 0, 1 ) ;
        RD2deriv1( 0, 1 ) ;
        RD2deriv2( 0, 1 ) ;
    }

    fdatum = RD2lpfilt( datum, 0 ) ;		// Low pass filter data.
    fdatum = RD2hpfilt( fdatum, 0 ) ;	// High pass filter data.
    fdatum = RD2deriv2( fdatum, 0 ) ;	// Take the derivative.
    fdatum = abs(fdatum) ;				// Take the absolute value.
    fdatum = RD2mvwint( fdatum, 0 ) ;	// Average over an 80 ms window .
    return(fdatum) ;
}

int CQRSDet::RD2lpfilt( int datum ,int init)
{
    long y0 ;
    int output, halfPtr ;
    if(init)
    {
        for(m_RD2_lpptr = 0; m_RD2_lpptr < LPBUFFER_LGTH; ++m_RD2_lpptr)
            m_RD2_lpdata[m_RD2_lpptr] = 0 ;
        m_RD2_y1 = m_RD2_y2 = 0 ;
        m_RD2_lpptr = 0 ;
    }
    halfPtr = m_RD2_lpptr-(LPBUFFER_LGTH/2) ;	// Use halfPtr to index
    if(halfPtr < 0)							// to x[n-6].
        halfPtr += LPBUFFER_LGTH ;
    y0 = (m_RD2_y1 << 1) - m_RD2_y2 + datum - (m_RD2_lpdata[halfPtr] << 1) + m_RD2_lpdata[m_RD2_lpptr] ;
    m_RD2_y2 = m_RD2_y1;
    m_RD2_y1 = y0;
    output = y0 / ((LPBUFFER_LGTH*LPBUFFER_LGTH)/4);
    m_RD2_lpdata[m_RD2_lpptr] = datum ;			// Stick most recent sample into
    if(++m_RD2_lpptr == LPBUFFER_LGTH)	// the circular buffer and update
        m_RD2_lpptr = 0 ;					// the buffer pointer.
    return(output) ;
}

int CQRSDet::RD2hpfilt( int datum, int init )
{
    int z, halfPtr ;

    if(init)
    {
        for( m_RD2_hpptr = 0; m_RD2_hpptr < HPBUFFER_LGTH; ++m_RD2_hpptr )
            m_RD2_hpdata[m_RD2_hpptr] = 0;
        m_RD2_hpptr = 0 ;
        m_RD2_y = 0 ;
    }

    m_RD2_y += datum - m_RD2_hpdata[m_RD2_hpptr];
    halfPtr = m_RD2_hpptr-(HPBUFFER_LGTH/2) ;
    if(halfPtr < 0)
        halfPtr += HPBUFFER_LGTH ;
    z = m_RD2_hpdata[halfPtr] - (m_RD2_y / HPBUFFER_LGTH);

    m_RD2_hpdata[m_RD2_hpptr] = datum ;
    if(++m_RD2_hpptr == HPBUFFER_LGTH)
        m_RD2_hpptr = 0 ;

    return( z );
}

int CQRSDet::RD2deriv1(int x, int init)
{
    int y ;

    if(init != 0)
    {
        for(m_RD2_derI1 = 0; m_RD2_derI1 < DERIV_LENGTH; ++m_RD2_derI1)
            m_RD2_derBuff1[m_RD2_derI1] = 0 ;
        m_RD2_derI1 = 0 ;
        return(0) ;
    }

    y = x - m_RD2_derBuff1[m_RD2_derI1] ;
    m_RD2_derBuff1[m_RD2_derI1] = x ;
    if(++m_RD2_derI1 == DERIV_LENGTH)
        m_RD2_derI1 = 0 ;
    return(y) ;
}

int CQRSDet::RD2deriv2(int x, int init)
{
    int y ;

    if(init != 0)
    {
        for(m_RD2_derI2 = 0; m_RD2_derI2 < DERIV_LENGTH; ++m_RD2_derI2)
            m_RD2_derBuff2[m_RD2_derI2] = 0 ;
        m_RD2_derI2 = 0 ;
        return(0) ;
    }

    y = x - m_RD2_derBuff2[m_RD2_derI2] ;
    m_RD2_derBuff2[m_RD2_derI2] = x ;
    if(++m_RD2_derI2 == DERIV_LENGTH)
        m_RD2_derI2 = 0 ;
    return(y) ;
}

int CQRSDet::RD2mvwint(int datum, int init)
{
    int output;
    if(init)
    {
        for(m_RD2_mwiptr = 0; m_RD2_mwiptr < WINDOW_WIDTH ; ++m_RD2_mwiptr)
            m_RD2_mwidata[m_RD2_mwiptr] = 0 ;
        m_RD2_mwisum = 0 ;
        m_RD2_mwiptr = 0 ;
    }
    m_RD2_mwisum += datum ;
    m_RD2_mwisum -= m_RD2_mwidata[m_RD2_mwiptr] ;
    m_RD2_mwidata[m_RD2_mwiptr] = datum ;
    if(++m_RD2_mwiptr == WINDOW_WIDTH)
        m_RD2_mwiptr = 0 ;
    if((m_RD2_mwisum / WINDOW_WIDTH) > 32000)
        output = 32000 ;
    else
        output = m_RD2_mwisum / WINDOW_WIDTH ;
    return(output) ;
}

int CQRSDet::QRSDet( int datum, int init )
{
    int fdatum, QrsDelay = 0 ;
    int i, newPeak, aPeak ;

/*	Initialize all buffers to 0 on the first call.	*/

    if( init )
    {
        for(i = 0; i < 8; ++i)
        {
            m_RD2_noise[i] = 0 ;	/* Initialize noise buffer */
            m_RD2_rrbuf[i] = MS1000 ;/* and R-to-R interval buffer. */
        }

        // Added by HMC
        RD2b8st = 0;
        m_RD2_First1s = 1;

        m_RD2_qpkcnt = m_RD2_maxder = m_RD2_lastmax = m_RD2_count = m_RD2_sbpeak = 0 ;
        m_RD2_initBlank = m_RD2_initMax = m_RD2_preBlankCnt = m_RD2_DDPtr = 0 ;
        m_RD2_sbcount = MS1500 ;
        QRSFilter(0,1) ;	/* initialize filters. */
        RD2Peak(0,1) ;
    }

    fdatum = QRSFilter(datum,0) ;	/* Filter data. */


    /* Wait until normal detector is ready before calling early detections. */

    aPeak = RD2Peak(fdatum,0) ;
    if(aPeak < MIN_PEAK_AMP)
        aPeak = 0 ;

    // Hold any peak that is detected for 200 ms
    // in case a bigger one comes along.  There
    // can only be one QRS complex in any 200 ms window.

    newPeak = 0 ;
    if(aPeak && !m_RD2_preBlankCnt)			// If there has been no peak for 200 ms
    {										// save this one and start counting.
        m_RD2_tempPeak = aPeak ;
        m_RD2_preBlankCnt = PRE_BLANK ;			// MS200
    }

    else if(!aPeak && m_RD2_preBlankCnt)	// If we have held onto a peak for
    {										// 200 ms pass it on for evaluation.
        if(--m_RD2_preBlankCnt == 0)
            newPeak = m_RD2_tempPeak ;
    }

    else if(aPeak)							// If we were holding a peak, but
    {										// this ones bigger, save it and
        if(aPeak > m_RD2_tempPeak)				// start counting to 200 ms again.
        {
            m_RD2_tempPeak = aPeak ;
            m_RD2_preBlankCnt = PRE_BLANK ; // MS200
        }
        else if(--m_RD2_preBlankCnt == 0)
            newPeak = m_RD2_tempPeak ;
    }

    /* Save derivative of raw signal for T-wave and baseline
	   shift discrimination. */

    m_RD2_DDBuffer[m_RD2_DDPtr] = RD2deriv1( datum, 0 ) ;
    if(++m_RD2_DDPtr == DER_DELAY)
        m_RD2_DDPtr = 0 ;

    /* Initialize the qrs peak buffer with the first eight 	*/
    /* local maximum peaks detected.						*/

    if( m_RD2_qpkcnt < 8 )
    {
        ++m_RD2_count ;
        if (newPeak > 0)
        {
            m_RD2_count = WINDOW_WIDTH;
        }
        if(++m_RD2_initBlank == MS1000)
        {
            m_RD2_initBlank = 0;
            if (m_RD2_First1s)
            {	// Skip the first peak
                m_RD2_First1s = 0;
                m_RD2_qrsbuf[m_RD2_qpkcnt] = 50;
            }
            else
                m_RD2_qrsbuf[m_RD2_qpkcnt] = m_RD2_initMax;
            m_RD2_initMax = 0;
            ++m_RD2_qpkcnt;
            if (m_RD2_qpkcnt == 8)
            {
                m_RD2_qmean = RD2mean(m_RD2_qrsbuf, 8);
                m_RD2_nmean = 0;
                m_RD2_rrmean = MS1000;
                m_RD2_sbcount = MS1500 + MS150;
                m_RD2_det_thresh = RD2thresh(m_RD2_qmean, m_RD2_nmean);
            }
        }
        if( newPeak > m_RD2_initMax )
            m_RD2_initMax = newPeak ;
    }

    else	/* Else test for a qrs. */
    {
        ++m_RD2_count ;

        // Added by HMC
        if( RD2b8st == 0 )
        {
            RD2_8to18Cnt = 0;
            RD2b8st = 1;
        }
        RD2_8to18Cnt++;
        if( RD2_8to18Cnt > FIX_R_MISS_PRD + 8 )		// 8: An arbitrary number
        {
            RD2_8to18Cnt = FIX_R_MISS_PRD + 8;
        }

        if(newPeak > 0)
        {


            /* Check for maximum derivative and matching minima and maxima
			   for T-wave and baseline shift rejection.  Only consider this
			   peak if it doesn't seem to be a base line shift. */

            if(!RD2BLSCheck(m_RD2_DDBuffer, m_RD2_DDPtr, &m_RD2_maxder))
            {


                // Classify the beat as a QRS complex
                // if the peak is larger than the detection threshold.

                if(newPeak > m_RD2_det_thresh)
                {
                    memmove(&m_RD2_qrsbuf[1], m_RD2_qrsbuf, m_RD2_MEMMOVELEN) ;
                    m_RD2_qrsbuf[0] = newPeak ;
                    m_RD2_qmean = RD2mean(m_RD2_qrsbuf,8) ;
                    m_RD2_det_thresh = RD2thresh(m_RD2_qmean,m_RD2_nmean) ;
                    memmove(&m_RD2_rrbuf[1], m_RD2_rrbuf, m_RD2_MEMMOVELEN) ;
                    m_RD2_rrbuf[0] = m_RD2_count - WINDOW_WIDTH ;
                    m_RD2_rrmean = RD2mean(m_RD2_rrbuf,8) ;
                    m_RD2_sbcount = m_RD2_rrmean + (m_RD2_rrmean >> 1) + WINDOW_WIDTH ;
                    m_RD2_count = WINDOW_WIDTH ;

                    m_RD2_sbpeak = 0 ;

                    m_RD2_lastmax = m_RD2_maxder ;
                    m_RD2_maxder = 0 ;
                    QrsDelay =  WINDOW_WIDTH + FILTER_DELAY ;
                    m_RD2_initBlank = m_RD2_initMax = m_RD2_rsetCount = 0 ;
                }

                    // If a peak isn't a QRS update noise buffer and estimate.
                    // Store the peak for possible search back.


                else
                {
                    memmove(&m_RD2_noise[1],m_RD2_noise,m_RD2_MEMMOVELEN) ;
                    m_RD2_noise[0] = newPeak ;
                    m_RD2_nmean = RD2mean(m_RD2_noise,8) ;
                    m_RD2_det_thresh = RD2thresh(m_RD2_qmean,m_RD2_nmean) ;

                    // Don't include early peaks (which might be T-waves)
                    // in the search back process.  A T-wave can mask
                    // a small following QRS.

                    if((newPeak > m_RD2_sbpeak) && ((m_RD2_count-WINDOW_WIDTH) >= MS360))
                    {
                        m_RD2_sbpeak = newPeak ;
                        m_RD2_sbloc = m_RD2_count  - WINDOW_WIDTH ;
                    }
                }
            }
        }

        /* Test for search back condition.  If a QRS is found in  */
        /* search back update the QRS buffer and m_RD2_det_thresh.      */

        // Added by HMC
        if( RD2_8to18Cnt > 0 && RD2_8to18Cnt <= FIX_R_MISS_PRD )
        {	if( m_RD2_sbcount > FIX_R_MISS_SBCMX )
                m_RD2_realSbcount = FIX_R_MISS_SBCMX;
            else
                m_RD2_realSbcount = m_RD2_sbcount;

            float fFixRMissThrsh = m_RD2_det_thresh * FIX_R_MISS_RTRSHR;
            int FixRMissThrsh = (int)(fFixRMissThrsh + 0.5);
            //if((m_RD2_count > m_RD2_sbcount) && (m_RD2_sbpeak > (m_RD2_det_thresh >> 1)))
            if((m_RD2_count > m_RD2_realSbcount) && (m_RD2_sbpeak > FixRMissThrsh))	// Modified by HMC and Added by HMC
            {
                memmove(&m_RD2_qrsbuf[1],m_RD2_qrsbuf,m_RD2_MEMMOVELEN) ;
                m_RD2_qrsbuf[0] = m_RD2_sbpeak ;
                m_RD2_qmean = RD2mean(m_RD2_qrsbuf,8) ;
                m_RD2_det_thresh = RD2thresh(m_RD2_qmean,m_RD2_nmean) ;
                memmove(&m_RD2_rrbuf[1],m_RD2_rrbuf,m_RD2_MEMMOVELEN) ;
                m_RD2_rrbuf[0] = m_RD2_sbloc ;
                m_RD2_rrmean = RD2mean(m_RD2_rrbuf,8) ;
                m_RD2_sbcount = m_RD2_rrmean + (m_RD2_rrmean >> 1) + WINDOW_WIDTH ;
                QrsDelay = m_RD2_count = m_RD2_count - m_RD2_sbloc ;
                QrsDelay += FILTER_DELAY ;
                m_RD2_sbpeak = 0 ;
                m_RD2_lastmax = m_RD2_maxder ;
                m_RD2_maxder = 0 ;

                m_RD2_initBlank = m_RD2_initMax = m_RD2_rsetCount = 0 ;
            }
        }
        else
        {
            if((m_RD2_count > m_RD2_sbcount) && (m_RD2_sbpeak > (m_RD2_det_thresh >> 1)))
            {
                memmove(&m_RD2_qrsbuf[1],m_RD2_qrsbuf,m_RD2_MEMMOVELEN) ;
                m_RD2_qrsbuf[0] = m_RD2_sbpeak ;
                m_RD2_qmean = RD2mean(m_RD2_qrsbuf,8) ;
                m_RD2_det_thresh = RD2thresh(m_RD2_qmean,m_RD2_nmean) ;
                memmove(&m_RD2_rrbuf[1],m_RD2_rrbuf,m_RD2_MEMMOVELEN) ;
                m_RD2_rrbuf[0] = m_RD2_sbloc ;
                m_RD2_rrmean = RD2mean(m_RD2_rrbuf,8) ;
                m_RD2_sbcount = m_RD2_rrmean + (m_RD2_rrmean >> 1) + WINDOW_WIDTH ;
                QrsDelay = m_RD2_count = m_RD2_count - m_RD2_sbloc ;
                QrsDelay += FILTER_DELAY ;
                m_RD2_sbpeak = 0 ;
                m_RD2_lastmax = m_RD2_maxder ;
                m_RD2_maxder = 0 ;

                m_RD2_initBlank = m_RD2_initMax = m_RD2_rsetCount = 0 ;
            }
        }
    }

    // In the background estimate threshold to replace adaptive threshold
    // if eight seconds elapses without a QRS detection.

    if( m_RD2_qpkcnt == 8 )
    {
        if(++m_RD2_initBlank == MS1000)
        {
            m_RD2_initBlank = 0 ;
            m_RD2_rsetBuff[m_RD2_rsetCount] = m_RD2_initMax ;
            m_RD2_initMax = 0 ;
            ++m_RD2_rsetCount ;

            // Reset threshold if it has been 8 seconds without
            // a detection.

            if(m_RD2_rsetCount == 8)
            {
                for(i = 0; i < 8; ++i)
                {
                    m_RD2_qrsbuf[i] = m_RD2_rsetBuff[i] ;
                    m_RD2_noise[i] = 0 ;
                }
                m_RD2_qmean = RD2mean( m_RD2_rsetBuff, 8 ) ;
                m_RD2_nmean = 0 ;
                m_RD2_rrmean = MS1000 ;
                m_RD2_sbcount = MS1500+MS150 ;
                m_RD2_det_thresh = RD2thresh(m_RD2_qmean,m_RD2_nmean) ;
                m_RD2_initBlank = m_RD2_initMax = m_RD2_rsetCount = 0 ;

                // Refresh Heart Rate display (Showing "x")
                //m_bCalHRFail = TRUE;
                //DisplayHeartRate();
            }
        }
        if( newPeak > m_RD2_initMax )
            m_RD2_initMax = newPeak ;
    }

    return(QrsDelay) ;
}

int CQRSDet::RD2Peak( int datum, int init )
{
    int pk = 0 ;

    if(init)
        m_RD2_pk_max = m_RD2_pk_timeSinceMax = 0 ;

    if(m_RD2_pk_timeSinceMax > 0)
        ++m_RD2_pk_timeSinceMax ;

    if((datum > m_RD2_pk_lastDatum) && (datum > m_RD2_pk_max))
    {
        m_RD2_pk_max = datum ;
        if(m_RD2_pk_max > 2)
            m_RD2_pk_timeSinceMax = 1 ;
    }

    else if(datum < (m_RD2_pk_max >> 1))
    {
        pk = m_RD2_pk_max ;
        m_RD2_pk_max = 0 ;
        m_RD2_pk_timeSinceMax = 0 ;
        m_RD2_Dly = 0 ;
    }

    else if(m_RD2_pk_timeSinceMax > MS95)
    {
        pk = m_RD2_pk_max ;
        m_RD2_pk_max = 0 ;
        m_RD2_pk_timeSinceMax = 0 ;
        m_RD2_Dly = 3 ;
    }
    m_RD2_pk_lastDatum = datum ;
    return(pk) ;

}

int CQRSDet::RD2mean(int *array, int datnum)
{
    long sum ;
    int i ;

    for(i = 0, sum = 0; i < datnum; ++i)
        sum += array[i] ;
    sum /= datnum ;
    return(sum) ;
}

int CQRSDet::RD2thresh(int qmean, int nmean)
{
    int thrsh, dmed ;
    double temp ;
    dmed = qmean - nmean ;
/*	thrsh = nmean + (dmed>>2) + (dmed>>3) + (dmed>>4); */
    temp = dmed ;
    temp *= m_RD2_TH ;
    dmed = temp ;
    thrsh = nmean + dmed ; /* dmed * THRESHOLD */
    return(thrsh) ;
}

int CQRSDet::RD2BLSCheck(int *dBuf,int dbPtr,int *maxder)
{
    int max, min, maxt, mint, t, x ;
    max = min = 0 ;

    for(t = 0; t < MS220; ++t)
    {
        x = dBuf[dbPtr] ;
        if(x > max)
        {
            maxt = t ;
            max = x ;
        }
        else if(x < min)
        {
            mint = t ;
            min = x;
        }
        if(++dbPtr == DER_DELAY)
            dbPtr = 0 ;
    }

    *maxder = max ;
    min = -min ;

    /* Possible beat if a maximum and minimum pair are found
		where the interval between them is less than 150 ms. */

    if((max > (min>>3)) && (min > (max>>3)) &&
       (abs(maxt - mint) < MS150))
        return(0) ;

    else
        return(1) ;
}


CBeatClassify::CBeatClassify(void)
{
    // Initialization

    ECGBufferIndex = 0 ;
    BeatQueCount = 0 ;
    RRCount = 0 ;
    InitBeatFlag = 1 ;
    TypeCount = 0 ;
    BeatCount = 0;
    ClassifyState	= LEARNING ;
    PCInitCount = 0 ;
    DMIrregCount = 0 ;
    NBPtr = 0 ;

    ST_elevation = false;
    bAF_fnd = false;
}

CBeatClassify::~CBeatClassify(void)
{
}

/******************************************************************************
	ResetBDAC() resets static variables required for beat detection and
	classification.
*******************************************************************************/

void CBeatClassify::ResetBDAC(void)
{
    int dummy ;
    m_QRSDetForAna.QRSDet(0,1) ;	// Reset the qrs detector
    RRCount = 0 ;
    Classify(BeatBuffer,0,0,&dummy,&dummy,1) ;
    InitBeatFlag = 1 ;
    BeatQueCount = 0 ;	// Flush the beat que.
}

/*****************************************************************************
Syntax:
	int BeatDetectAndClassify(int ecgSample, int *beatType, *beatMatch) ;

Description:
	BeatDetectAndClassify() implements a beat detector and classifier.
	ECG samples are passed into BeatDetectAndClassify() one sample at a
	time.  BeatDetectAndClassify has been designed for a sample rate of
	200 Hz.  When a beat has been detected and classified the detection
	delay is returned and the beat classification is returned through the
	pointer *beatType.  For use in debugging, the number of the template
   that the beat was matched to is returned in via *beatMatch.

Returns
	BeatDetectAndClassify() returns 0 if no new beat has been detected and
	classified.  If a beat has been classified, BeatDetectAndClassify returns
	the number of samples since the approximate location of the R-wave.

****************************************************************************/

int CBeatClassify::BeatDetectAndClassify(int ecgSample, int *beatType, int *beatMatch)
{
    int detectDelay, rr, i, j ;
    int noiseEst = 0, beatBegin, beatEnd ;
    int domType ;
    int fidAdj ;
    int tempBeat[(SAMPLE_RATE/BEAT_SAMPLE_RATE)*BEATLGTH] ;

    beatEnd = 400; // Added by HMC, initialization
    beatBegin = 140; // Added by HMC, initialization
    rr = 850; // Added by HMC, initialization

    ST_elevation = false;
    bAF_fnd = false;

    // Store new sample in the circular buffer.

    ECGBuffer[ECGBufferIndex] = ecgSample ;
    if(++ECGBufferIndex == ECG_BUFFER_LENGTH)
        ECGBufferIndex = 0 ;

    // Increment RRInterval count.

    ++RRCount ;

    // Increment detection delays for any beats in the que.

    for(i = 0; i < BeatQueCount; ++i)
        ++BeatQue[i] ;

    // Run the sample through the QRS detector.

    //detectDelay = m_QRSDetForAna.QRSDet(ecgSample,0) ;
    //if(detectDelay != 0)
    if(bDS_GloRfndForCla)
    {
        detectDelay = 1;
        BeatQue[BeatQueCount] = detectDelay ;
        ++BeatQueCount ;
    }

    // Return if no beat is ready for classification.

    if((BeatQue[0] < (BEATLGTH-FIDMARK)*(SAMPLE_RATE/BEAT_SAMPLE_RATE))
       || (BeatQueCount == 0))
    {
        NoiseCheck(ecgSample,0,rr, beatBegin, beatEnd) ;	// Update noise check buffer
        return 0 ;
    }

    // Otherwise classify the beat at the head of the que.

    rr = RRCount - BeatQue[0] ;	// Calculate the R-to-R interval
    detectDelay = RRCount = BeatQue[0] ;

    // Estimate low frequency noise in the beat.
    // Might want to move this into classify().

    domType = GetDominantType() ;
    if(domType == -1)
    {
        beatBegin = MS250 ;
        beatEnd = MS300 ;
    }
    else
    {
        beatBegin = (SAMPLE_RATE/BEAT_SAMPLE_RATE)*(FIDMARK-GetBeatBegin(domType)) ;
        beatEnd = (SAMPLE_RATE/BEAT_SAMPLE_RATE)*(GetBeatEnd(domType)-FIDMARK) ;
    }
    noiseEst = NoiseCheck(ecgSample,detectDelay,rr,beatBegin,beatEnd) ;

    // Copy the beat from the circular buffer to the beat buffer
    // and reduce the sample rate by averageing pairs of data
    // points.

    j = ECGBufferIndex - detectDelay - (SAMPLE_RATE/BEAT_SAMPLE_RATE)*FIDMARK ;
    if(j < 0) j += ECG_BUFFER_LENGTH ;

    for(i = 0; i < (SAMPLE_RATE/BEAT_SAMPLE_RATE)*BEATLGTH; ++i)
    {
        tempBeat[i] = ECGBuffer[j] ;
        if(++j == ECG_BUFFER_LENGTH)
            j = 0 ;
    }

    DownSampleBeat(BeatBuffer,tempBeat) ;

    // Update the QUE.

    for(i = 0; i < BeatQueCount-1; ++i)
        BeatQue[i] = BeatQue[i+1] ;
    --BeatQueCount ;


    // Skip the first beat.

    if(InitBeatFlag)
    {
        InitBeatFlag = 0 ;
        *beatType = 13 ;
        *beatMatch = 0 ;
        fidAdj = 0 ;
    }

        // Classify all other beats.

    else
    {
        *beatType = Classify(BeatBuffer,rr,noiseEst,beatMatch,&fidAdj,0) ;
        fidAdj *= SAMPLE_RATE/BEAT_SAMPLE_RATE ;
    }

    // Ignore detection if the classifier decides that this
    // was the trailing edge of a PVC.

    if(*beatType == 100)
    {
        RRCount += rr ;
        return(0) ;
    }

    // Limit the fiducial mark adjustment in case of problems with
    // beat onset and offset estimation.

    if(fidAdj > MS80)
        fidAdj = MS80 ;
    else if(fidAdj < -MS80)
        fidAdj = -MS80 ;

    //return(detectDelay-fidAdj) ;
    return(detectDelay);
}

void CBeatClassify::DownSampleBeat(int *beatOut, int *beatIn)
{
    int i ;

    float downAvg = 0.0;
    //float downAvgTot = SAMPLE_RATE / BEAT_SAMPLE_RATE;
    float downAvgTot = 2.0;
    float tempDownAvg = 0.0;
    for (i = 0; i < BEATLGTH * (SAMPLE_RATE / BEAT_SAMPLE_RATE); ++i)
    {
        if (i % (SAMPLE_RATE / BEAT_SAMPLE_RATE) == 0)
        {
            downAvg = 0.0;
            tempDownAvg = beatIn[i];
            downAvg += tempDownAvg / downAvgTot;
        }
        else if (i % (SAMPLE_RATE / BEAT_SAMPLE_RATE) == SAMPLE_RATE / BEAT_SAMPLE_RATE / 2)
        {
            tempDownAvg = beatIn[i];
            downAvg += tempDownAvg / downAvgTot;
        }
        else if (i % (SAMPLE_RATE / BEAT_SAMPLE_RATE) == SAMPLE_RATE / BEAT_SAMPLE_RATE - 1)
        {
            //tempDownAvg = beatIn[i];
            //downAvg += tempDownAvg / downAvgTot;
            beatOut[i / (SAMPLE_RATE / BEAT_SAMPLE_RATE)] = (int)downAvg;
        }
    }
}

/***************************************************************************
ResetMatch() resets static variables involved with template matching.
****************************************************************************/

void CBeatClassify::ResetMatch(void)
{
    int i, j ;
    TypeCount = 0 ;
    for(i = 0; i < MAXTYPES; ++i)
    {
        BeatCounts[i] = 0 ;
        BeatClassifications[i] = UNKNOWN ;
        for(j = 0; j < 8; ++j)
        {
            MIs[i][j] = 0 ;
        }
    }
}

/**************************************************************************
	CompareBeats() takes two beat buffers and compares how well they match
	point-by-point.  Beat2 is shifted and scaled to produce the closest
	possible match.  The metric returned is the sum of the absolute
	differences between beats divided by the amplitude of the beats.  The
	shift used for the match is returned via the pointer *shiftAdj.
***************************************************************************/

double CBeatClassify::CompareBeats(int *beat1, int *beat2, int *shiftAdj)
{
    int i, max, min, magSum, shift ;
    long beatDiff, meanDiff, minDiff, minShift ;
    double metric, scaleFactor, tempD ;

    // Calculate the magnitude of each beat.

    max = min = beat1[MATCH_START] ;
    for(i = MATCH_START+1; i < MATCH_END; ++i)
        if(beat1[i] > max)
            max = beat1[i] ;
        else if(beat1[i] < min)
            min = beat1[i] ;

    magSum = max - min ;

    i = MATCH_START ;
    max = min = beat2[i] ;
    for(i = MATCH_START+1; i < MATCH_END; ++i)
        if(beat2[i] > max)
            max = beat2[i] ;
        else if(beat2[i] < min)
            min = beat2[i] ;

    // magSum += max - min ;
    scaleFactor = magSum ;
    scaleFactor /= max-min ;
    magSum *= 2 ;

    // Calculate the sum of the point-by-point
    // absolute differences for five possible shifts.

    for(shift = -MAX_SHIFT; shift <= MAX_SHIFT; ++shift)
    {
        for(i = FIDMARK-(MATCH_LENGTH>>1), meanDiff = 0;
            i < FIDMARK + (MATCH_LENGTH>>1); ++i)
        {
            tempD = beat2[i+shift] ;
            tempD *= scaleFactor ;
            meanDiff += beat1[i]- tempD ; // beat2[i+shift] ;
        }
        meanDiff /= MATCH_LENGTH ;

        for(i = FIDMARK-(MATCH_LENGTH>>1), beatDiff = 0;
            i < FIDMARK + (MATCH_LENGTH>>1); ++i)
        {
            tempD = beat2[i+shift] ;
            tempD *= scaleFactor ;
            beatDiff += abs(beat1[i] - meanDiff- tempD) ; // beat2[i+shift]  ) ;
        }


        if(shift == -MAX_SHIFT)
        {
            minDiff = beatDiff ;
            minShift = -MAX_SHIFT ;
        }
        else if(beatDiff < minDiff)
        {
            minDiff = beatDiff ;
            minShift = shift ;
        }
    }

    metric = minDiff ;
    *shiftAdj = minShift ;
    metric /= magSum ;

    // Metric scales inversely with match length.
    // algorithm was originally tuned with a match
    // length of 30.

    metric *= 30 ;
    metric /= MATCH_LENGTH ;
    return(metric) ;
}

/***************************************************************************
	CompareBeats2 is nearly the same as CompareBeats above, but beat2 is
	not scaled before calculating the match metric.  The match metric is
	then the sum of the absolute differences divided by the average amplitude
	of the two beats.
****************************************************************************/

double CBeatClassify::CompareBeats2(int *beat1, int *beat2, int *shiftAdj)
{
    int i, max, min, shift ;
    int mag1, mag2 ;
    long beatDiff, meanDiff, minDiff, minShift ;
    double metric ;

    // Calculate the magnitude of each beat.

    max = min = beat1[MATCH_START] ;
    for(i = MATCH_START+1; i < MATCH_END; ++i)
        if(beat1[i] > max)
            max = beat1[i] ;
        else if(beat1[i] < min)
            min = beat1[i] ;

    mag1 = max - min ;

    i = MATCH_START ;
    max = min = beat2[i] ;
    for(i = MATCH_START+1; i < MATCH_END; ++i)
        if(beat2[i] > max)
            max = beat2[i] ;
        else if(beat2[i] < min)
            min = beat2[i] ;

    mag2 = max-min ;

    // Calculate the sum of the point-by-point
    // absolute differences for five possible shifts.

    for(shift = -MAX_SHIFT; shift <= MAX_SHIFT; ++shift)
    {
        for(i = FIDMARK-(MATCH_LENGTH>>1), meanDiff = 0;
            i < FIDMARK + (MATCH_LENGTH>>1); ++i)
            meanDiff += beat1[i]- beat2[i+shift] ;
        meanDiff /= MATCH_LENGTH ;

        for(i = FIDMARK-(MATCH_LENGTH>>1), beatDiff = 0;
            i < FIDMARK + (MATCH_LENGTH>>1); ++i)
            beatDiff += abs(beat1[i] - meanDiff- beat2[i+shift]) ; ;

        if(shift == -MAX_SHIFT)
        {
            minDiff = beatDiff ;
            minShift = -MAX_SHIFT ;
        }
        else if(beatDiff < minDiff)
        {
            minDiff = beatDiff ;
            minShift = shift ;
        }
    }

    metric = minDiff ;
    *shiftAdj = minShift ;
    metric /= (mag1+mag2) ;

    // Metric scales inversely with match length.
    // algorithm was originally tuned with a match
    // length of 30.

    metric *= 30 ;
    metric /= MATCH_LENGTH ;

    return(metric) ;
}

/************************************************************************
UpdateBeat() averages a new beat into an average beat template by adding
1/8th of the new beat to 7/8ths of the average beat.
*************************************************************************/

void CBeatClassify::UpdateBeat(int *aveBeat, int *newBeat, int shift)
{
    int i ;
    long tempLong ;

    for(i = 0; i < BEATLGTH; ++i)
    {
        if((i+shift >= 0) && (i+shift < BEATLGTH))
        {
            tempLong = aveBeat[i] ;
            tempLong *= 7 ;
            tempLong += newBeat[i+shift] ;
            tempLong >>= 3 ;
            aveBeat[i] = tempLong ;
        }
    }
}

/*******************************************************
	GetTypesCount returns the number of types that have
	been detected.
*******************************************************/

int CBeatClassify::GetTypesCount(void)
{
    return(TypeCount) ;
}

/********************************************************
	GetBeatTypeCount returns the number of beats of a
	a particular type have been detected.
********************************************************/

int CBeatClassify::GetBeatTypeCount(int type)
{
    return(BeatCounts[type]) ;
}

/*******************************************************
	GetBeatWidth returns the QRS width estimate for
	a given type of beat.
*******************************************************/
int CBeatClassify::GetBeatWidth(int type)
{
    return(BeatWidths[type]) ;
}

/*******************************************************
	GetBeatCenter returns the point between the onset and
	offset of a beat.
********************************************************/

int CBeatClassify::GetBeatCenter(int type)
{
    return(BeatCenters[type]) ;
}

/*******************************************************
	GetBeatClass returns the present classification for
	a given beat type (NORMAL, PVC, or UNKNOWN).
********************************************************/

int CBeatClassify::GetBeatClass(int type)
{
    if(type == MAXTYPES)
        return(UNKNOWN) ;
    return(BeatClassifications[type]) ;
}

/******************************************************
	SetBeatClass sets up a beat classifation for a
	given type.
******************************************************/

void CBeatClassify::SetBeatClass(int type, int beatClass)
{
    BeatClassifications[type] = beatClass ;
}

/******************************************************************************
	NewBeatType starts a new beat type by storing the new beat and its
	features as the next available beat type.
******************************************************************************/

int CBeatClassify::NewBeatType(int *newBeat )
{
    int i, onset, offset, isoLevel, beatBegin, beatEnd ;
    int mcType, amp ;

    // Update count of beats since each template was matched.

    for(i = 0; i < TypeCount; ++i)
        ++BeatsSinceLastMatch[i] ;

    if(TypeCount < MAXTYPES)
    {
        for(i = 0; i < BEATLGTH; ++i)
            BeatTemplates[TypeCount][i] = newBeat[i] ;

        BeatCounts[TypeCount] = 1 ;
        BeatClassifications[TypeCount] = UNKNOWN ;
        AnalyzeBeat(&BeatTemplates[TypeCount][0],&onset,&offset, &isoLevel,
                    &beatBegin, &beatEnd, &amp) ;
        BeatWidths[TypeCount] = offset-onset ;
        BeatCenters[TypeCount] = (offset+onset)/2 ;
        BeatBegins[TypeCount] = beatBegin ;
        BeatEnds[TypeCount] = beatEnd ;
        BeatAmps[TypeCount] = amp ;

        BeatsSinceLastMatch[TypeCount] = 0 ;

        ++TypeCount ;
        return(TypeCount-1) ;
    }

        // If we have used all the template space, replace the beat
        // that has occurred the fewest number of times.

    else
    {
        // Find the template with the fewest occurances,
        // that hasn't been matched in at least 500 beats.

        mcType = -1 ;

        if(mcType == -1)
        {
            mcType = 0 ;
            for(i = 1; i < MAXTYPES; ++i)
                if(BeatCounts[i] < BeatCounts[mcType])
                    mcType = i ;
                else if(BeatCounts[i] == BeatCounts[mcType])
                {
                    if(BeatsSinceLastMatch[i] > BeatsSinceLastMatch[mcType])
                        mcType = i ;
                }
        }

        // Adjust dominant beat monitor data.

        AdjustDomData(mcType,MAXTYPES) ;

        // Substitute this beat.

        for(i = 0; i < BEATLGTH; ++i)
            BeatTemplates[mcType][i] = newBeat[i] ;

        BeatCounts[mcType] = 1 ;
        BeatClassifications[mcType] = UNKNOWN ;
        AnalyzeBeat(&BeatTemplates[mcType][0],&onset,&offset, &isoLevel,
                    &beatBegin, &beatEnd, &amp) ;
        BeatWidths[mcType] = offset-onset ;
        BeatCenters[mcType] = (offset+onset)/2 ;
        BeatBegins[mcType] = beatBegin ;
        BeatEnds[mcType] = beatEnd ;
        BeatsSinceLastMatch[mcType] = 0 ;
        BeatAmps[mcType] = amp ;
        return(mcType) ;
    }
}

/***************************************************************************
	BestMorphMatch tests a new beat against all available beat types and
	returns (via pointers) the existing type that best matches, the match
	metric for that type, and the shift used for that match.
***************************************************************************/

void CBeatClassify::BestMorphMatch(int *newBeat,int *matchType,double *matchIndex, double *mi2,
                                   int *shiftAdj)
{
    int type, i, bestMatch, nextBest, minShift, shift, temp ;
    int bestShift2, nextShift2 ;
    double bestDiff2, nextDiff2;
    double beatDiff, minDiff, nextDiff=10000 ;

    if(TypeCount == 0)
    {
        *matchType = 0 ;
        *matchIndex = 1000 ;		// Make sure there is no match so a new beat is
        *shiftAdj = 0 ;			// created.
        return ;
    }

    // Compare the new beat to all type beat
    // types that have been saved.

    for(type = 0; type < TypeCount; ++type)
    {
        beatDiff = CompareBeats(&BeatTemplates[type][0],newBeat,&shift) ;
        if(type == 0)
        {
            bestMatch = 0 ;
            minDiff = beatDiff ;
            minShift = shift ;
        }
        else if(beatDiff < minDiff)
        {
            nextBest = bestMatch ;
            nextDiff = minDiff ;
            bestMatch = type ;
            minDiff = beatDiff ;
            minShift = shift ;
        }
        else if((TypeCount > 1) && (type == 1))
        {
            nextBest = type ;
            nextDiff = beatDiff ;
        }
        else if(beatDiff < nextDiff)
        {
            nextBest = type ;
            nextDiff = beatDiff ;
        }
    }

    // If this beat was close to two different
    // templates, see if the templates which template
    // is the best match when no scaling is used.
    // Then check whether the two close types can be combined.

    if((minDiff < MATCH_LIMIT) && (nextDiff < MATCH_LIMIT) && (TypeCount > 1))
    {
        // Compare without scaling.

        bestDiff2 = CompareBeats2(&BeatTemplates[bestMatch][0],newBeat,&bestShift2) ;
        nextDiff2 = CompareBeats2(&BeatTemplates[nextBest][0],newBeat,&nextShift2) ;
        if(nextDiff2 < bestDiff2)
        {
            temp = bestMatch ;
            bestMatch = nextBest ;
            nextBest = temp ;
            temp = minDiff ;
            minDiff = nextDiff ;
            nextDiff = temp ;
            minShift = nextShift2 ;
            *mi2 = bestDiff2 ;
        }
        else *mi2 = nextDiff2 ;

        beatDiff = CompareBeats(&BeatTemplates[bestMatch][0],&BeatTemplates[nextBest][0],&shift) ;

        if((beatDiff < COMBINE_LIMIT) &&
           ((*mi2 < 1.0) || (!MinimumBeatVariation(nextBest))))
        {

            // Combine beats into bestMatch

            if(bestMatch < nextBest)
            {
                for(i = 0; i < BEATLGTH; ++i)
                {
                    if((i+shift > 0) && (i + shift < BEATLGTH))
                    {
                        BeatTemplates[bestMatch][i] += BeatTemplates[nextBest][i+shift] ;
                        BeatTemplates[bestMatch][i] >>= 1 ;
                    }
                }

                if((BeatClassifications[bestMatch] == NORMAL) || (BeatClassifications[nextBest] == NORMAL))
                    BeatClassifications[bestMatch] = NORMAL ;
                else if((BeatClassifications[bestMatch] == PVC) || (BeatClassifications[nextBest] == PVC))
                    BeatClassifications[bestMatch] = PVC ;

                BeatCounts[bestMatch] += BeatCounts[nextBest] ;

                CombineDomData(nextBest,bestMatch) ;

                // Shift other templates over.

                for(type = nextBest; type < TypeCount-1; ++type)
                    BeatCopy(type+1,type) ;

            }

                // Otherwise combine beats it nextBest.

            else
            {
                for(i = 0; i < BEATLGTH; ++i)
                {
                    BeatTemplates[nextBest][i] += BeatTemplates[bestMatch][i] ;
                    BeatTemplates[nextBest][i] >>= 1 ;
                }

                if((BeatClassifications[bestMatch] == NORMAL) || (BeatClassifications[nextBest] == NORMAL))
                    BeatClassifications[nextBest] = NORMAL ;
                else if((BeatClassifications[bestMatch] == PVC) || (BeatClassifications[nextBest] == PVC))
                    BeatClassifications[nextBest] = PVC ;

                BeatCounts[nextBest] += BeatCounts[bestMatch] ;

                CombineDomData(bestMatch,nextBest) ;

                // Shift other templates over.

                for(type = bestMatch; type < TypeCount-1; ++type)
                    BeatCopy(type+1,type) ;


                bestMatch = nextBest ;
            }
            --TypeCount ;
            BeatClassifications[TypeCount] = UNKNOWN ;
        }
    }
    *mi2 = CompareBeats2(&BeatTemplates[bestMatch][0],newBeat,&bestShift2) ;
    *matchType = bestMatch ;
    *matchIndex = minDiff ;
    *shiftAdj = minShift ;
}

/***************************************************************************
	UpdateBeatType updates the beat template and features of a given beat type
	using a new beat.
***************************************************************************/

void CBeatClassify::UpdateBeatType(int matchType,int *newBeat, double mi2,
                                   int shiftAdj)
{
    int i,onset,offset, isoLevel, beatBegin, beatEnd ;
    int amp ;

    // Update beats since templates were matched.

    for(i = 0; i < TypeCount; ++i)
    {
        if(i != matchType)
            ++BeatsSinceLastMatch[i] ;
        else BeatsSinceLastMatch[i] = 0 ;
    }

    // If this is only the second beat, average it with the existing
    // template.

    if(BeatCounts[matchType] == 1)
        for(i = 0; i < BEATLGTH; ++i)
        {
            if((i+shiftAdj >= 0) && (i+shiftAdj < BEATLGTH))
                BeatTemplates[matchType][i] = (BeatTemplates[matchType][i] + newBeat[i+shiftAdj])>>1 ;
        }

        // Otherwise do a normal update.

    else
        UpdateBeat(&BeatTemplates[matchType][0], newBeat, shiftAdj) ;

    // Determine beat features for the new average beat.

    AnalyzeBeat(&BeatTemplates[matchType][0],&onset,&offset,&isoLevel,
                &beatBegin, &beatEnd, &amp) ;

    BeatWidths[matchType] = offset-onset ;
    BeatCenters[matchType] = (offset+onset)/2 ;
    BeatBegins[matchType] = beatBegin ;
    BeatEnds[matchType] = beatEnd ;
    BeatAmps[matchType] = amp ;

    ++BeatCounts[matchType] ;

    for(i = MAXPREV-1; i > 0; --i)
        MIs[matchType][i] = MIs[matchType][i-1] ;
    MIs[matchType][0] = mi2 ;

}


/****************************************************************************
	GetDominantType returns the NORMAL beat type that has occurred most
	frequently.
****************************************************************************/

int CBeatClassify::GetDominantType(void)
{
    int maxCount = 0, maxType = -1 ;
    int type, totalCount ;

    for(type = 0; type < MAXTYPES; ++type)
    {
        if((BeatClassifications[type] == NORMAL) && (BeatCounts[type] > maxCount))
        {
            maxType = type ;
            maxCount = BeatCounts[type] ;
        }
    }

    // If no normals are found and at least 300 beats have occurred, just use
    // the most frequently occurring beat.

    if(maxType == -1)
    {
        for(type = 0, totalCount = 0; type < TypeCount; ++type)
            totalCount += BeatCounts[type] ;
        if(totalCount > 300)
            for(type = 0; type < TypeCount; ++type)
                if(BeatCounts[type] > maxCount)
                {
                    maxType = type ;
                    maxCount = BeatCounts[type] ;
                }
    }

    return(maxType) ;
}


/***********************************************************************
	ClearLastNewType removes the last new type that was initiated
************************************************************************/

void CBeatClassify::ClearLastNewType(void)
{
    if(TypeCount != 0)
        --TypeCount ;
}

/****************************************************************
	GetBeatBegin returns the offset from the R-wave for the
	beginning of the beat (P-wave onset if a P-wave is found).
*****************************************************************/

int CBeatClassify::GetBeatBegin(int type)
{
    return(BeatBegins[type]) ;
}

/****************************************************************
	GetBeatEnd returns the offset from the R-wave for the end of
	a beat (T-wave offset).
*****************************************************************/

int CBeatClassify::GetBeatEnd(int type)
{
    return(BeatEnds[type]) ;
}

int CBeatClassify::GetBeatAmp(int type)
{
    return(BeatAmps[type]) ;
}


/************************************************************************
	DomCompare2 and DomCompare return similarity indexes between a given
	beat and the dominant normal type or a given type and the dominant
	normal type.
************************************************************************/

double CBeatClassify::DomCompare2(int *newBeat, int domType)
{
    int shift ;
    return(CompareBeats2(&BeatTemplates[domType][0],newBeat,&shift)) ;
}

double CBeatClassify::DomCompare(int newType, int domType)
{
    int shift ;
    return(CompareBeats2(&BeatTemplates[domType][0],&BeatTemplates[newType][0],
                         &shift)) ;
}

/*************************************************************************
BeatCopy copies beat data from a source beat to a destination beat.
*************************************************************************/

void CBeatClassify::BeatCopy(int srcBeat, int destBeat)
{
    int i ;

    // Copy template.

    for(i = 0; i < BEATLGTH; ++i)
        BeatTemplates[destBeat][i] = BeatTemplates[srcBeat][i] ;

    // Move feature information.

    BeatCounts[destBeat] = BeatCounts[srcBeat] ;
    BeatWidths[destBeat] = BeatWidths[srcBeat] ;
    BeatCenters[destBeat] = BeatCenters[srcBeat] ;
    for(i = 0; i < MAXPREV; ++i)
    {
        PostClass[destBeat][i] = PostClass[srcBeat][i] ;
        PCRhythm[destBeat][i] = PCRhythm[srcBeat][i] ;
    }

    BeatClassifications[destBeat] = BeatClassifications[srcBeat] ;
    BeatBegins[destBeat] = BeatBegins[srcBeat] ;
    BeatEnds[destBeat] = BeatBegins[srcBeat] ;
    BeatsSinceLastMatch[destBeat] = BeatsSinceLastMatch[srcBeat];
    BeatAmps[destBeat] = BeatAmps[srcBeat] ;

    // Adjust data in dominant beat monitor.

    AdjustDomData(srcBeat,destBeat) ;
}

/********************************************************************
	Minimum beat variation returns a 1 if the previous eight beats
	have all had similarity indexes less than 0.5.
*********************************************************************/

int CBeatClassify::MinimumBeatVariation(int type)
{
    int i ;
    for(i = 0; i < MAXTYPES; ++i)
        if(MIs[type][i] > 0.5)
            i = MAXTYPES+2 ;
    if(i == MAXTYPES)
        return(1) ;
    else return(0) ;
}

/**********************************************************************
	WideBeatVariation returns true if the average similarity index
	for a given beat type to its template is greater than WIDE_VAR_LIMIT.
***********************************************************************/

#define WIDE_VAR_LIMIT	0.50

int CBeatClassify::WideBeatVariation(int type)
{
    int i, n ;
    double aveMI ;

    n = BeatCounts[type] ;
    if(n > 8)
        n = 8 ;

    for(i = 0, aveMI = 0; i <n; ++i)
        aveMI += MIs[type][i] ;

    aveMI /= n ;
    if(aveMI > WIDE_VAR_LIMIT)
        return(1) ;
    else return(0) ;
}

/***************************************************************************
	ResetRhythmChk() resets static variables used for rhythm classification.
****************************************************************************/

void CBeatClassify::ResetRhythmChk(void)
{
    BeatCount = 0 ;
    ClassifyState = LEARNING ;
}

/*****************************************************************************
	RhythmChk() takes an R-to-R interval as input and, based on previous R-to-R
	intervals, classifys the interval as NORMAL, PVC, or UNKNOWN.
******************************************************************************/

int CBeatClassify::RhythmChk(int rr)
{
    int i, regular = 1 ;
    int NNEst, NVEst ;

    BigeminyFlag = 0 ;

    // Wait for at least 4 beats before classifying anything.

    if(BeatCount < 4)
    {
        if(++BeatCount == 4)
            ClassifyState = READY ;
    }

    // Stick the new RR interval into the RR interval Buffer.

    for(i = RBB_LENGTH-1; i > 0; --i)
    {
        RRBuffer[i] = RRBuffer[i-1] ;
        RRTypes[i] = RRTypes[i-1] ;
    }

    RRBuffer[0] = rr ;

    if(ClassifyState == LEARNING)
    {
        RRTypes[0] = QQ ;
        return(UNKNOWN) ;
    }

    // If we couldn't tell what the last interval was...

    if(RRTypes[1] == QQ)
    {
        for(i = 0, regular = 1; i < 3; ++i)
            if(RRMatch(RRBuffer[i],RRBuffer[i+1]) == 0)
                regular = 0 ;

        // If this, and the last three intervals matched, classify
        // it as Normal-Normal.

        if(regular == 1)
        {
            RRTypes[0] = NN ;
            return(NORMAL) ;
        }

        // Check for bigeminy.
        // Call bigeminy if every other RR matches and
        // consecutive beats do not match.

        for(i = 0, regular = 1; i < 6; ++i)
            if(RRMatch(RRBuffer[i],RRBuffer[i+2]) == 0)
                regular = 0 ;
        for(i = 0; i < 6; ++i)
            if(RRMatch(RRBuffer[i],RRBuffer[i+1]) != 0)
                regular = 0 ;

        if(regular == 1)
        {
            BigeminyFlag = 1 ;
            if(RRBuffer[0] < RRBuffer[1])
            {
                RRTypes[0] = NV ;
                RRTypes[1] = VN ;
                return(PVC) ;
            }
            else
            {
                RRTypes[0] = VN ;
                RRTypes[1] = NV ;
                return(NORMAL) ;
            }
        }

        // Check for NNVNNNV pattern.

        if(RRShort(RRBuffer[0],RRBuffer[1]) && RRMatch(RRBuffer[1],RRBuffer[2])
           && RRMatch(RRBuffer[2]*2,RRBuffer[3]+RRBuffer[4]) &&
           RRMatch(RRBuffer[4],RRBuffer[0]) && RRMatch(RRBuffer[5],RRBuffer[2]))
        {
            RRTypes[0] = NV ;
            RRTypes[1] = NN ;
            return(PVC) ;
        }

            // If the interval is not part of a
            // bigeminal or regular pattern, give up.

        else
        {
            RRTypes[0] = QQ ;
            return(UNKNOWN) ;
        }
    }

        // If the previous two beats were normal...

    else if(RRTypes[1] == NN)
    {

        if(RRShort2(RRBuffer,RRTypes))
        {
            if(RRBuffer[1] < BRADY_LIMIT)
            {
                RRTypes[0] = NV ;
                return(PVC) ;
            }
            else RRTypes[0] = QQ ;
            return(UNKNOWN) ;
        }


            // If this interval matches the previous interval, then it
            // is regular.

        else if(RRMatch(RRBuffer[0],RRBuffer[1]))
        {
            RRTypes[0] = NN ;
            return(NORMAL) ;
        }

            // If this interval is short..

        else if(RRShort(RRBuffer[0],RRBuffer[1]))
        {

            // But matches the one before last and the one before
            // last was NN, this is a normal interval.

            if(RRMatch(RRBuffer[0],RRBuffer[2]) && (RRTypes[2] == NN))
            {
                RRTypes[0] = NN ;
                return(NORMAL) ;
            }

                // If the rhythm wasn't bradycardia, call it a PVC.

            else if(RRBuffer[1] < BRADY_LIMIT)
            {
                RRTypes[0] = NV ;
                return(PVC) ;
            }

                // If the regular rhythm was bradycardia, don't assume that
                // it was a PVC.

            else
            {
                RRTypes[0] = QQ ;
                return(UNKNOWN) ;
            }
        }

            // If the interval isn't normal or short, then classify
            // it as normal but don't assume normal for future
            // rhythm classification.

        else
        {
            RRTypes[0] = QQ ;
            return(NORMAL) ;
        }
    }

        // If the previous beat was a PVC...

    else if(RRTypes[1] == NV)
    {

        if(RRShort2(&RRBuffer[1],&RRTypes[1]))
        {
            /*		if(RRMatch2(RRBuffer[0],RRBuffer[1]))
				{
				RRTypes[0] = VV ;
				return(PVC) ;
				} */

            if(RRMatch(RRBuffer[0],RRBuffer[1]))
            {
                RRTypes[0] = NN ;
                RRTypes[1] = NN ;
                return(NORMAL) ;
            }
            else if(RRBuffer[0] > RRBuffer[1])
            {
                RRTypes[0] = VN ;
                return(NORMAL) ;
            }
            else
            {
                RRTypes[0] = QQ ;
                return(UNKNOWN) ;
            }


        }

            // If this interval matches the previous premature
            // interval assume a ventricular couplet.

        else if(RRMatch(RRBuffer[0],RRBuffer[1]))
        {
            RRTypes[0] = VV ;
            return(PVC) ;
        }

            // If this interval is larger than the previous
            // interval, assume that it is NORMAL.

        else if(RRBuffer[0] > RRBuffer[1])
        {
            RRTypes[0] = VN ;
            return(NORMAL) ;
        }

            // Otherwise don't make any assumputions about
            // what this interval represents.

        else
        {
            RRTypes[0] = QQ ;
            return(UNKNOWN) ;
        }
    }

        // If the previous beat followed a PVC or couplet etc...

    else if(RRTypes[1] == VN)
    {

        // Find the last NN interval.

        for(i = 2; (RRTypes[i] != NN) && (i < RBB_LENGTH); ++i) ;

        // If there was an NN interval in the interval buffer...
        if(i != RBB_LENGTH)
        {
            NNEst = RRBuffer[i] ;

            // and it matches, classify this interval as NORMAL.

            if(RRMatch(RRBuffer[0],NNEst))
            {
                RRTypes[0] = NN ;
                return(NORMAL) ;
            }
        }

        else NNEst = 0 ;
        for(i = 2; (RRTypes[i] != NV) && (i < RBB_LENGTH); ++i) ;
        if(i != RBB_LENGTH)
            NVEst = RRBuffer[i] ;
        else NVEst = 0 ;
        if((NNEst == 0) && (NVEst != 0))
            NNEst = (RRBuffer[1]+NVEst) >> 1 ;

        // NNEst is either the last NN interval or the average
        // of the most recent NV and VN intervals.

        // If the interval is closer to NN than NV, try
        // matching to NN.

        if((NVEst != 0) &&
           (abs(NNEst - RRBuffer[0]) < abs(NVEst - RRBuffer[0])) &&
           RRMatch(NNEst,RRBuffer[0]))
        {
            RRTypes[0] = NN ;
            return(NORMAL) ;
        }

            // If this interval is closer to NV than NN, try
            // matching to NV.

        else if((NVEst != 0) &&
                (abs(NNEst - RRBuffer[0]) > abs(NVEst - RRBuffer[0])) &&
                RRMatch(NVEst,RRBuffer[0]))
        {
            RRTypes[0] = NV ;
            return(PVC) ;
        }

            // If equally close, or we don't have an NN or NV in the buffer,
            // who knows what it is.

        else
        {
            RRTypes[0] = QQ ;
            return(UNKNOWN) ;
        }
    }

        // Otherwise the previous interval must have been a VV

    else
    {

        // Does this match previous VV.

        if(RRMatch(RRBuffer[0],RRBuffer[1]))
        {
            RRTypes[0] = VV ;
            return(PVC) ;
        }

            // If this doesn't match a previous VV interval, assume
            // any new interval is recovery to Normal beat.

        else
        {
            if(RRShort(RRBuffer[0],RRBuffer[1]))
            {
                RRTypes[0] = QQ ;
                return(UNKNOWN) ;
            }
            else
            {
                RRTypes[0] = VN ;
                return(NORMAL) ;
            }
        }
    }
}


/***********************************************************************
	RRMatch() test whether two intervals are within 12.5% of their mean.
************************************************************************/

int CBeatClassify::RRMatch(int rr0,int rr1)
{
    if(abs(rr0-rr1) < ((rr0+rr1)>>3))
        return(1) ;
    else return(0) ;
}

/************************************************************************
	RRShort() tests whether an interval is less than 75% of the previous
	interval.
*************************************************************************/

int CBeatClassify::RRShort(int rr0, int rr1)
{
    if(rr0 < rr1-(rr1>>2))
        return(1) ;
    else return(0) ;
}

/*************************************************************************
	IsBigeminy() allows external access to the bigeminy flag to check whether
	a bigeminal rhythm is in progress.
**************************************************************************/

int CBeatClassify::IsBigeminy(void)
{
    return(BigeminyFlag) ;
}

/**************************************************************************
 Check for short interval in very regular rhythm.
**************************************************************************/

int CBeatClassify::RRShort2(int *rrIntervals, int *rrTypes)
{
    int rrMean = 0, i, nnCount ;

    for(i = 1, nnCount = 0; (i < 7) && (nnCount < 4); ++i)
        if(rrTypes[i] == NN)
        {
            ++nnCount ;
            rrMean += rrIntervals[i] ;
        }

    // Return if there aren't at least 4 normal intervals.

    if(nnCount != 4)
        return(0) ;
    rrMean >>= 2 ;


    for(i = 1, nnCount = 0; (i < 7) && (nnCount < 4); ++i)
        if(rrTypes[i] == NN)
        {
            if(abs(rrMean-rrIntervals[i]) > (rrMean>>4))
                i = 10 ;
        }

    if((i < 9) && (rrIntervals[0] < (rrMean - (rrMean>>3))))
        return(1) ;
    else
        return(0) ;
}

int CBeatClassify::RRMatch2(int rr0,int rr1)
{
    if(abs(rr0-rr1) < ((rr0+rr1)>>4))
        return(1) ;
    else return(0) ;
}

/****************************************************************
	IsoCheck determines whether the amplitudes of a run
	of data fall within a sufficiently small amplitude that
	the run can be considered isoelectric.
*****************************************************************/

int CBeatClassify::IsoCheck(int *data, int isoLength)
{
    int i, max, min ;

    for(i = 1, max=min = data[0]; i < isoLength; ++i)
    {
        if(data[i] > max)
            max = data[i] ;
        else if(data[i] < min)
            min = data[i] ;
    }

    if(max - min < ISO_LIMIT)
        return(1) ;
    return(0) ;
}


int CBeatClassify::AFisoCheck(int *data, int isoLength)
{
    int i, max, min ;

    for(i = 1, max=min = data[0]; i < isoLength; ++i)
    {
        if(data[i] > max)
            max = data[i] ;
        else if(data[i] < min)
            min = data[i] ;
    }

    if(max - min < AF_ISO_LIMIT)
        return(1) ;
    return(0) ;
}


/**********************************************************************
	AnalyzeBeat takes a beat buffer as input and returns (via pointers)
	estimates of the QRS onset, QRS offset, polarity, isoelectric level
	beat beginning (P-wave onset), and beat ending (T-wave offset).
	Analyze Beat assumes that the beat has been sampled at 100 Hz, is
	BEATLGTH long, and has an R-wave location of roughly FIDMARK.

	Note that beatBegin is the number of samples before FIDMARK that
	the beat begins and beatEnd is the number of samples after the
	FIDMARK that the beat ends.
************************************************************************/

#define INF_CHK_N	BEAT_MS40

void CBeatClassify::AnalyzeBeat(int *beat, int *onset, int *offset, int *isoLevel,
                                int *beatBegin, int *beatEnd, int *amp)
{
    int maxSlope = 0, maxSlopeI, minSlope = 0, minSlopeI  ;
    int maxV, minV ;
    int isoStart, isoEnd ;
    int slope, i ;

    // Search back from the fiducial mark to find the isoelectric
    // region preceeding the QRS complex.

    for(i = FIDMARK-ISO_LENGTH2; (i > 0) && (IsoCheck(&beat[i],ISO_LENGTH2) == 0); --i) ;

    // If the first search didn't turn up any isoelectric region, look for
    // a shorter isoelectric region.

    if(i == 0)
    {

        // Added by HMC
        //bAF_fnd = true;

        for(i = FIDMARK-ISO_LENGTH1; (i > 0) && (IsoCheck(&beat[i],ISO_LENGTH1) == 0); --i) ;
        isoStart = i + (ISO_LENGTH1 - 1) ;
    }
    else isoStart = i + (ISO_LENGTH2 - 1) ;

    // Search forward from the R-wave to find an isoelectric region following
    // the QRS complex.

    for(i = FIDMARK; (i < BEATLGTH) && (IsoCheck(&beat[i],ISO_LENGTH1) == 0); ++i) ;
    isoEnd = i ;

    // Added by HMC

    int AFfindIsoStart, AFfindIsoEnd;

    for(i = FIDMARK - AF_ISO_LENGTH; (i > 0) && (AFisoCheck(&beat[i], AF_ISO_LENGTH) == 0); --i) ;
    AFfindIsoStart = i;

    for(i = FIDMARK; (i < BEATLGTH) && (AFisoCheck(&beat[i], AF_ISO_LENGTH) == 0); ++i) ;
    AFfindIsoEnd = i ;

    if( AFfindIsoStart <= 0 && AFfindIsoEnd >= BEATLGTH - AF_ISO_LENGTH )
    {	// No isoelectric region found
        bAF_fnd = true;
    }

    // Find the maximum and minimum slopes on the
    // QRS complex.

    i = FIDMARK-BEAT_MS150 ;
    maxSlope = maxSlope = beat[i] - beat[i-1] ;
    maxSlopeI = minSlopeI = i ;

    for(; i < FIDMARK+BEAT_MS150; ++i)
    {
        slope = beat[i] - beat[i-1] ;
        if(slope > maxSlope)
        {
            maxSlope = slope ;
            maxSlopeI = i ;
        }
        else if(slope < minSlope)
        {
            minSlope = slope ;
            minSlopeI = i ;
        }
    }

    // Use the smallest of max or min slope for search parameters.

    if(maxSlope > -minSlope)
        maxSlope = -minSlope ;
    else minSlope = -maxSlope ;

    if(maxSlopeI < minSlopeI)
    {

        // Search back from the maximum slope point for the QRS onset.

        for(i = maxSlopeI;
            (i > 0) && ((beat[i]-beat[i-1]) > (maxSlope >> 2)); --i) ;
        *onset = i-1 ;

        // Check to see if this was just a brief inflection.

        for(; (i > *onset-INF_CHK_N) && ((beat[i]-beat[i-1]) <= (maxSlope >>2)); --i) ;
        if(i > *onset-INF_CHK_N)
        {
            for(;(i > 0) && ((beat[i]-beat[i-1]) > (maxSlope >> 2)); --i) ;
            *onset = i-1 ;
        }
        i = *onset+1 ;

        // Check to see if a large negative slope follows an inflection.
        // If so, extend the onset a little more.

        for(;(i > *onset-INF_CHK_N) && ((beat[i-1]-beat[i]) < (maxSlope>>2)); --i);
        if(i > *onset-INF_CHK_N)
        {
            for(; (i > 0) && ((beat[i-1]-beat[i]) > (maxSlope>>2)); --i) ;
            *onset = i-1 ;
        }

        // Search forward from minimum slope point for QRS offset.

        for(i = minSlopeI;
            (i < BEATLGTH) && ((beat[i] - beat[i-1]) < (minSlope >>2)); ++i);
        *offset = i ;

        // Make sure this wasn't just an inflection.

        for(; (i < *offset+INF_CHK_N) && ((beat[i]-beat[i-1]) >= (minSlope>>2)); ++i) ;
        if(i < *offset+INF_CHK_N)
        {
            for(;(i < BEATLGTH) && ((beat[i]-beat[i-1]) < (minSlope >>2)); ++i) ;
            *offset = i ;
        }
        i = *offset ;

        // Check to see if there is a significant upslope following
        // the end of the down slope.

        for(;(i < *offset+BEAT_MS40) && ((beat[i-1]-beat[i]) > (minSlope>>2)); ++i);
        if(i < *offset+BEAT_MS40)
        {
            for(; (i < BEATLGTH) && ((beat[i-1]-beat[i]) < (minSlope>>2)); ++i) ;
            *offset = i ;

            // One more search motivated by PVC shape in 123.

            for(; (i < *offset+BEAT_MS60) && (beat[i]-beat[i-1] > (minSlope>>2)); ++i) ;
            if(i < *offset + BEAT_MS60)
            {
                for(;(i < BEATLGTH) && (beat[i]-beat[i-1] < (minSlope>>2)); ++i) ;
                *offset = i ;
            }
        }
    }

    else
    {

        // Search back from the minimum slope point for the QRS onset.

        for(i = minSlopeI;
            (i > 0) && ((beat[i]-beat[i-1]) < (minSlope >> 2)); --i) ;
        *onset = i-1 ;

        // Check to see if this was just a brief inflection.

        for(; (i > *onset-INF_CHK_N) && ((beat[i]-beat[i-1]) >= (minSlope>>2)); --i) ;
        if(i > *onset-INF_CHK_N)
        {
            for(; (i > 0) && ((beat[i]-beat[i-1]) < (minSlope>>2));--i) ;
            *onset = i-1 ;
        }
        i = *onset+1 ;

        // Check for significant positive slope after a turning point.

        for(;(i > *onset-INF_CHK_N) && ((beat[i-1]-beat[i]) > (minSlope>>2)); --i);
        if(i > *onset-INF_CHK_N)
        {
            for(; (i > 0) && ((beat[i-1]-beat[i]) < (minSlope>>2)); --i) ;
            *onset = i-1 ;
        }

        // Search forward from maximum slope point for QRS offset.

        for(i = maxSlopeI;
            (i < BEATLGTH) && ((beat[i] - beat[i-1]) > (maxSlope >>2)); ++i) ;
        *offset = i ;

        // Check to see if this was just a brief inflection.

        for(; (i < *offset+INF_CHK_N) && ((beat[i] - beat[i-1]) <= (maxSlope >> 2)); ++i) ;
        if(i < *offset+INF_CHK_N)
        {
            for(;(i < BEATLGTH) && ((beat[i] - beat[i-1]) > (maxSlope >>2)); ++i) ;
            *offset = i ;
        }
        i = *offset ;

        // Check to see if there is a significant downslope following
        // the end of the up slope.

        for(;(i < *offset+BEAT_MS40) && ((beat[i-1]-beat[i]) < (maxSlope>>2)); ++i);
        if(i < *offset+BEAT_MS40)
        {
            for(; (i < BEATLGTH) && ((beat[i-1]-beat[i]) > (maxSlope>>2)); ++i) ;
            *offset = i ;
        }
    }

    // If the estimate of the beginning of the isoelectric level was
    // at the beginning of the beat, use the slope based QRS onset point
    // as the iso electric point.

    if((isoStart == ISO_LENGTH1-1)&& (*onset > isoStart)) // ** 4/19 **
        isoStart = *onset ;

        // Otherwise, if the isoelectric start and the slope based points
        // are close, use the isoelectric start point.

    else if(*onset-isoStart < BEAT_MS50)
        *onset = isoStart ;

    // If the isoelectric end and the slope based QRS offset are close
    // use the isoelectic based point.

    if(isoEnd - *offset < BEAT_MS50)
        *offset = isoEnd ;

    *isoLevel = beat[isoStart] ;


    // Find the maximum and minimum values in the QRS.

    for(i = *onset, maxV = minV = beat[*onset]; i < *offset; ++i)
        if(beat[i] > maxV)
            maxV = beat[i] ;
        else if(beat[i] < minV)
            minV = beat[i] ;

    // If the offset is significantly below the onset and the offset is
    // on a negative slope, add the next up slope to the width.

    if((beat[*onset]-beat[*offset] > ((maxV-minV)>>2)+((maxV-minV)>>3)))
    {

        // Find the maximum slope between the finish and the end of the buffer.

        for(i = maxSlopeI = *offset, maxSlope = beat[*offset] - beat[*offset-1];
            (i < *offset+BEAT_MS100) && (i < BEATLGTH); ++i)
        {
            slope = beat[i]-beat[i-1] ;
            if(slope > maxSlope)
            {
                maxSlope = slope ;
                maxSlopeI = i ;
            }
        }

        // Find the new offset.

        if(maxSlope > 0)
        {
            for(i = maxSlopeI;
                (i < BEATLGTH) && (beat[i]-beat[i-1] > (maxSlope>>1)); ++i) ;
            *offset = i ;
        }
    }

    // Determine beginning and ending of the beat.
    // Search for an isoelectric region that precedes the R-wave.
    // by at least 250 ms.

    for(i = FIDMARK-BEAT_MS250;
        (i >= BEAT_MS80) && (IsoCheck(&beat[i-BEAT_MS80],BEAT_MS80) == 0); --i) ;
    *beatBegin = i ;

    // If there was an isoelectric section at 250 ms before the
    // R-wave, search forward for the isoelectric region closest
    // to the R-wave.  But leave at least 50 ms between beat begin
    // and onset, or else normal beat onset is within PVC QRS complexes.
    // that screws up noise estimation.

    if(*beatBegin == FIDMARK-BEAT_MS250)
    {
        for(; (i < *onset-BEAT_MS50) &&
              (IsoCheck(&beat[i-BEAT_MS80],BEAT_MS80) == 1); ++i) ;
        *beatBegin = i-1 ;
    }

        // Rev 1.1
    else if(*beatBegin == BEAT_MS80 - 1)
    {
        for(; (i < *onset) && (IsoCheck(&beat[i-BEAT_MS80],BEAT_MS80) == 0); ++i);
        if(i < *onset)
        {
            for(; (i < *onset) && (IsoCheck(&beat[i-BEAT_MS80],BEAT_MS80) == 1); ++i) ;
            if(i < *onset)
                *beatBegin = i-1 ;
        }
    }

    // Search for the end of the beat as the first isoelectric
    // segment that follows the beat by at least 300 ms.

    for(i = FIDMARK+BEAT_MS300;
        (i < BEATLGTH) && (IsoCheck(&beat[i],BEAT_MS80) == 0); ++i) ;
    *beatEnd = i ;

    // If the signal was isoelectric at 300 ms. search backwards.
/*	if(*beatEnd == FIDMARK+30)
		{
		for(; (i > *offset) && (IsoCheck(&beat[i],8) != 0); --i) ;
		*beatEnd = i ;
		}
*/
    // Calculate beat amplitude.

    maxV=minV=beat[*onset] ;
    for(i = *onset; i < *offset; ++i)
        if(beat[i] > maxV)
            maxV = beat[i] ;
        else if(beat[i] < minV)
            minV = beat[i] ;
    *amp = maxV-minV ;

}


/**********************************************************************
 Resets post classifications for beats.
**********************************************************************/

void CBeatClassify::ResetPostClassify()
{
    int i, j ;
    for(i = 0; i < MAXTYPES; ++i)
        for(j = 0; j < 8; ++j)
        {
            PostClass[i][j] = 0 ;
            PCRhythm[i][j] = 0 ;
        }
    PCInitCount = 0 ;
}

/***********************************************************************
	Classify the previous beat type and rhythm type based on this beat
	and the preceding beat.  This classifier is more sensitive
	to detecting premature beats followed by compensitory pauses.
************************************************************************/

void CBeatClassify::PostClassify(int *recentTypes, int domType, int *recentRRs, int width, double mi2,
                                 int rhythmClass)
{
    static int lastRC, lastWidth ;
    static double lastMI2 ;
    int i, regCount, pvcCount, normRR ;
    double mi3 ;

    // If the preceeding and following beats are the same type,
    // they are generally regular, and reasonably close in shape
    // to the dominant type, consider them to be dominant.

    if((recentTypes[0] == recentTypes[2]) && (recentTypes[0] != domType)
       && (recentTypes[0] != recentTypes[1]))
    {
        mi3 = DomCompare(recentTypes[0],domType) ;
        for(i = regCount = 0; i < 8; ++i)
            if(PCRhythm[recentTypes[0]][i] == NORMAL)
                ++regCount ;
        if((mi3 < 2.0) && (regCount > 6))
            domType = recentTypes[0] ;
    }

    // Don't do anything until four beats have gone by.

    if(PCInitCount < 3)
    {
        ++PCInitCount ;
        lastWidth = width ;
        lastMI2 = 0 ;
        lastRC = 0 ;
        return ;
    }

    if(recentTypes[1] < MAXTYPES)
    {

        // Find first NN interval.
        for(i = 2; (i < 7) && (recentTypes[i] != recentTypes[i+1]); ++i) ;
        if(i == 7) normRR = 0 ;
        else normRR = recentRRs[i] ;

        // Shift the previous beat classifications to make room for the
        // new classification.
        for(i = pvcCount = 0; i < 8; ++i)
            if(PostClass[recentTypes[1]][i] == PVC)
                ++pvcCount ;

        for(i = 7; i > 0; --i)
        {
            PostClass[recentTypes[1]][i] = PostClass[recentTypes[1]][i-1] ;
            PCRhythm[recentTypes[1]][i] = PCRhythm[recentTypes[1]][i-1] ;
        }

        // If the beat is premature followed by a compensitory pause and the
        // previous and following beats are normal, post classify as
        // a PVC.

        if(((normRR-(normRR>>3)) >= recentRRs[1]) && ((recentRRs[0]-(recentRRs[0]>>3)) >= normRR)// && (lastMI2 > 3)
           && (recentTypes[0] == domType) && (recentTypes[2] == domType)
           && (recentTypes[1] != domType))
            PostClass[recentTypes[1]][0] = PVC ;

            // If previous two were classified as PVCs, and this is at least slightly
            // premature, classify as a PVC.

        else if(((normRR-(normRR>>4)) > recentRRs[1]) && ((normRR+(normRR>>4)) < recentRRs[0]) &&
                (((PostClass[recentTypes[1]][1] == PVC) && (PostClass[recentTypes[1]][2] == PVC)) ||
                 (pvcCount >= 6) ) &&
                (recentTypes[0] == domType) && (recentTypes[2] == domType) && (recentTypes[1] != domType))
            PostClass[recentTypes[1]][0] = PVC ;

            // If the previous and following beats are the dominant beat type,
            // and this beat is significantly different from the dominant,
            // call it a PVC.

        else if((recentTypes[0] == domType) && (recentTypes[2] == domType) && (lastMI2 > 2.5))
            PostClass[recentTypes[1]][0] = PVC ;

            // Otherwise post classify this beat as UNKNOWN.

        else PostClass[recentTypes[1]][0] = UNKNOWN ;

        // If the beat is premature followed by a compensitory pause, post
        // classify the rhythm as PVC.

        if(((normRR-(normRR>>3)) > recentRRs[1]) && ((recentRRs[0]-(recentRRs[0]>>3)) > normRR))
            PCRhythm[recentTypes[1]][0] = PVC ;

            // Otherwise, post classify the rhythm as the same as the
            // regular rhythm classification.

        else PCRhythm[recentTypes[1]][0] = lastRC ;
    }

    lastWidth = width ;
    lastMI2 = mi2 ;
    lastRC = rhythmClass ;
}


/*************************************************************************
	CheckPostClass checks to see if three of the last four or six of the
	last eight of a given beat type have been post classified as PVC.
*************************************************************************/

int CBeatClassify::CheckPostClass(int type)
{
    int i, pvcs4 = 0, pvcs8 ;

    if(type == MAXTYPES)
        return(UNKNOWN) ;

    for(i = 0; i < 4; ++i)
        if(PostClass[type][i] == PVC)
            ++pvcs4 ;
    for(pvcs8=pvcs4; i < 8; ++i)
        if(PostClass[type][i] == PVC)
            ++pvcs8 ;

    if((pvcs4 >= 3) || (pvcs8 >= 6))
        return(PVC) ;
    else return(UNKNOWN) ;
}

/****************************************************************************
	Check classification of previous beats' rhythms based on post beat
	classification.  If 7 of 8 previous beats were classified as NORMAL
	(regular) classify the beat type as NORMAL (regular).
	Call it a PVC if 2 of the last 8 were regular.
****************************************************************************/

int CBeatClassify::CheckPCRhythm(int type)
{
    int i, normCount, n ;


    if(type == MAXTYPES)
        return(UNKNOWN) ;

    if(GetBeatTypeCount(type) < 9)
        n = GetBeatTypeCount(type)-1 ;
    else n = 8 ;

    for(i = normCount = 0; i < n; ++i)
        if(PCRhythm[type][i] == NORMAL)
            ++normCount;
    if(normCount >= 7)
        return(NORMAL) ;
    if(((normCount == 0) && (n < 4)) ||
       ((normCount <= 1) && (n >= 4) && (n < 7)) ||
       ((normCount <= 2) && (n >= 7)))
        return(PVC) ;
    return(UNKNOWN) ;
}

/***************************************************************************
*  Classify() takes a beat buffer, the previous rr interval, and the present
*  noise level estimate and returns a beat classification of NORMAL, PVC, or
*  UNKNOWN.  The UNKNOWN classification is only returned.  The beat template
*  type that the beat has been matched to is returned through the pointer
*  *beatMatch for debugging display.  Passing anything other than 0 in init
*  resets the static variables used by Classify.
****************************************************************************/

int CBeatClassify::Classify(int *newBeat,int rr, int noiseLevel, int *beatMatch, int *fidAdj,
                            int init)
{
    int rhythmClass, beatClass, i, beatWidth, blShift ;
    static int morphType, runCount = 0 ;
    double matchIndex, domIndex, mi2 ;
    int shiftAdj ;
    int domType, domWidth, onset, offset, amp ;
    int beatBegin, beatEnd, tempClass ;
    int hfNoise, isoLevel ;
    static int lastIsoLevel=0, lastRhythmClass = UNKNOWN, lastBeatWasNew = 0 ;

    // If initializing...

    if(init)
    {
        ResetRhythmChk() ;
        ResetMatch() ;
        ResetPostClassify() ;
        runCount = 0 ;
        DomMonitor(0, 0, 0, 0, 1) ;
        return(0) ;
    }

    hfNoise = HFNoiseCheck(newBeat) ;	// Check for muscle noise.
    rhythmClass = RhythmChk(rr) ;			// Check the rhythm.

    // Estimate beat features.

    AnalyzeBeat(newBeat, &onset, &offset, &isoLevel,
                &beatBegin, &beatEnd, &amp) ;

    // Added ST Elevation check
    double ST_E = (double)(newBeat[offset+4] - isoLevel) * ADC_TO_MV;
    if( ST_E >= 0.2 || ST_E <= -0.2 )
        ST_elevation = true;

    blShift = abs(lastIsoLevel-isoLevel) ;
    lastIsoLevel = isoLevel ;

    // Make isoelectric level 0.

    for(i = 0; i < BEATLGTH; ++i)
        newBeat[i] -= isoLevel ;

    // If there was a significant baseline shift since
    // the last beat and the last beat was a new type,
    // delete the new type because it might have resulted
    // from a baseline shift.

    if( (blShift > BL_SHIFT_LIMIT)
        && (lastBeatWasNew == 1)
        && (lastRhythmClass == NORMAL)
        && (rhythmClass == NORMAL) )
        ClearLastNewType() ;

    lastBeatWasNew = 0 ;

    // Find the template that best matches this beat.

    BestMorphMatch(newBeat,&morphType,&matchIndex,&mi2,&shiftAdj) ;

    // Disregard noise if the match is good. (New)

    if(matchIndex < MATCH_NOISE_THRESHOLD)
        hfNoise = noiseLevel = blShift = 0 ;

    // Apply a stricter match limit to premature beats.

    if((matchIndex < MATCH_LIMIT_CLAS) && (rhythmClass == PVC) &&
       MinimumBeatVariation(morphType) && (mi2 > PVC_MATCH_WITH_AMP_LIMIT))
    {
        morphType = NewBeatType(newBeat) ;
        lastBeatWasNew = 1 ;
    }

        // Match if within standard match limits.

    else if((matchIndex < MATCH_LIMIT_CLAS) && (mi2 <= MATCH_WITH_AMP_LIMIT))
        UpdateBeatType(morphType,newBeat,mi2,shiftAdj) ;

        // If the beat isn't noisy but doesn't match, start a new beat.

    else if((blShift < BL_SHIFT_LIMIT) && (noiseLevel < NEW_TYPE_NOISE_THRESHOLD)
            && (hfNoise < NEW_TYPE_HF_NOISE_LIMIT))
    {
        morphType = NewBeatType(newBeat) ;
        lastBeatWasNew = 1 ;
    }

        // Even if it is a noisy, start new beat if it was an irregular beat.

    else if((lastRhythmClass != NORMAL) || (rhythmClass != NORMAL))
    {
        morphType = NewBeatType(newBeat) ;
        lastBeatWasNew = 1 ;
    }

        // If its noisy and regular, don't waste space starting a new beat.

    else morphType = MAXTYPES ;

    // Update recent rr and type arrays.

    for(i = 7; i > 0; --i)
    {
        RecentRRs[i] = RecentRRs[i-1] ;
        RecentTypes[i] = RecentTypes[i-1] ;
    }
    RecentRRs[0] = rr ;
    RecentTypes[0] = morphType ;

    lastRhythmClass = rhythmClass ;
    lastIsoLevel = isoLevel ;

    // Fetch beat features needed for classification.
    // Get features from average beat if it matched.

    if(morphType != MAXTYPES)
    {
        beatClass = GetBeatClass(morphType) ;
        beatWidth = GetBeatWidth(morphType) ;
        *fidAdj = GetBeatCenter(morphType)-FIDMARK ;

        // If the width seems large and there have only been a few
        // beats of this type, use the actual beat for width
        // estimate.

        if((beatWidth > offset-onset) && (GetBeatTypeCount(morphType) <= 4))
        {
            beatWidth = offset-onset ;
            *fidAdj = ((offset+onset)/2)-FIDMARK ;
        }
    }

        // If this beat didn't match get beat features directly
        // from this beat.

    else
    {
        beatWidth = offset-onset ;
        beatClass = UNKNOWN ;
        *fidAdj = ((offset+onset)/2)-FIDMARK ;
    }

    // Fetch dominant type beat features.

    DomType = domType = DomMonitor(morphType, rhythmClass, beatWidth, rr, 0) ;
    domWidth = GetBeatWidth(domType) ;

    // Compare the beat type, or actual beat to the dominant beat.

    if((morphType != domType) && (morphType != 8))
        domIndex = DomCompare(morphType,domType) ;
    else if(morphType == 8)
        domIndex = DomCompare2(newBeat,domType) ;
    else domIndex = matchIndex ;

    // Update post classificaton of the previous beat.

    PostClassify(RecentTypes, domType, RecentRRs, beatWidth, domIndex, rhythmClass) ;

    // Classify regardless of how the morphology
    // was previously classified.

    tempClass = TempClass(rhythmClass, morphType, beatWidth, domWidth,
                          domType, hfNoise, noiseLevel, blShift, domIndex) ;

    // If this morphology has not been classified yet, attempt to classify
    // it.

    if((beatClass == UNKNOWN) && (morphType < MAXTYPES))
    {

        // Classify as normal if there are 6 in a row
        // or at least two in a row that meet rhythm
        // rules for normal.

        runCount = GetRunCount() ;

        // Classify a morphology as NORMAL if it is not too wide, and there
        // are three in a row.  The width criterion prevents ventricular beats
        // from being classified as normal during VTACH (MIT/BIH 205).

        if((runCount >= 3) && (domType != -1) && (beatWidth < domWidth+BEAT_MS20))
            SetBeatClass(morphType,NORMAL) ;

            // If there is no dominant type established yet, classify any type
            // with six in a row as NORMAL.

        else if((runCount >= 6) && (domType == -1))
            SetBeatClass(morphType,NORMAL) ;

            // During bigeminy, classify the premature beats as ventricular if
            // they are not too narrow.

        else if(IsBigeminy() == 1)
        {
            if((rhythmClass == PVC) && (beatWidth > BEAT_MS100))
                SetBeatClass(morphType,PVC) ;
            else if(rhythmClass == NORMAL)
                SetBeatClass(morphType,NORMAL) ;
        }
    }

    // Save morphology type of this beat for next classification.

    *beatMatch = morphType ;

    beatClass = GetBeatClass(morphType) ;

    // If the morphology has been previously classified.
    // use that classification.
    //	return(rhythmClass) ;

    if(beatClass != UNKNOWN)
        return(beatClass) ;

    if(CheckPostClass(morphType) == PVC)
        return(PVC) ;

    // Otherwise use the temporary classification.

    return(tempClass) ;
}

/**************************************************************************
*  HFNoiseCheck() gauges the high frequency (muscle noise) present in the
*  beat template.  The high frequency noise level is estimated by highpass
*  filtering the beat (y[n] = x[n] - 2*x[n-1] + x[n-2]), averaging the
*  highpass filtered signal over five samples, and finding the maximum of
*  this averaged highpass filtered signal.  The high frequency noise metric
*  is then taken to be the ratio of the maximum averaged highpassed signal
*  to the QRS amplitude.
**************************************************************************/

#define AVELENGTH	BEAT_MS50

int CBeatClassify::HFNoiseCheck(int *beat)
{
    int maxNoiseAve = 0, i ;
    int sum=0, aveBuff[AVELENGTH], avePtr = 0 ;
    int qrsMax = 0, qrsMin = 0 ;

    // Determine the QRS amplitude.

    for(i = FIDMARK-BEAT_MS70; i < FIDMARK+BEAT_MS80; ++i)
        if(beat[i] > qrsMax)
            qrsMax = beat[i] ;
        else if(beat[i] < qrsMin)
            qrsMin = beat[i] ;

    for(i = 0; i < AVELENGTH; ++i)
        aveBuff[i] = 0 ;

    for(i = FIDMARK-BEAT_MS280; i < FIDMARK+BEAT_MS280; ++i)
    {
        sum -= aveBuff[avePtr] ;
        aveBuff[avePtr] = abs(beat[i] - (beat[i-BEAT_MS10]<<1) + beat[i-2*BEAT_MS10]) ;
        sum += aveBuff[avePtr] ;
        if(++avePtr == AVELENGTH)
            avePtr = 0 ;
        if((i < (FIDMARK - BEAT_MS50)) || (i > (FIDMARK + BEAT_MS110)))
            if(sum > maxNoiseAve)
                maxNoiseAve = sum ;
    }
    if((qrsMax - qrsMin)>=4)
        return((maxNoiseAve * (50/AVELENGTH))/((qrsMax-qrsMin)>>2)) ;
    else return(0) ;
}

/************************************************************************
*  TempClass() classifies beats based on their beat features, relative
*  to the features of the dominant beat and the present noise level.
*************************************************************************/

int CBeatClassify::TempClass(int rhythmClass, int morphType,
                             int beatWidth, int domWidth, int domType,
                             int hfNoise, int noiseLevel, int blShift, double domIndex)
{

    // Rule 1:  If no dominant type has been detected classify all
    // beats as UNKNOWN.

    if(domType < 0)
        return(UNKNOWN) ;

    // Rule 2:  If the dominant rhythm is normal, the dominant
    // beat type doesn't vary much, this beat is premature
    // and looks sufficiently different than the dominant beat
    // classify as PVC.

    if(MinimumBeatVariation(domType) && (rhythmClass == PVC)
       && (domIndex > R2_DI_THRESHOLD) && (GetDomRhythm() == 1))
        return(PVC) ;

    // Rule 3:  If the beat is sufficiently narrow, classify as normal.

    if(beatWidth < R3_WIDTH_THRESHOLD)
        return(NORMAL) ;

    // Rule 5:  If the beat cannot be matched to any previously
    // detected morphology and it is not premature, consider it normal
    // (probably noisy).

    if((morphType == MAXTYPES) && (rhythmClass != PVC)) // == UNKNOWN
        return(NORMAL) ;

    // Rule 6:  If the maximum number of beat types have been stored,
    // this beat is not regular or premature and only one
    // beat of this morphology has been seen, call it normal (probably
    // noisy).

    if((GetTypesCount() == MAXTYPES) && (GetBeatTypeCount(morphType)==1)
       && (rhythmClass == UNKNOWN))
        return(NORMAL) ;

    // Rule 7:  If this beat looks like the dominant beat and the
    // rhythm is regular, call it normal.

    if((domIndex < R7_DI_THRESHOLD) && (rhythmClass == NORMAL))
        return(NORMAL) ;

    // Rule 8:  If post classification rhythm is normal for this
    // type and its shape is close to the dominant shape, classify
    // as normal.

    if((domIndex < R8_DI_THRESHOLD) && (CheckPCRhythm(morphType) == NORMAL))
        return(NORMAL) ;

    // Rule 9:  If the beat is not premature, it looks similar to the dominant
    // beat type, and the dominant beat type is variable (noisy), classify as
    // normal.

    if((domIndex < R9_DI_THRESHOLD) && (rhythmClass != PVC) && WideBeatVariation(domType))
        return(NORMAL) ;

    // Rule 10:  If this beat is significantly different from the dominant beat
    // there have previously been matching beats, the post rhythm classification
    // of this type is PVC, and the dominant rhythm is regular, classify as PVC.

    if((domIndex > R10_DI_THRESHOLD)
       && (GetBeatTypeCount(morphType) >= R10_BC_LIM) &&
       (CheckPCRhythm(morphType) == PVC) && (GetDomRhythm() == 1))
        return(PVC) ;

    // Rule 11: if the beat is wide, wider than the dominant beat, doesn't
    // appear to be noisy, and matches a previous type, classify it as
    // a PVC.

    if( (beatWidth >= R11_MIN_WIDTH) &&
        (((beatWidth - domWidth >= R11_WIDTH_DIFF1) && (domWidth < R11_WIDTH_BREAK)) ||
         (beatWidth - domWidth >= R11_WIDTH_DIFF2)) &&
        (hfNoise < R11_HF_THRESHOLD) && (noiseLevel < R11_MA_THRESHOLD) && (blShift < BL_SHIFT_LIMIT) &&
        (morphType < MAXTYPES) && (GetBeatTypeCount(morphType) > R11_BC_LIM))	// Rev 1.1

        return(PVC) ;

    // Rule 12:  If the dominant rhythm is regular and this beat is premature
    // then classify as PVC.

    if((rhythmClass == PVC) && (GetDomRhythm() == 1))
        return(PVC) ;

    // Rule 14:  If the beat is regular and the dominant rhythm is regular
    // call the beat normal.

    if((rhythmClass == NORMAL) && (GetDomRhythm() == 1))
        return(NORMAL) ;

    // By this point, we know that rhythm will not help us, so we
    // have to classify based on width and similarity to the dominant
    // beat type.

    // Rule 15: If the beat is wider than normal, wide on an
    // absolute scale, and significantly different from the
    // dominant beat, call it a PVC.

    if((beatWidth > domWidth) && (domIndex > R15_DI_THRESHOLD) &&
       (beatWidth >= R15_WIDTH_THRESHOLD))
        return(PVC) ;

    // Rule 16:  If the beat is sufficiently narrow, call it normal.

    if(beatWidth < R16_WIDTH_THRESHOLD)
        return(NORMAL) ;

    // Rule 17:  If the beat isn't much wider than the dominant beat
    // call it normal.

    if(beatWidth < domWidth + R17_WIDTH_DELTA)
        return(NORMAL) ;

    // If the beat is noisy but reasonably close to dominant,
    // call it normal.

    // Rule 18:  If the beat is similar to the dominant beat, call it normal.

    if(domIndex < R18_DI_THRESHOLD)
        return(NORMAL) ;

    // If it's noisy don't trust the width.

    // Rule 19:  If the beat is noisy, we can't trust our width estimate
    // and we have no useful rhythm information, so guess normal.

    if(hfNoise > R19_HF_THRESHOLD)
        return(NORMAL) ;

    // Rule 20:  By this point, we have no rhythm information, the beat
    // isn't particularly narrow, the beat isn't particulary similar to
    // the dominant beat, so guess a PVC.

    return(PVC) ;

}


/****************************************************************************
*  DomMonitor, monitors which beat morphology is considered to be dominant.
*  The dominant morphology is the beat morphology that has been most frequently
*  classified as normal over the course of the last 120 beats.  The dominant
*  beat rhythm is classified as regular if at least 3/4 of the dominant beats
*  have been classified as regular.
*******************************************************************************/

int CBeatClassify::DomMonitor(int morphType, int rhythmClass, int beatWidth, int rr, int reset)
{
    static int brIndex = 0 ;
    int i, oldType, runCount, dom, max ;

    // Fetch the type of the beat before the last beat.

    i = brIndex - 2 ;
    if(i < 0)
        i += DM_BUFFER_LENGTH ;
    oldType = DMBeatTypes[i] ;

    // If reset flag is set, reset beat type counts and
    // beat information buffers.

    if(reset != 0)
    {
        for(i = 0; i < DM_BUFFER_LENGTH; ++i)
        {
            DMBeatTypes[i] = -1 ;
            DMBeatClasses[i] = 0 ;
        }

        for(i = 0; i < 8; ++i)
        {
            DMNormCounts[i] = 0 ;
            DMBeatCounts[i] = 0 ;
        }
        DMIrregCount = 0 ;
        return(0) ;
    }

    // Once we have wrapped around, subtract old beat types from
    // the beat counts.

    if((DMBeatTypes[brIndex] != -1) && (DMBeatTypes[brIndex] != MAXTYPES))
    {
        if( DMBeatTypes[brIndex] >= 0 && DMBeatTypes[brIndex] < 8 )		// Added by HMC
        {	--DMBeatCounts[DMBeatTypes[brIndex]] ;
            DMNormCounts[DMBeatTypes[brIndex]] -= DMBeatClasses[brIndex] ;
            if(DMBeatRhythms[brIndex] == UNKNOWN)
                --DMIrregCount ;
        }
    }

    // If this is a morphology that has been detected before, decide
    // (for the purposes of selecting the dominant normal beattype)
    // whether it is normal or not and update the approporiate counts.

    if(morphType != 8)
    {

        // Update the buffers of previous beats and increment the
        // count for this beat type.

        DMBeatTypes[brIndex] = morphType ;
        ++DMBeatCounts[morphType] ;
        DMBeatRhythms[brIndex] = rhythmClass ;

        // If the rhythm appears regular, update the regular rhythm
        // count.

        if(rhythmClass == UNKNOWN)
            ++DMIrregCount ;

        // Check to see how many beats of this type have occurred in
        // a row (stop counting at six).

        i = brIndex - 1 ;
        if(i < 0) i += DM_BUFFER_LENGTH ;
        for(runCount = 0; (DMBeatTypes[i] == morphType) && (runCount < 6); ++runCount)
            if(--i < 0) i += DM_BUFFER_LENGTH ;

        // If the rhythm is regular, the beat width is less than 130 ms, and
        // there have been at least two in a row, consider the beat to be
        // normal.

        if((rhythmClass == NORMAL) && (beatWidth < BEAT_MS130) && (runCount >= 1))
        {
            DMBeatClasses[brIndex] = 1 ;
            ++DMNormCounts[morphType] ;
        }

            // If the last beat was within the normal P-R interval for this beat,
            // and the one before that was this beat type, assume the last beat
            // was noise and this beat is normal.

        else if(rr < ((FIDMARK-GetBeatBegin(morphType))*SAMPLE_RATE/BEAT_SAMPLE_RATE)
                && (oldType == morphType))
        {
            DMBeatClasses[brIndex] = 1 ;
            ++DMNormCounts[morphType] ;
        }

            // Otherwise assume that this is not a normal beat.

        else DMBeatClasses[brIndex] = 0 ;
    }

        // If the beat does not match any of the beat types, store
        // an indication that the beat does not match.

    else
    {
        DMBeatClasses[brIndex] = 0 ;
        DMBeatTypes[brIndex] = -1 ;
    }

    // Increment the index to the beginning of the circular buffers.

    if(++brIndex == DM_BUFFER_LENGTH)
        brIndex = 0 ;

    // Determine which beat type has the most beats that seem
    // normal.

    dom = 0 ;
    for(i = 1; i < 8; ++i)
        if(DMNormCounts[i] > DMNormCounts[dom])
            dom = i ;

    max = 0 ;
    for(i = 1; i < 8; ++i)
        if(DMBeatCounts[i] > DMBeatCounts[max])
            max = i ;

    // If there are no normal looking beats, fall back on which beat
    // has occurred most frequently since classification began.

    // Remarked by HMC
    //if((DMNormCounts[dom] == 0) || (DMBeatCounts[max]/DMBeatCounts[dom] >= 2))			// == 0
    //	dom = GetDominantType() ;

    bool HMCnewDomFnd;		// Added by HMC to prevent from divided-by-zero error
    HMCnewDomFnd = false;
    if( DMBeatCounts[dom] != 0 )
    {
        if((DMNormCounts[dom] == 0) || (DMBeatCounts[max]/DMBeatCounts[dom] >= 2))			// == 0
        {
            HMCnewDomFnd = true;
            dom = GetDominantType() ;
        }
    }

    // If at least half of the most frequently occuring normal
    // type do not seem normal, fall back on choosing the most frequently
    // occurring type since classification began.

    // Remarked by HMC
    //else if(DMBeatCounts[dom]/DMNormCounts[dom] >= 2)
    //	dom = GetDominantType() ;

    // Added by HMC
    if( HMCnewDomFnd == false && DMNormCounts[dom] != 0 )
    {	if(DMBeatCounts[dom]/DMNormCounts[dom] >= 2)
            dom = GetDominantType() ;
    }

    // If there is any beat type that has been classfied as normal,
    // but at least 10 don't seem normal, reclassify it to UNKNOWN.

    for(i = 0; i < 8; ++i)
        if((DMBeatCounts[i] > 10) && (DMNormCounts[i] == 0) && (i != dom)
           && (GetBeatClass(i) == NORMAL))
            SetBeatClass(i,UNKNOWN) ;

    // Save the dominant type in a global variable so that it is
    // accessable for debugging.

    NewDom = dom ;
    return(dom) ;
}

int CBeatClassify::GetNewDominantType(void)
{
    return(NewDom) ;
}

int CBeatClassify::GetDomRhythm(void)
{
    if(DMIrregCount > IRREG_RR_LIMIT)
        return(0) ;
    else return(1) ;
}


void CBeatClassify::AdjustDomData(int oldType, int newType)
{
    int i ;

    for(i = 0; i < DM_BUFFER_LENGTH; ++i)
    {
        if(DMBeatTypes[i] == oldType)
            DMBeatTypes[i] = newType ;
    }

    if(newType != MAXTYPES)
    {
        DMNormCounts[newType] = DMNormCounts[oldType] ;
        DMBeatCounts[newType] = DMBeatCounts[oldType] ;
    }

    DMNormCounts[oldType] = DMBeatCounts[oldType] = 0 ;

}

void CBeatClassify::CombineDomData(int oldType, int newType)
{
    int i ;

    for(i = 0; i < DM_BUFFER_LENGTH; ++i)
    {
        if(DMBeatTypes[i] == oldType)
            DMBeatTypes[i] = newType ;
    }

    if(newType != MAXTYPES)
    {
        DMNormCounts[newType] += DMNormCounts[oldType] ;
        DMBeatCounts[newType] += DMBeatCounts[oldType] ;
    }

    DMNormCounts[oldType] = DMBeatCounts[oldType] = 0 ;

}

/***********************************************************************
	GetRunCount() checks how many of the present beat type have occurred
	in a row.
***********************************************************************/

int CBeatClassify::GetRunCount()
{
    int i ;
    for(i = 1; (i < 8) && (RecentTypes[0] == RecentTypes[i]); ++i) ;
    return(i) ;
}

/************************************************************************
	GetNoiseEstimate() allows external access the present noise estimate.
	this function is only used for debugging.
*************************************************************************/

int CBeatClassify::GetNoiseEstimate()
{
    return(NoiseEstimate) ;
}

/***********************************************************************
	NoiseCheck() must be called for every sample of data.  The data is
	stored in a circular buffer to facilitate noise analysis.  When a
	beat is detected NoiseCheck() is passed the sample delay since the
	R-wave of the beat occurred (delay), the RR interval between this
	beat and the next most recent beat, the estimated offset from the
	R-wave to the beginning of the beat (beatBegin), and the estimated
	offset from the R-wave to the end of the beat.

	NoiseCheck() estimates the noise in the beat by the maximum and
	minimum signal values in either a window from the end of the
	previous beat to the beginning of the present beat, or a 250 ms
	window preceding the present beat, which ever is shorter.

	NoiseCheck() returns ratio of the signal variation in the window
	between beats to the length of the window between the beats.  If
	the heart rate is too high and the beat durations are too long,
	NoiseCheck() returns 0.

***********************************************************************/

int CBeatClassify::NoiseCheck(int datum, int delay, int RR, int beatBegin, int beatEnd)
{
    int ptr, i;
    int ncStart, ncEnd, ncMax, ncMin ;
    double noiseIndex ;

    NoiseBuffer[NBPtr] = datum ;
    if(++NBPtr == NB_LENGTH)
        NBPtr = 0 ;

    // Check for noise in region that is 300 ms following
    // last R-wave and 250 ms preceding present R-wave.

    ncStart = delay+RR-beatEnd ;	// Calculate offset to end of previous beat.
    ncEnd = delay+beatBegin ;		// Calculate offset to beginning of this beat.
    if(ncStart > ncEnd + MS250)
        ncStart = ncEnd + MS250 ;


    // Estimate noise if delay indicates a beat has been detected,
    // the delay is not to long for the data buffer, and there is
    // some space between the end of the last beat and the beginning
    // of this beat.

    if((delay != 0) && (ncStart < NB_LENGTH) && (ncStart > ncEnd))
    {

        ptr = NBPtr - ncStart ;	// Find index to end of last beat in
        if(ptr < 0)					// the circular buffer.
            ptr += NB_LENGTH ;

        // Find the maximum and minimum values in the
        // isoelectric region between beats.

        ncMax = ncMin = NoiseBuffer[ptr] ;
        for(i = 0; i < ncStart-ncEnd; ++i)
        {
            if(NoiseBuffer[ptr] > ncMax)
                ncMax = NoiseBuffer[ptr] ;
            else if(NoiseBuffer[ptr] < ncMin)
                ncMin = NoiseBuffer[ptr] ;
            if(++ptr == NB_LENGTH)
                ptr = 0 ;
        }

        // The noise index is the ratio of the signal variation
        // over the isoelectric window length, scaled by 10.

        noiseIndex = (ncMax-ncMin) ;
        noiseIndex /= (ncStart-ncEnd) ;
        NoiseEstimate = noiseIndex * 10 ;
    }
    else
        NoiseEstimate = 0 ;
    return(NoiseEstimate) ;
}
