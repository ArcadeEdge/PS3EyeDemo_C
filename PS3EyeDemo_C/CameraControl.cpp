//CameraControl.cpp

#include "stdafx.h"
#include "PSEyeDemo.h"

using namespace std;

CLEyeCameraInstance StartCam(int FrameRate,CLEyeCameraResolution Resolution){

	CLEyeCameraInstance EyeCamera;

	//Check for presence of EYE
	int CameraCount=CLEyeGetCameraCount();
	if(CameraCount>0) std::cout << "Number of EYE's detected: " << CameraCount << std::endl;
	else{
		std::cout << "No camera detected, press any key...";
		getchar();
		assert(CameraCount>0);
		return EyeCamera;
	}

	//For each camera get its unique id and add to stack and initialize
	GUID CameraID;

	//get ID
	CameraID = CLEyeGetCameraUUID(0);
	//init camera
	EyeCamera = CLEyeCreateCamera(CameraID,CLEYE_COLOR_RAW,Resolution,FrameRate);
	assert (EyeCamera!=NULL);
	//set camera paramas
	
	CLEyeSetCameraParameter(EyeCamera, CLEYE_AUTO_GAIN, 0);
	CLEyeSetCameraParameter(EyeCamera, CLEYE_AUTO_EXPOSURE, 0);
	CLEyeSetCameraParameter(EyeCamera, CLEYE_GAIN, 20);
	CLEyeSetCameraParameter(EyeCamera, CLEYE_AUTO_WHITEBALANCE, 1);
	

	//Start the eye camera
	CLEyeCameraStart(EyeCamera);

	return EyeCamera;
}

void StopCam(CLEyeCameraInstance & EyeCamera){
	CLEyeCameraStop(EyeCamera);
}

void NullFunction(int code,int x, int y,int flags,void *parameters){}
void UpdateCamParam(int code,int x, int y,int flags,void *parameters){
	CamAndParam *Input;

	Input=(CamAndParam*)parameters;

	CLEyeCameraParameter ParameterNum = Input->Parameter;
	CLEyeCameraInstance Camera = Input->Camera;

	if(code == CV_EVENT_LBUTTONDOWN || flags == CV_EVENT_FLAG_LBUTTON){
		CLEyeSetCameraParameter(Camera,ParameterNum,CLEyeGetCameraParameter(Camera,ParameterNum)-1);
	}

	if(code == CV_EVENT_RBUTTONDOWN || flags == CV_EVENT_FLAG_RBUTTON){
		CLEyeSetCameraParameter(Camera,ParameterNum,CLEyeGetCameraParameter(Camera,ParameterNum)+1);
	}

}
void SaveCameraParameters(const CLEyeCameraInstance &LeftCam){
	ofstream Output;
	Output.open("Camera.param");
	for(int i=0;i<20;i++){
		Output << CLEyeGetCameraParameter(LeftCam,(CLEyeCameraParameter)i) << endl;
	}
	Output.close();
	cout << "Saved...\n" << endl;
}
void LoadCameraParameters(const CLEyeCameraInstance &LeftCam){
	ifstream Input;
	int Value;
	Input.open("Camera.param");
	for(int i=0;i<20;i++){
		if(Input.eof()) break;
		Input >> Value;
		CLEyeSetCameraParameter(LeftCam,(CLEyeCameraParameter)i,Value);
	}
	Input.close();
}