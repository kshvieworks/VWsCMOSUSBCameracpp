#pragma once
#include "VwUSB.Global.h"
#include "opencv2/opencv.hpp"

#ifndef VWCAM_H
#define VWCAM_H

namespace VWSDK
{
	enum PIXEL_FORMAT;
	class VwUSB;
	class VwUSBCamera;
	struct OBJECT_INFO;
	struct IMAGE_INFO;
	struct DISCONNECT_INFO;
	enum RESULT;
};

class VwCAM
{
public:
	VwCAM();
	VWSDK::RESULT GetCustomCommand(VWSDK::VwUSBCamera* phCamera, char* cpFeatureName, UINT* unValue, VWSDK::GET_CUSTOM_COMMAND eCmdType = VWSDK::GET_CUSTOM_COMMAND_VALUE);
	void BmpInit();
	BOOL OpenCamera();
	void MakeUnPackedBuffer();
	BOOL Grab();
	BOOL ConvertPixelFormat(VWSDK::PIXEL_FORMAT ePixelFormat, BYTE* pDest, BYTE* pSource, int nWidth, int nHeight);
	int GetPixelBitCount(VWSDK::PIXEL_FORMAT ePixelFormat);
	void CloneImage(cv::Mat Img);
	cv::Mat GetImage();

	static void DrawImage(VWSDK::OBJECT_INFO* pObjectInfo, VWSDK::IMAGE_INFO* pImageInfo);
	static void Disconnect(VWSDK::OBJECT_INFO* pObjectInfo, VWSDK::DISCONNECT_INFO tDisconnect);

protected:

	// USB Connection
	VWSDK::VwUSB* hVwUSB;
	VWSDK::RESULT ret_Open;
	VWSDK::RESULT ret_Discovery;

	UINT nCameraNum;
	VWSDK::RESULT ret_CameraNum;

	// Image Acquisition
	// Declaration of Variables
	VWSDK::VwUSBCamera* _pCamera;
	//HDC _hScreen;
	//HDC _hdc;
	char _szName[128];
	BYTE* _pUnpackedImage;
	BITMAPINFO* _pBmpInfo;
	VWSDK::OBJECT_INFO* _pObjectInfo;

	const int BUFFER_SIZE = 10;
	UINT _imagecontrolHeight;
	UINT _imagecontrolWidth;
	cv::Mat CVImageArray;
};
#endif