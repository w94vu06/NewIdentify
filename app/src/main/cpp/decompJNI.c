//
// Created by User on 12/11/2017.
//

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "lz4.h"
#include <jni.h>

#define ECG_COMP_BLOCK_SIZE		544

enum {
	MESSAGE_MAX_BYTES = ECG_COMP_BLOCK_SIZE,
	RING_BUFFER_BYTES = ECG_COMP_BLOCK_SIZE * 8 + MESSAGE_MAX_BYTES,
	DECODE_RING_BUFFER = RING_BUFFER_BYTES + MESSAGE_MAX_BYTES   // Intentionally larger, to test unsynchronized ring buffers
};

#define CMPBUFSIZE (LZ4_COMPRESSBOUND(MESSAGE_MAX_BYTES))
#define FILE_NAME_LEN_MAX   4096    // The max length of the file path

JNIEXPORT jint
JNICALL Java_com_example_newidentify_MainActivity_decpEcgFile
        (JNIEnv *env, jobject jobj, jstring path){
    char * lpszPath;

    lpszPath = (*env)->GetStringUTFChars( env, path , NULL ) ;

    char * pDotSubName;

    char CompFileName[FILE_NAME_LEN_MAX];   // The file name of the compressed ECG file
    char DecFileName[FILE_NAME_LEN_MAX];    // The file name of the decompressed ECG file

    strcpy( CompFileName, lpszPath );
    pDotSubName = strrchr(CompFileName, '.');
    strcpy( pDotSubName, ".lp4");

    strcpy( DecFileName, lpszPath );
    pDotSubName = strrchr(DecFileName, '.');
    strcpy( pDotSubName, ".cha");

    FILE * pReadFile;	// 來源心電圖檔
    FILE * pOut1;		// 轉換後的心電圖檔

                        // 開啟來源心電圖檔
    pReadFile = fopen(CompFileName, "r+b");
    if (pReadFile == NULL)
    {
        printf("%s open failed.\n", CompFileName);
        return EXIT_FAILURE;
    }

    // 開啟轉換後的心電圖檔
    pOut1 = fopen(DecFileName, "wb");
    if (pOut1 == NULL)
    {
        printf("%s open failed.\n", DecFileName);
        return EXIT_FAILURE;
    }

    unsigned int nBytesReceived;
    unsigned int nBytesWritten;
    int nBytesDecomp;	// 一個Frame解壓縮後的大小
    int fi, fi2, fi3;
    char compBuff[CMPBUFSIZE];
    static char decompRingBuf[DECODE_RING_BUFFER];
    int   decOffset = 0;
    LZ4_streamDecode_t lz4StreamDecode_body = { 0 };
    LZ4_streamDecode_t* lz4StreamDecode = &lz4StreamDecode_body;

    while (1)
    {
        // 實際讀取來源心電圖壓縮檔資料
        short frameCompSize;	// 壓縮檔中的一個Frame大小
        nBytesReceived = fread(&frameCompSize, 1, 2, pReadFile);
        if (nBytesReceived != 2)
            break;

        nBytesReceived = fread(compBuff, 1, frameCompSize, pReadFile);
        if (nBytesReceived != frameCompSize)
            break;

        //nBytesDecomp = LZ4_decompress_safe(compBuff, decompBuff, frameCompSize, ECG_COMP_BLOCK_SIZE);
        //nBytesDecomp = LZ4_decompress_fast(compBuff, decompBuff, frameCompSize);
        //nBytesDecomp = LZ4_decompress_fast(compBuff, decompBuff, ECG_COMP_BLOCK_SIZE);

        char* const decPtr = &decompRingBuf[decOffset];

        const int decBytes = LZ4_decompress_safe_continue(
            lz4StreamDecode, compBuff, decPtr, frameCompSize, MESSAGE_MAX_BYTES);
        if (decBytes <= 0)
            break;

        // 輸出檔案
        nBytesWritten = fwrite(decPtr, 1, decBytes, pOut1);
        if (nBytesWritten != decBytes)
            break;

        decOffset += decBytes;

        //const int decBytes = LZ4_decompress_fast_continue(
        //	lz4StreamDecode, compBuff, decPtr, ECG_COMP_BLOCK_SIZE);
        //if (decBytes <= 0)
        //	break;

        // 輸出檔案
        //nBytesWritten = fwrite(decPtr, 1, ECG_COMP_BLOCK_SIZE, pOut1);
        //if (nBytesWritten != ECG_COMP_BLOCK_SIZE)
        //	break;

        //decOffset += ECG_COMP_BLOCK_SIZE;

        // Wraparound the ringbuffer offset
        if (decOffset >= DECODE_RING_BUFFER - MESSAGE_MAX_BYTES)
            decOffset = 0;
    }

    // 關閉檔案
    if (pReadFile != NULL)
    {
        fclose(pReadFile);
        pReadFile = NULL;
    }
    if (pOut1 != NULL)
    {
        fclose(pOut1);
        pOut1 = NULL;
    }

    return 0;
}


JNIEXPORT jint JNICALL
Java_com_example_newidentify_LoginActivity_decpEcgFile(JNIEnv *env, jobject thiz, jstring path) {
    // TODO: implement decpEcgFile()
}