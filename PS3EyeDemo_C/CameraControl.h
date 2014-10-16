//CameraControl.h
//#include "stdafx.h"
#include "CLEyeMulticam.h"

using namespace std;

const string CamParamNames[20]=
{"CLEYE_AUTO_GAIN",
"CLEYE_GAIN",
"CLEYE_AUTO_EXPOSURE",
"CLEYE_EXPOSURE",
"CLEYE_AUTO_WHITEBALANCE",
"CLEYE_WHITEBALANCE_RED",
"CLEYE_WHITEBALANCE_GREEN",
"CLEYE_WHITEBALANCE_BLUE",
"CLEYE_HFLIP",
"CLEYE_VFLIP",
"CLEYE_HKEYSTONE",
"CLEYE_VKEYSTONE",
"CLEYE_XOFFSET"	,
"CLEYE_YOFFSET"	,
"CLEYE_ROTATION",
"CLEYE_ZOOM",	
"CLEYE_LENSCORRECTION1",
"CLEYE_LENSCORRECTION2",
"CLEYE_LENSCORRECTION3",
"CLEYE_LENSBRIGHTNESS"};


//Function declarations in CameraControl.cpp
CLEyeCameraInstance StartCam(int FrameRate,CLEyeCameraResolution Resolution);
void StopCam(CLEyeCameraInstance & EyeCamera);
void UpdateCamParam(int code,int x, int y,int flags,void *parameters);
void SaveCameraParameters(const CLEyeCameraInstance &LeftCam);
void LoadCameraParameters(const CLEyeCameraInstance &LeftCam);
void NullFunction(int code,int x, int y,int flags,void *parameters);

struct CamAndParam{
	CLEyeCameraInstance Camera;
	CLEyeCameraParameter Parameter;
};