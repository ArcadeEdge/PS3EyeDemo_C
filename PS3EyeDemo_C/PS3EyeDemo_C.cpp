// OpenCVCam.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

// These three header files required by OpenCV 
#include <cv.h>
#include <cxcore.h>
#include <highgui.h>
#include "time.h"
#include "iostream"
#include <Windows.h>
// Header for the PSEye
#include "CLEyeMulticam.h"

#define FRAME_RATE		75
#define FRAME_SIZE		CLEYE_VGA
#define FRAME_FORMAT	CLEYE_COLOR_RAW

IplImage *frame;

typedef struct{
	CLEyeCameraInstance CameraInstance;
	PBYTE FramePointer;
}CAMERA_AND_FRAME;

static DWORD WINAPI CaptureThread(LPVOID ThreadPointer);


//***************************************************************
// Slider for the low threshold value of our edge detection
int maxLowThreshold = 1024;
int maxLowCanny = 1024;
int lowSliderPosT = 150;
int lowSliderPosC = 150;
int maxHighThreshold = 1024;
int maxHighCanny = 1024;
int highSliderPosT = 250;
int highSliderPosC = 250;
int maxGain=10, gainPosition=1;
int maxExposure = 900,expPosition=511;

// Callback function to adjust values for threshold
void onLowThresholdSlide(int theSliderValue)
{lowSliderPosT = theSliderValue;}
void onHighThresholdSlide(int theSliderValue)
{highSliderPosT = theSliderValue;}

// Callback function to adjust values for Canny Edge Detection
void onHighCannySlide(int theSliderValue)
{highSliderPosC = theSliderValue;}
void onLowCannySlide(int theSliderValue)
{lowSliderPosC = theSliderValue;}
 
void GainSlider(int theSliderValue)
{ gainPosition = theSliderValue;}
void ExpSlider(int theSliderValue)
{ expPosition = theSliderValue;}

//*******************************************************************


int _tmain(int argc, _TCHAR* argv[])
{

	///////MY VARS////////
	PBYTE FramePointer=NULL;
	int width,height,CameraCount,FramerCounter=0;
	CLEyeCameraInstance EyeCamera=NULL;
	GUID CameraID;
	//IplImage *frame;
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
	EyeCamera = CLEyeCreateCamera(CameraID,FRAME_FORMAT,FRAME_SIZE,FRAME_RATE);
	//Couldn't Connect to camera
	if(EyeCamera == NULL){
		printf("Couldn't connect to camera, press any key to exit...");
		getchar();
		return 0;
	}
	cvNamedWindow( "Camera", CV_WINDOW_AUTOSIZE );
	// Format: Slider name, window name, reference to variable for slider, max value of slider, callback function
	/*
	cvCreateTrackbar("Exposure", "Camera", &expPosition, expPosition, ExpSlider);
	cvCreateTrackbar("Gain", "Camera", &gainPosition, maxGain, GainSlider);
	*/

	// Set some camera parameters;
	CLEyeSetCameraParameter(EyeCamera, CLEYE_EXPOSURE, 511); //default 511
	CLEyeSetCameraParameter(EyeCamera, CLEYE_GAIN, 0); //default 0
	// Get camera frame dimensions;
	CLEyeCameraGetFrameDimensions(EyeCamera, width, height);
	// Create a window in which the captured images will be presented
	//cvNamedWindow( "Camera", CV_WINDOW_AUTOSIZE );
	//Make a image to hold the frames captured from the camera
	frame=cvCreateImage(cvSize(width ,height),IPL_DEPTH_8U, 4);
	// GetPointer To Image Data For frame
	cvGetImageRawData(frame,&FramePointer);
	//Start the eye camera
	CLEyeCameraStart(EyeCamera);	


	/*
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
	*/


		if(FRAME_RATE>60){
			//Need to copy vars into one var to launch the second thread
			ThreadPointer.CameraInstance=EyeCamera;
			ThreadPointer.FramePointer = FramePointer;
			//ThreadPointer.Threshold=0;
			//Launch thread and confirm its running
			_hThread = CreateThread(NULL, 0, &CaptureThread, &ThreadPointer, 0, 0);
			if(_hThread == NULL){
				printf("failed to create thread...");
				getchar();
				return false;
			}
		}

	while( 1 ) {
		//Display the captured frame
		cvShowImage( "Camera", frame );
		cvSaveImage( "parts.jpg", frame );
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

	//*******************************************************************8
	int width, height;
	double avg[10];
	double  avg_p_a=0;
	IplImage *grayframe, *cannyIM, *erodeIM, *cc_img;


	//************
	CvMemStorage *storage = cvCreateMemStorage(0);
	CvSeq *contour = 0;
	CvScalar(ext_color);
	//************
	//***************

	//variables used to do PCA analysis
	CvMat* data_pts = NULL, *eigenval = NULL, *eigenvecs = NULL, *mean = NULL;
	CvPoint* input;
	CvPoint pos, axisMin, axisMaj; 

	int i = 0,k=0,j=0;

	//object metric variables
	double perimeter = 0, area = 0;
	//variables used to write on images

	//font type used to write on image
	CvFont font;
	//buffer to hold text written on image 
	char text[200];
	//initialize font sizing etc
	cvInitFont(&font,CV_FONT_HERSHEY_PLAIN,0.75,0.75,0.75,0.75,8);
	//get dimensions
	CLEyeCameraGetFrameDimensions(Camera, width, height);

	cvNamedWindow( "Canny Edge", CV_WINDOW_AUTOSIZE );
	cvNamedWindow( "Contours", CV_WINDOW_AUTOSIZE );

	grayframe=cvCreateImage(cvSize(width ,height),IPL_DEPTH_8U, 1);
	cannyIM=cvCreateImage(cvSize(width ,height),IPL_DEPTH_8U, 1);

	// Create Sliders
	//**********************************
	//CANNY
	// Format: Slider name, window name, reference to variable for slider, max value of slider, callback function
	cvCreateTrackbar("Low Threshold", "Canny Edge", &lowSliderPosC, maxLowCanny, onLowCannySlide);
	cvCreateTrackbar("High Threshold", "Canny Edge", &highSliderPosC, maxHighCanny, onHighCannySlide);

			

	/*
	//************************************************************
	//declare variables
	int width, height;
	IplImage *img=NULL, *grayframe=NULL, *cannyIM=NULL, *erodeIM=NULL, *dilateIM=NULL, *temp=NULL;

	//get dimensions
	CLEyeCameraGetFrameDimensions(Camera, width, height);

	//Create a window in which the captured images will be presented
	cvNamedWindow( "Gray", CV_WINDOW_AUTOSIZE );
	cvNamedWindow( "Canny Edge", CV_WINDOW_AUTOSIZE );
	cvNamedWindow( "Erode", CV_WINDOW_AUTOSIZE );
	//cvNamedWindow( "Contours", CV_WINDOW_AUTOSIZE );

	// make image to hold frames captured from camera
	img=cvCreateImage(cvSize(width ,height),IPL_DEPTH_8U, 4);
	grayframe=cvCreateImage(cvSize(width ,height),IPL_DEPTH_8U, 1);
	temp=cvCreateImage(cvSize(width ,height),IPL_DEPTH_8U, 1);
	cannyIM=cvCreateImage(cvSize(width ,height),IPL_DEPTH_8U, 1); 
	erodeIM=cvCreateImage(cvSize(width ,height),IPL_DEPTH_8U, 1); 

	// Create Sliders
	//**********************************
	//CANNY
	// Format: Slider name, window name, reference to variable for slider, max value of slider, callback function
	cvCreateTrackbar("Low Threshold", "Canny Edge", &lowSliderPosC, maxLowCanny, onLowCannySlide);
	cvCreateTrackbar("High Threshold", "Canny Edge", &highSliderPosC, maxHighCanny, onHighCannySlide);

	//THRESHOLD
	// Format: Slider name, window name, reference to variable for slider, max value of slider, callback function
	cvCreateTrackbar("Low Threshold", "Erode", &lowSliderPosT, maxLowThreshold, onLowThresholdSlide);
	cvCreateTrackbar("High Threshold", "Erode", &highSliderPosT, maxHighThreshold, onHighThresholdSlide);
	//*************************************************************
	*/
	while(1){

		Sleep(100);
		//Get Frame From Camera
		CLEyeCameraGetFrame(Camera,FramePtr);

					cc_img=cvCreateImage(cvSize(width ,height),IPL_DEPTH_8U, 3);

		//convery to grayscale
		cvCvtColor(frame, grayframe, CV_RGB2GRAY);
		
		//do canny edge detection
		
				//cvSetImageROI(cannyIM, cvRect(0, 0, 480, 500));

		cvCanny(grayframe, cannyIM, lowSliderPosC, highSliderPosC, 3); //3,5,7
		//cvSobel(grayframe, cannyIM, 3, 3, 3);
		//cvErode(cannyIM, cannyIM, NULL,1);

		cvShowImage ("Canny Edge", cannyIM);


		//**********************************************BEGIN Find OBJECT PROPERTIES**************************************************************
		//find a sequence of points that describe each connected contour. Each contour will relate to the boundary of one object
		cvFindContours( cannyIM, storage, &contour, sizeof(CvContour), CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE, cvPoint(0, 0));
		//operate on each contour one at a time to find the perimeter, area, center of mass and major and minor axes.
		for (; contour != 0; contour = contour->h_next)
		{
			//find perimeter of object
			perimeter = cvArcLength(contour, CV_WHOLE_SEQ, -1);
			//find area of object
			area = cvContourArea(contour, CV_WHOLE_SEQ, -1);
			//can also find moments here
			//cvMoments(contour, moments, 1);
			//if contour area is too small, then it will be considered noise and discarded. Otherwise it will be treated as an object
			if(area > 200) //default 700
			{
				//ext_color = CV_RGB( rand()&255, rand()&255, rand()&255 ); //randomly coloring different contours
				ext_color = CV_RGB(10,100,100);//draw all contours blue
				//draw filled contours onto processed image
				//cvDrawContours(cc_img, contour, ext_color, CV_RGB(0,255,0), -1, CV_FILLED, 8, cvPoint(0,0)); 

				//draw outline of contours in bright green
				cvDrawContours(cc_img, contour, ext_color, CV_RGB(0,255,0), -1, 1, 8, cvPoint(0,0));

				//Construct a buffer of points to be used in principal component analysis(PCA)
				data_pts = cvCreateMat(contour->total, 2, CV_64FC1);//this buffer has the same size as the current contour
				for(i = 0; i < contour->total; ++i)
				{
					//get a point in contour array
					input = (CvPoint*)cvGetSeqElem(contour,i);
					//add it to the set of data points to be operated on
					cvmSet(data_pts,i,0,input[0].x);
					cvmSet(data_pts,i,1,input[0].y);
				}

				//Perform PCA analysis
				eigenval = cvCreateMat(1,2,CV_64FC1);//create a space to store the major and minor axes eigenvalues
				eigenvecs = cvCreateMat(2,2,CV_64FC1);//create a space to store the major and minor axes eigenvectors
				mean = cvCreateMat(1,2,CV_64FC1); //create a place to store the mean/'center of mass' of the contour 
				cvCalcPCA(data_pts, mean, eigenval, eigenvecs, CV_PCA_DATA_AS_ROW);//do PCA

				//Store the position of the object's center
				pos.x = (int)cvmGet(mean,0,0);
				pos.y = (int)cvmGet(mean,0,1);

				//Store the eigenvalues and eigenvectors. Major and minor axes are scaled to fit image
				//axisMaj is the last point on the major axis
				axisMaj.x = (int)(pos.x + 0.02 * ( cvmGet(eigenvecs,0,0)*cvmGet(eigenval,0,0) ) );
				axisMaj.y = (int)(pos.y + 0.02 * ( cvmGet(eigenvecs,0,1)*cvmGet(eigenval,0,0) ) );

				//always draw major axis pointing upwards 
				if(axisMaj.y > pos.y)
				{
    				axisMaj.y = pos.y + (pos.y - axisMaj.y);
    				axisMaj.x = pos.x + (pos.x - axisMaj.x);
				}

				//axisMin is the last point on the minor axis
				axisMin.x = (int)(pos.x + 0.02 * ( cvmGet(eigenvecs,1,0)*cvmGet(eigenval,0,1) ) );
				axisMin.y = (int)(pos.y + 0.02 * ( cvmGet(eigenvecs,1,1)*cvmGet(eigenval,0,1) ) );

				// Draw the principal components
				//Draw center of mass
				cvCircle(cc_img, pos, 1, CV_RGB(255, 0, 255), 2, 8, 0);

				//Draw major axis of component in yellow
				cvLine(cc_img, pos, axisMaj , CV_RGB(255, 255, 0), 1, 8, 0);

				//Draw minor axis of component in cyan
				cvLine(cc_img, pos, axisMin, CV_RGB(0, 255, 255),1, 8, 0);

				//display details of each object
				//printf("Location: (%3d, %3d), Perimeter: %4.1f, Area: %4.1f P/A: %4.3f\n", pos.x, pos.y, perimeter, area, perimeter/area);
				/*
				if(perimeter/area >=0.140 && perimeter/area <=0.155)
					printf("Found part #10: Nylon Machine Screw (#6-32x1/2 round)\n");
				if(perimeter/area >=0.100 && perimeter/area <=0.103)
					printf("Found part #16: Nylon Nut (#6)\n");
				if(perimeter/area >=0.125 && perimeter/area <=0.135)
					printf("Found part #20: Steel Washer (1/4x 1-1/4)\n");
				//if(perimeter/area >=0.075 && perimeter/area <=0.088)
				//	printf("Found part #12: Nylon Machine Screw (1/4-20 x 1 round)\n");
				if(perimeter/area >=0.108 && perimeter/area <=0.120)
					printf("Found part #11: Nylon Machine Screw (1/4-20 x 1 round)\n");
				if(perimeter/area >=0.177 && perimeter/area <=0.187)
					printf("Found part #21: Steel Washer (#10 x 11/16)\n");
				if(perimeter/area >=0.038 && perimeter/area <=0.048)
					printf("Found part #25: Nylon Washer (5/16x0.74)\n");
				*/
				//orientation = atan2( cvmGet(eigenvecs,0,1), cvmGet(eigenvecs,0,0) )*180/PI + 90;
			
				//prepare text to be written on image. Text will contain perimeter(P) and area(A)
				sprintf(text, "(%d,%d)", pos.x, pos.y);
				//place text on image near to the center of mass of image
				cvPutText(cc_img, text, cvPoint(pos.x + 2,pos.y+2), &font, CV_RGB(255,255,255)  );
				sprintf(text, "P:%4.1f", perimeter);

				//place text on image near to the center of mass of image
				cvPutText(cc_img, text, cvPoint(pos.x + 2,pos.y+10), &font, CV_RGB(255,255,255)  );
				sprintf(text, "A:%4.4f", perimeter/area);
				cvPutText(cc_img, text, cvPoint(pos.x + 2,pos.y+18), &font, CV_RGB(255,255,255)  );
				
				if(k != 10 && area!=0){
					avg[k]=perimeter/area;
					k++;
				}
			

				//release allocated memory for matrix variables
				cvReleaseMat( &data_pts );
				cvReleaseMat( &eigenval );
				cvReleaseMat( &eigenvecs );
				cvReleaseMat( &mean );
			}
		}

		//**********************************************END FIND OBJECT PROPERTIES**************************************************************



	  cvShowImage ( "Contours", cc_img );

	  if( (cvWaitKey(1) & 255) == 112 ){

		  if(k=0){
			  printf("No area found!\n");
		  }
		  else{
			  for(j=0;j<10;j++){
				printf("p/a = %4.5f\n", avg[j]);
				avg_p_a = avg_p_a + avg[j];
			  }
			  printf("Average P/A = %4.5f", avg_p_a/10);
		  }
		  if( (cvWaitKey(0) & 255) == 112 ){
			  k=0;
			  avg_p_a=0;
			  for(j=0;j<10;j++){
				avg[j]=0;
			  }
			  continue;
		  }

	  }
	  cvReleaseImage(&cc_img);
	  
		// Track FPS
		
		if(FramerCounter==0) StartTime=clock();
		FramerCounter++;
		EndTime=clock();
		if((EndTime-StartTime)/CLOCKS_PER_SEC>=1)
		{
			printf("FPS: %d\n",FramerCounter);
			FramerCounter=0;
		}
		
	}//end while

	cvReleaseImage(&grayframe);
	cvReleaseImage(&cannyIM);

	CLEyeCameraStop(Camera);
	CLEyeDestroyCamera(Camera);
	Camera = NULL;
	cvDestroyWindow( "Canny Edge" );
	cvDestroyWindow( "Erode" );
	return 0;
}

