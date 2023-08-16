//#include "VwResourceType.h"
#include "stdafx.h"
#include "VWSCmosUSBCam.h"
#include "VwUSB.h"
#include "VwUSBCamera.h"
#include "VwImageProcess.h"
//#include "VwSDK.h"

//#include <list>
#include <iostream>
#include <algorithm>
#include <vector>
#include <conio.h>
#include <thread>
#include "opencv2/opencv.hpp"
#include "gnuplot-iostream.h"
#include "boost/tuple/tuple.hpp"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


VwCAM::VwCAM()
{
	this->hVwUSB = new VWSDK::VwUSB;
	this->ret_Open = this->hVwUSB->Open();
	this->ret_Discovery = this->hVwUSB->Discovery();

	this->nCameraNum = 0;
	this->ret_CameraNum = this->hVwUSB->GetNumCameras(&this->nCameraNum);

	for (int i = 0; i < (int)(this->nCameraNum); i++)
	{
		VWSDK::USB::VWCAMERA_INFO CameraInfo;
		VWSDK::RESULT reth = this->hVwUSB->GetCameraInfo(i, &CameraInfo);
		strcpy_s(this->_szName, 128, CameraInfo.name);

		/*int nMultiByteLen = WideCharToMultiByte(CP_ACP, 0, (LPCWCH)CameraInfo.name, -1, NULL, 0, NULL, NULL);
		WideCharToMultiByte(CP_ACP, 0, (LPCWCH)CameraInfo.name, -1, this->_szName, nMultiByteLen, NULL, NULL);*/

	}
	//strcpy_s(this->_szName, 128, this->_szName);

	this->_pCamera = NULL;
	this->_pObjectInfo = new VWSDK::OBJECT_INFO;
	this->_pUnpackedImage = NULL;
	this->_pBmpInfo = (BITMAPINFO*)new BYTE[(sizeof(BITMAPINFOHEADER) * 256 * sizeof(RGBQUAD))];

	this->_imagecontrolHeight = 0;
	this->_imagecontrolWidth = 0;

	//this->_hScreen = GetDC(NULL);
	//this->_hdc = CreateCompatibleDC(this->_hScreen);

	BmpInit();
}

void VwCAM::CloneImage(cv::Mat Img)
{
	CVImageArray = Img.clone();
}

cv::Mat VwCAM::GetImage()
{
	return CVImageArray;
}


VWSDK::RESULT VwCAM::GetCustomCommand(VWSDK::VwUSBCamera* phCamera, char* cpFeatureName, UINT* unValue, VWSDK::GET_CUSTOM_COMMAND eCmdType)
{
	VWSDK::RESULT eRet = VWSDK::RESULT_ERROR;

	char chResult[100] = { 0, };
	size_t szResult = sizeof(chResult);

	eRet = phCamera->GetCustomCommand(cpFeatureName, chResult, &szResult, eCmdType);
	if (eRet == VWSDK::RESULT_SUCCESS)
		*unValue = atoi(chResult);
	
	return eRet;
}

void VwCAM::DrawImage(VWSDK::OBJECT_INFO* pObjectInfo, VWSDK::IMAGE_INFO* pImageInfo)
{

	VwCAM* self = (VwCAM*)pObjectInfo->pUserPointer;
	
	UINT unBitCount = 0;
	UINT unbiClrUsed = 0;
	int nCalcHeight = 0;
	void* pBuf = NULL;

	UINT unHeight = pImageInfo->height;
	UINT unWidth = pImageInfo->width;
	UINT unBufIdx = pImageInfo->bufferIndex;
	void* vpBuffer = pImageInfo->pImage;
	VWSDK::PIXEL_FORMAT ePixelFormat = pImageInfo->pixelFormat;

	self->GetCustomCommand((VWSDK::VwUSBCamera*)pObjectInfo->pVwCamera, "Width", &unWidth);
	self->GetCustomCommand((VWSDK::VwUSBCamera*)pObjectInfo->pVwCamera, "Width", &unHeight);
	((VWSDK::VwUSBCamera*)pObjectInfo->pVwCamera)->GetPixelFormat(&ePixelFormat);

	if (VWSDK::PIXEL_FORMAT_MONO8 == ePixelFormat)
	{
		nCalcHeight = (-1 * unHeight);
		unBitCount = 8;
		unbiClrUsed = 256;
	}
	else
	{
		nCalcHeight = (-1 * unHeight);
		unBitCount = 24;
		unbiClrUsed = 0;
	}

	unBitCount = self->GetPixelBitCount(ePixelFormat);

	self->_pBmpInfo->bmiHeader.biWidth = unWidth;
	self->_pBmpInfo->bmiHeader.biHeight = nCalcHeight;
	self->_pBmpInfo->bmiHeader.biBitCount = unBitCount;
	self->_pBmpInfo->bmiHeader.biCompression = BI_RGB;
	self->_pBmpInfo->bmiHeader.biClrUsed = unbiClrUsed;

	BOOL bRet = self->ConvertPixelFormat(ePixelFormat, self->_pUnpackedImage, (BYTE*)vpBuffer, unWidth, unHeight);

	pBuf = self->_pUnpackedImage;

	//UINT unWidthPos = 0;
	//UINT unHeightPos = 0;
	//
	//::SetStretchBltMode(self->_hdc, COLORONCOLOR);
	//StretchDIBits(self->_hdc, unWidthPos, unHeightPos, self->_imagecontrolWidth, self->_imagecontrolHeight, 0, 0, unWidth, unHeight, pBuf, self->_pBmpInfo, DIB_RGB_COLORS, SRCCOPY);

	cv::Mat Image_Array = cv::Mat(unHeight, unWidth, CV_8UC3, pBuf);
	self->CloneImage(Image_Array);
	cv::imshow("image_array", Image_Array);
	cv::waitKey(1);
	int asd = 1;

}

void VwCAM::Disconnect(VWSDK::OBJECT_INFO* pObjectInfo, VWSDK::DISCONNECT_INFO tDisconnect)
{
	VwCAM* self = (VwCAM*)pObjectInfo->pUserPointer;
	std::cout << tDisconnect.nCurrHeartBeatTimeOut << tDisconnect.nTimeOutTryCount << endl;
}


void VwCAM::BmpInit()
{
	ZeroMemory(_pBmpInfo, sizeof(BITMAPINFOHEADER));

	_pBmpInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	_pBmpInfo->bmiHeader.biPlanes = 1;
	_pBmpInfo->bmiHeader.biCompression = BI_RGB;
	_pBmpInfo->bmiHeader.biClrImportant = 0;
	_pBmpInfo->bmiHeader.biBitCount = 8;
	_pBmpInfo->bmiHeader.biClrUsed = 256;

	for (UINT i = 0; i < _pBmpInfo->bmiHeader.biClrUsed; i++)
	{
		_pBmpInfo->bmiColors[i].rgbBlue = i;
		_pBmpInfo->bmiColors[i].rgbGreen = i;
		_pBmpInfo->bmiColors[i].rgbRed = i;
		_pBmpInfo->bmiColors[i].rgbReserved = 0;
	}
}

void VwCAM::MakeUnPackedBuffer()
{
	if (_pUnpackedImage)
	{
		delete[] _pUnpackedImage;
		_pUnpackedImage = NULL;
	}

	UINT nWidth = 0;
	UINT nHeight = 0;
	VWSDK::PIXEL_FORMAT pixelFormat = VWSDK::PIXEL_FORMAT_MONO8;

	GetCustomCommand(_pCamera, "Width", &nWidth);
	GetCustomCommand(_pCamera, "Height", &nHeight);
	_pCamera->GetPixelFormat(&pixelFormat);

	_pUnpackedImage = new BYTE[(size_t)(nWidth) * (size_t)(nHeight) * 3];
	int asdf = 1;
}

BOOL VwCAM::OpenCamera()
{
	if (NULL == this->hVwUSB)
	{
		//assert(this->hVwUSB);
		return FALSE;
	}

	VWSDK::RESULT ret = VWSDK::RESULT_ERROR;
	ret = this->hVwUSB->OpenCamera( this->_szName, &(this->_pCamera), this->BUFFER_SIZE, 0, 0, 0, this->_pObjectInfo, DrawImage, NULL);

	if (ret != VWSDK::RESULT_SUCCESS)
		return FALSE;

	this->_pObjectInfo->pUserPointer = this;
	this->_pObjectInfo->pVwCamera = this->_pCamera;
	MakeUnPackedBuffer();

	return TRUE;

}

BOOL VwCAM::Grab()
{
	if (NULL == this->_pCamera)
	{
		return FALSE;
	}

	BOOL bGrabbing = FALSE;

	//VWSDK::RESULT ret_here = VWSDK::RESULT_ERROR;
	//int pCount;
	//ret_here = this->_pCamera->GetPropertyCount(&pCount);

	this->_pCamera->GetGrabCondition(bGrabbing);
	//DWORD ret = this->_pCamera->WaitGrabThread(1000);
	//VWSDK::RESULT rett = this->_pCamera->SetImageCallback(DrawImage);
	if(this->_pCamera->Grab() == VWSDK::RESULT_SUCCESS)
	
		return TRUE;
	return FALSE;
}


BOOL VwCAM::ConvertPixelFormat(VWSDK::PIXEL_FORMAT ePixelFormat, BYTE* pDest, BYTE* pSource, int nWidth, int nHeight)
{
	if (NULL == pDest ||
		NULL == pSource)
	{
		return FALSE;
	}

	if (0 == nWidth || 0 == nHeight)
	{
		return FALSE;
	}

	BOOL bRet = TRUE;
	BYTE* bpConvertPixelFormat = new BYTE[nWidth * nHeight * 2];

	switch (ePixelFormat)
	{
		//-----------------------------------------------------------------
		// about MONO Pixel Format Series ---------------------------------
		//-----------------------------------------------------------------
	case VWSDK::PIXEL_FORMAT_MONO8:
		memcpy(pDest, pSource, nWidth * nHeight);
		break;
	case VWSDK::PIXEL_FORMAT_MONO10:
		VWSDK::VwImageProcess::ConvertMono10ToBGR8(PBYTE(pSource), nWidth * nHeight * 2, pDest);
		break;
	case VWSDK::PIXEL_FORMAT_MONO12:
		VWSDK::VwImageProcess::ConvertMono12ToBGR8(PBYTE(pSource), nWidth * nHeight * 2, pDest);
		break;
	case VWSDK::PIXEL_FORMAT_MONO10_PACKED:
	case VWSDK::PIXEL_FORMAT_MONO12_PACKED:
		VWSDK::VwImageProcess::ConvertMonoPackedToBGR8(pSource,
			UINT(1.5 * nWidth * nHeight),
			pDest);
		break;

	case VWSDK::PIXEL_FORMAT_MONO14:
		VWSDK::VwImageProcess::ConvertMono14ToBGR8(PBYTE(pSource), nWidth * nHeight * 2, pDest);
		break;

	case VWSDK::PIXEL_FORMAT_MONO16:
		VWSDK::VwImageProcess::ConvertMono16PackedToBGR8(pSource,
			UINT(2 * nWidth * nHeight),
			pDest);
		break;
		//-----------------------------------------------------------------
		// about BAYER Pixel Format Series --------------------------------
		//-----------------------------------------------------------------
	case VWSDK::PIXEL_FORMAT_BAYGR8:
		VWSDK::VwImageProcess::ConvertBAYGR8ToBGR8(pSource,
			pDest,
			nWidth,
			nHeight);
		break;

		/// Delete::20150629, Dong-Uk Lee, Basler USB 카메라 테스트용
		// 	case VWSDK::PIXEL_FORMAT_BAYBG8:
		// 		ConvertBAYBG8ToBGR8( pSource,
		// 											pDest,
		// 											nWidth,
		// 											nHeight );
		// 		break;
		/// Delete::20150629, Dong-Uk Lee, 
		/// 
	case VWSDK::PIXEL_FORMAT_BAYRG8:
		VWSDK::VwImageProcess::ConvertBAYRG8ToBGR8(pSource,
			pDest,
			nWidth,
			nHeight);
		break;
	case VWSDK::PIXEL_FORMAT_BAYGR10:
		VWSDK::VwImageProcess::ConvertBAYGR10ToBGR8((WORD*)(pSource),
			pDest,
			nWidth,
			nHeight);
		break;
	case VWSDK::PIXEL_FORMAT_BAYRG10:
		VWSDK::VwImageProcess::ConvertBAYRG10ToBGR8((WORD*)(pSource),
			pDest,
			nWidth,
			nHeight);
		break;
	case VWSDK::PIXEL_FORMAT_BAYGR12:
		VWSDK::VwImageProcess::ConvertBAYGR12ToBGR8((WORD*)(pSource),
			pDest,
			nWidth,
			nHeight);
		break;
	case VWSDK::PIXEL_FORMAT_BAYRG12:
		VWSDK::VwImageProcess::ConvertBAYRG12ToBGR8((WORD*)(pSource),
			pDest,
			nWidth,
			nHeight);
		break;
	case VWSDK::PIXEL_FORMAT_BAYGR10_PACKED:
		VWSDK::VwImageProcess::ConvertMono10PackedToMono16bit((PBYTE)pSource,
			nWidth,
			nHeight,
			bpConvertPixelFormat);
		VWSDK::VwImageProcess::ConvertBAYGR10ToBGR8((WORD*)bpConvertPixelFormat,
			pDest,
			nWidth,
			nHeight);
		break;
	case VWSDK::PIXEL_FORMAT_BAYRG10_PACKED:
		VWSDK::VwImageProcess::ConvertMono10PackedToMono16bit((PBYTE)pSource,
			nWidth,
			nHeight,
			bpConvertPixelFormat);
		VWSDK::VwImageProcess::ConvertBAYRG10ToBGR8((WORD*)bpConvertPixelFormat,
			pDest,
			nWidth,
			nHeight);
		break;
	case VWSDK::PIXEL_FORMAT_BAYGR12_PACKED:
		VWSDK::VwImageProcess::ConvertMono12PackedToMono16bit((PBYTE)pSource,
			nWidth,
			nHeight,
			bpConvertPixelFormat);
		VWSDK::VwImageProcess::ConvertBAYGR12ToBGR8((WORD*)bpConvertPixelFormat,
			pDest,
			nWidth,
			nHeight);
		break;
	case VWSDK::PIXEL_FORMAT_BAYRG12_PACKED:
		VWSDK::VwImageProcess::ConvertMono12PackedToMono16bit((PBYTE)pSource,
			nWidth,
			nHeight,
			bpConvertPixelFormat);
		VWSDK::VwImageProcess::ConvertBAYRG12ToBGR8((WORD*)bpConvertPixelFormat,
			pDest,
			nWidth,
			nHeight);
		break;
	case VWSDK::PIXEL_FORMAT_RGB8:
		VWSDK::VwImageProcess::ConvertRGB8ToBGR8((PBYTE)pSource,
			UINT(3 * nWidth * nHeight),
			pDest);
		break;
	case VWSDK::PIXEL_FORMAT_BGR8:
		bRet = FALSE;
		break;
	case VWSDK::PIXEL_FORMAT_RGB12_PACKED:
		VWSDK::VwImageProcess::ConvertRGB12PackedToBGR8((PBYTE)pSource,
			UINT(6 * nWidth * nHeight),
			pDest);
		break;
	case VWSDK::PIXEL_FORMAT_BGR12_PACKED:
		bRet = FALSE;
		break;
	case VWSDK::PIXEL_FORMAT_YUV411:
		VWSDK::VwImageProcess::ConvertYUV411ToBGR8((PBYTE)pSource,
			UINT(1.5 * nWidth * nHeight),
			pDest);
		break;
	case VWSDK::PIXEL_FORMAT_YUV422_UYVY:
		VWSDK::VwImageProcess::ConvertYUV422_UYVYToBGR8((PBYTE)pSource,
			nWidth,
			nHeight,
			pDest);
		break;
	case VWSDK::PIXEL_FORMAT_YUV422_YUYV:
		VWSDK::VwImageProcess::ConvertYUV422_YUYVToBGR8((PBYTE)pSource,
			nWidth,
			nHeight,
			pDest);
		break;
	case VWSDK::PIXEL_FORMAT_YUV444:
		VWSDK::VwImageProcess::ConvertYUV444ToBGR8((PBYTE)pSource,
			UINT(1.5 * nWidth * nHeight),
			pDest);
		break;
	case VWSDK::PIXEL_FORMAT_BGR10V1_PACKED:
		bRet = FALSE;
		break;
	case VWSDK::PIXEL_FORMAT_YUV411_10_PACKED:
	case VWSDK::PIXEL_FORMAT_YUV411_12_PACKED:
		VWSDK::VwImageProcess::ConvertYUV411PackedToBGR8((PBYTE)pSource,
			UINT(2.25 * nWidth * nHeight),
			pDest);
		break;
	case VWSDK::PIXEL_FORMAT_YUV422_10_PACKED:
	case VWSDK::PIXEL_FORMAT_YUV422_12_PACKED:
		VWSDK::VwImageProcess::ConvertYUV422PackedToBGR8((PBYTE)pSource,
			UINT(3 * nWidth * nHeight),
			pDest);
		break;
	case VWSDK::PIXEL_FORMAT_PAL_INTERLACED:
	case VWSDK::PIXEL_FORMAT_NTSC_INTERLACED:
		break;
	default:
	{
		bRet = FALSE;
	}
	}

	if (NULL != bpConvertPixelFormat)
	{
		delete[] bpConvertPixelFormat;
	}

	return bRet;
}

int VwCAM::GetPixelBitCount( VWSDK::PIXEL_FORMAT ePixelFormat )
{
	int nRet = 0;
	switch( ePixelFormat )
	{
		//-----------------------------------------------------------------
		// about MONO Pixel Format Series ---------------------------------
		//-----------------------------------------------------------------
	case VWSDK::PIXEL_FORMAT_MONO8:
		nRet	=	8;
		break;
	case VWSDK::PIXEL_FORMAT_MONO10:
	case VWSDK::PIXEL_FORMAT_MONO12:
	case VWSDK::PIXEL_FORMAT_MONO10_PACKED:
	case VWSDK::PIXEL_FORMAT_MONO12_PACKED:
	case VWSDK::PIXEL_FORMAT_MONO14:
	case VWSDK::PIXEL_FORMAT_MONO16:
		nRet	=	24;
		break;
		//-----------------------------------------------------------------
		// about BAYER Pixel Format Series --------------------------------
		//-----------------------------------------------------------------
	case VWSDK::PIXEL_FORMAT_BAYGR8:
	case VWSDK::PIXEL_FORMAT_BAYRG8:
	case VWSDK::PIXEL_FORMAT_BAYGR10:
	case VWSDK::PIXEL_FORMAT_BAYRG10:
	case VWSDK::PIXEL_FORMAT_BAYGR12:
	case VWSDK::PIXEL_FORMAT_BAYRG12:
	case VWSDK::PIXEL_FORMAT_BAYGR10_PACKED:
	case VWSDK::PIXEL_FORMAT_BAYRG10_PACKED:
	case VWSDK::PIXEL_FORMAT_BAYGR12_PACKED:
	case VWSDK::PIXEL_FORMAT_BAYRG12_PACKED:
		nRet	=	24;
		break;
	case VWSDK::PIXEL_FORMAT_RGB8:
	case VWSDK::PIXEL_FORMAT_BGR8:
	case VWSDK::PIXEL_FORMAT_RGB12_PACKED:
	case VWSDK::PIXEL_FORMAT_BGR12_PACKED:
		nRet	=	24;
		break;
	case VWSDK::PIXEL_FORMAT_BGR10V2_PACKED:
		nRet	=	24;
		break;
	case VWSDK::PIXEL_FORMAT_YUV411:
	case VWSDK::PIXEL_FORMAT_YUV422_UYVY:
	case VWSDK::PIXEL_FORMAT_YUV422_YUYV:
	case VWSDK::PIXEL_FORMAT_YUV444:
		nRet	=	24;
		break;
	case VWSDK::PIXEL_FORMAT_BGR10V1_PACKED:
		nRet	=	32;
		break;
	case VWSDK::PIXEL_FORMAT_YUV411_10_PACKED:
	case VWSDK::PIXEL_FORMAT_YUV411_12_PACKED:
	case VWSDK::PIXEL_FORMAT_YUV422_10_PACKED:
	case VWSDK::PIXEL_FORMAT_YUV422_12_PACKED:
		nRet	=	24;
		break;
	case VWSDK::PIXEL_FORMAT_PAL_INTERLACED:
	case VWSDK::PIXEL_FORMAT_NTSC_INTERLACED:
		nRet	=	24;
		break;
	default:
		{
		}
	}

	return nRet;
}

void keyboard_interrupt()
{
	while (true)
	{
		char input = getch();
		if (input == 27)
			break;
	}
}

void thread_handler(VwCAM* hVwCAM, cv::Mat hVwCAM_Image)
{
	while (true)
	{
		hVwCAM_Image = hVwCAM->GetImage();
		cv::imshow("img", hVwCAM_Image);
		cv::waitKey();
	}
}
