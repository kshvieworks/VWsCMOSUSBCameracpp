#include "stdafx.h"
#include "VWSCmosUSBCam.h"
#include <conio.h>

int main()
{
	// USB Connection
	VwCAM* hVwCAM;
	hVwCAM = new VwCAM();
	BOOL ret_CamOpen = hVwCAM->OpenCamera();
	BOOL ret_CamGrab = hVwCAM->Grab();

	cv::Mat hVwCAM_Image;
	char input = getch();

	while (true)
	{
		hVwCAM_Image = hVwCAM->GetImage();
		cv::imshow("img", hVwCAM_Image);
		cv::waitKey(1);
	}
	//
	//thread t(thread_handler, hVwCAM, hVwCAM_Image);
	//keyboard_interrupt();
	//t.join();

	return 0;
}