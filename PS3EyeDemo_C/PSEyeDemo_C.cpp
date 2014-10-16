// OpenCVCam.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

// These three header files required by OpenCV 
#include <cv.h>
#include <cxcore.h>
#include <highgui.h>
#include "time.h"
#include "iostream"
// Header for the PSEye
#include "CLEyeMulticam.h"

#define FRAME_RATE 60

typedef struct{
	CLEyeCameraInstance CameraInstance;
	PBYTE FramePointer;
}CAMERA_AND_FRAME;

static DWORD WINAPI CaptureThread(LPVOID ThreadPointer);

int _tmain(int argc, _TCHAR* argv[])
{

	///////MY VARS////////
	PBYTE FramePointer=NULL;
	int width,height,CameraCount,FramerCounter=0;
	CLEyeCameraInstance EyeCamera=NULL;
	GUID CameraID;
	IplImage *frame;
	clock_t StartTime,EndTime;
	CAMERA_AND_FRAME ThreadPointer;
	HANDLE _hThread;
	//////////////////////

	//Check for presence of EYE
	CameraCount=CLEyeGetCameraCount();
	if(CameraCount>0) printf("Number of EYE's detected: %d\n\n",CameraCount);
	else{
		printf("No camera detected, press any key to exit...");
		getchar();
		return 0;
	}
	// Get ID of first PSEYE
	CameraID = CLEyeGetCameraUUID(0);
	// Get connection to camera and send it running parameters
	EyeCamera = CLEyeCreateCamera(CameraID,CLEYE_MONO_RAW,CLEYE_QVGA,FRAME_RATE);
	//Couldn't Connect to camera
	if(EyeCamera == NULL){
		printf("Couldn't connect to camera, press any key to exit...");
		getchar();
		return 0;
	}
	// Set some camera parameters;
	CLEyeSetCameraParameter(EyeCamera, CLEYE_EXPOSURE, 511);
	CLEyeSetCameraParameter(EyeCamera, CLEYE_GAIN, 0);
	// Get camera frame dimensions;
	CLEyeCameraGetFrameDimensions(EyeCamera, width, height);
	// Create a window in which the captured images will be presented
	cvNamedWindow( "Camera", CV_WINDOW_AUTOSIZE );
	//Make a image to hold the frames captured from the camera
	frame=cvCreateImage(cvSize(width ,height),IPL_DEPTH_8U, 1);
	// GetPointer To Image Data For frame
	cvGetImageRawData(frame,&FramePointer);
	//Start the eye camera
	CLEyeCameraStart(EyeCamera);	

	//Need to copy vars into one var to launch the second thread
	ThreadPointer.CameraInstance=EyeCamera;
	ThreadPointer.FramePointer=FramePointer;
	//Launch thread and confirm its running
	_hThread = CreateThread(NULL, 0, &CaptureThread, &ThreadPointer, 0, 0);
	if(_hThread == NULL)
	{
		printf("failed to create thread...");
		getchar();
		return false;
	}


	while( 1 ) {
		//Display the captured frame
		cvShowImage( "Camera", frame );
		//If ESC key pressed, Key=0x10001B under OpenCV 0.9.7(linux version),
		//remove higher bits using AND operator
		if( (cvWaitKey(1) & 255) == 27 ) break;
		//}
	}
	
	CLEyeCameraStop(EyeCamera);
	CLEyeDestroyCamera(EyeCamera);
	EyeCamera = NULL;
	cvDestroyWindow( "Camera" );

	return 0;
}

static DWORD WINAPI CaptureThread(LPVOID ThreadPointer){
	CAMERA_AND_FRAME *Instance=(CAMERA_AND_FRAME*)ThreadPointer;
	CLEyeCameraInstance Camera=Instance->CameraInstance;
	PBYTE FramePtr= Instance->FramePointer;
	int FramerCounter=0;
	clock_t StartTime,EndTime;
	while(1){
		//Get Frame From Camera
		CLEyeCameraGetFrame(Camera,FramePtr);

		// put the vision code

		// Track FPS
		if(FramerCounter==0) StartTime=clock();
		FramerCounter++;
		EndTime=clock();
		if((EndTime-StartTime)/CLOCKS_PER_SEC>=1)
		{
			printf("FPS: %d\n",FramerCounter);
			FramerCounter=0;
		}
	}
	return 0;
}

