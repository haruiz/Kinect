// SkeletalTracking.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#include <NuiApi.h>
#include <opencv2\opencv.hpp>

void drawSkeleton(cv::Mat m ,  NUI_SKELETON_DATA  skeleton);


int _tmain(int argc, _TCHAR* argv[]){


	cv::setUseOptimized( true );


		/*
	La función NuiCreateSensorByIndex crea una nueva Instancia de la clase INUISensor, 
	el primer parámetro corresponde al índex del sensor con el que vamos a trabajar, 
	en caso de que haya más de un sensor conectado
	*/
	INuiSensor* kinect;
	HRESULT hr = NuiCreateSensorByIndex( 0, &kinect );
	if( FAILED( hr ) ){
		std::cerr << "Error : NuiCreateSensorByIndex" << std::endl;
		return EXIT_FAILURE;
	}

	//inicializamos el sensor usando la función NuiInitialize,especificando 
	//que características queremos habilitar usando las banderas
	//NUI_INITIALIZE_FLAG_USES_COLOR: color stream
	//NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX: depth stream
	//NUI_INITIALIZE_FLAG_USES_SKELETON: Sekeleton Tracking		
	hr = kinect->NuiInitialize( NUI_INITIALIZE_FLAG_USES_COLOR | NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX | NUI_INITIALIZE_FLAG_USES_SKELETON );
	if( FAILED( hr ) ){
		std::cerr << "Error : NuiInitialize" << std::endl;
		return -1;
	}

	// Enable Skeleton Tracking
	HANDLE hSkeletonEvent = INVALID_HANDLE_VALUE;
	hSkeletonEvent = CreateEvent( nullptr, true, false, nullptr );
	hr = kinect->NuiSkeletonTrackingEnable( hSkeletonEvent, 0);//NUI_SKELETON_TRACKING_FLAG_ENABLE_SEATED_SUPPORT );
	if( FAILED( hr ) ){
		std::cerr << "Error : NuiSkeletonTrackingEnable" << std::endl;
		return -1;
	}

	/*
	NuiImageStreamOpen() esta función es un poco confusa, superficialmente lo que hace es inicializar un 
	HANDLE Object que podemos usar luego para obtener cada frame de video (RGB Stream/DEPTH Stream, de acuerdo a lo que hayamos definido en  el primer argumento,
	NUI_IMAGE_TYPE_COLOR para el RGB color image stream, o NUI_IMAGE_TYPE_DEPTH para el Depth image Stream) a una 
	Resolución de NUI_IMAGE_RESOLUTION_640x480.
	*/
	HANDLE hColorEvent = INVALID_HANDLE_VALUE;
	HANDLE hColorHandle = INVALID_HANDLE_VALUE;
	hColorEvent = CreateEvent( nullptr, true, false, nullptr );
	hr = kinect->NuiImageStreamOpen( NUI_IMAGE_TYPE_COLOR, NUI_IMAGE_RESOLUTION_640x480, 0, 2, hColorEvent, &hColorHandle );
	if( FAILED( hr ) ){
		std::cerr << "Error : NuiImageStreamOpen( COLOR )" << std::endl;
		return -1;
	}
	//crea una ventana, para mostrar el skeleton
	cv::namedWindow( "Skeleton" );

	HANDLE hEvents[2] = { hColorEvent,  hSkeletonEvent };


	
	while (true)
	{
		ResetEvent( hSkeletonEvent );
		ResetEvent( hColorEvent );
		
		//WaitForSingleObject(hSkeletonEvent, INFINITE);
		WaitForMultipleObjects( ARRAYSIZE( hEvents ), hEvents, true, INFINITE );

		// get Color Frame
		NUI_IMAGE_FRAME pColorImageFrame = { 0 };
		hr = kinect->NuiImageStreamGetNextFrame( hColorHandle, 0, &pColorImageFrame );
		if( FAILED( hr ) ){
			std::cerr << "Error : NuiImageStreamGetNextFrame( COLOR )" << std::endl;
			return -1;
		}


		// get Skeleton Frame
		NUI_SKELETON_FRAME pSkeletonFrame = { 0 };
		hr = kinect->NuiSkeletonGetNextFrame( 0, &pSkeletonFrame );
		if( FAILED( hr ) ){
			std::cout << "Error : NuiSkeletonGetNextFrame" << std::endl;
			return -1;
		}

		// GetColor Stream
		INuiFrameTexture* pColorFrameTexture = pColorImageFrame.pFrameTexture;
		NUI_LOCKED_RECT sColorLockedRect;
		pColorFrameTexture->LockRect( 0, &sColorLockedRect, nullptr, 0 );
		cv::Mat m( 480, 640, CV_8UC4, reinterpret_cast<uchar*>( sColorLockedRect.pBits ) );


		//Get Skeleton Stream		
		cv::Point2f point;
		for( int count = 0; count < NUI_SKELETON_COUNT; count++ ){
			NUI_SKELETON_DATA skeleton = pSkeletonFrame.SkeletonData[count];
				if( skeleton.eTrackingState == NUI_SKELETON_TRACKED ){
					drawSkeleton(m, skeleton);
				}
		}

		cv::imshow( "Skeleton", m );//show Skeleton
		
		//release color stream 
		pColorFrameTexture->UnlockRect( 0 ); kinect->NuiImageStreamReleaseFrame( hColorHandle, &pColorImageFrame );

		if( cv::waitKey( 30 ) == VK_ESCAPE ){ break;}
	}

	// Kinect
	kinect->NuiShutdown(); kinect->NuiSkeletonTrackingDisable();CloseHandle( hColorEvent );CloseHandle( hSkeletonEvent );cv::destroyAllWindows();

	return 0;
}


//draw Join
void drawBone(cv::Mat m ,  NUI_SKELETON_DATA  skeleton,  NUI_SKELETON_POSITION_INDEX jointFrom,NUI_SKELETON_POSITION_INDEX jointTo ){

	  NUI_SKELETON_POSITION_TRACKING_STATE jointFromState = skeleton.eSkeletonPositionTrackingState[jointFrom];
     
	  NUI_SKELETON_POSITION_TRACKING_STATE jointToState = skeleton.eSkeletonPositionTrackingState[jointTo];

	  if (jointFromState == NUI_SKELETON_POSITION_NOT_TRACKED || jointToState == NUI_SKELETON_POSITION_NOT_TRACKED){
          return; // nothing to draw, one of the joints is not tracked
        }

	   // Don't draw if both points are inferred
        if (jointFromState == NUI_SKELETON_POSITION_INFERRED || jointToState == NUI_SKELETON_POSITION_INFERRED){
			cv::Point2f pointFrom;
			NuiTransformSkeletonToDepthImage( skeleton.SkeletonPositions[jointFrom], &pointFrom.x, &pointFrom.y, NUI_IMAGE_RESOLUTION_640x480 );
			cv::Point2f pointTo;
			NuiTransformSkeletonToDepthImage( skeleton.SkeletonPositions[jointTo], &pointTo.x, &pointTo.y, NUI_IMAGE_RESOLUTION_640x480 );
			cv::line(m,pointFrom, pointTo, static_cast<cv::Scalar>( cv::Vec3b(   0, 0,  255 ) ),2,CV_AA);
			cv::circle( m, pointTo, 5, static_cast<cv::Scalar>(cv::Vec3b(   0, 255,  255 )), -1, CV_AA );
		}

		 // We assume all drawn bones are inferred unless BOTH joints are tracked
        if (jointFromState == NUI_SKELETON_POSITION_TRACKED && jointToState == NUI_SKELETON_POSITION_TRACKED)
        {
			cv::Point2f pointFrom;
			NuiTransformSkeletonToDepthImage( skeleton.SkeletonPositions[jointFrom], &pointFrom.x, &pointFrom.y, NUI_IMAGE_RESOLUTION_640x480 );
			cv::Point2f pointTo;
			NuiTransformSkeletonToDepthImage( skeleton.SkeletonPositions[jointTo], &pointTo.x, &pointTo.y, NUI_IMAGE_RESOLUTION_640x480 );
			//dibujamos una linea que entre los dos punto
			cv::line(m,pointFrom, pointTo, static_cast<cv::Scalar>( cv::Vec3b(   0, 255,   0 ) ),2,CV_AA);
			//en donde inicia cada linea dibujamos un circulo
			cv::circle( m, pointTo, 5, static_cast<cv::Scalar>(cv::Vec3b(   0, 255,  255 )), -1, CV_AA );
		}

}

//draw the body
void drawSkeleton(cv::Mat m ,  NUI_SKELETON_DATA  skeleton){
	 //Head and Shoulders (cabeza y espalda)
      drawBone(m , skeleton, NUI_SKELETON_POSITION_HEAD, NUI_SKELETON_POSITION_SHOULDER_CENTER);
      drawBone(m, skeleton, NUI_SKELETON_POSITION_SHOULDER_CENTER, NUI_SKELETON_POSITION_SHOULDER_LEFT);
      drawBone(m, skeleton, NUI_SKELETON_POSITION_SHOULDER_CENTER, NUI_SKELETON_POSITION_SHOULDER_RIGHT);
	
	  //hip(cadera)
	  drawBone(m,skeleton, NUI_SKELETON_POSITION_SHOULDER_CENTER, NUI_SKELETON_POSITION_SPINE);
	  drawBone(m,skeleton, NUI_SKELETON_POSITION_SPINE, NUI_SKELETON_POSITION_HIP_CENTER);
	  drawBone(m,skeleton, NUI_SKELETON_POSITION_HIP_CENTER, NUI_SKELETON_POSITION_HIP_LEFT);
	  drawBone(m,skeleton, NUI_SKELETON_POSITION_HIP_CENTER, NUI_SKELETON_POSITION_HIP_RIGHT);

	  //Knee (rodillas)
	  drawBone(m,skeleton, NUI_SKELETON_POSITION_HIP_LEFT, NUI_SKELETON_POSITION_KNEE_LEFT);
	  drawBone(m,skeleton, NUI_SKELETON_POSITION_HIP_RIGHT, NUI_SKELETON_POSITION_KNEE_RIGHT);
      
	   //Ankle (rodillas)
	  drawBone(m,skeleton, NUI_SKELETON_POSITION_KNEE_LEFT, NUI_SKELETON_POSITION_ANKLE_LEFT);
	  drawBone(m,skeleton, NUI_SKELETON_POSITION_KNEE_RIGHT, NUI_SKELETON_POSITION_ANKLE_RIGHT);
	  drawBone(m,skeleton, NUI_SKELETON_POSITION_ANKLE_LEFT, NUI_SKELETON_POSITION_FOOT_LEFT);
	  drawBone(m,skeleton, NUI_SKELETON_POSITION_ANKLE_RIGHT, NUI_SKELETON_POSITION_FOOT_RIGHT);
      

	  //Left Arm(brazo izquierdo)
      drawBone(m,skeleton, NUI_SKELETON_POSITION_SHOULDER_LEFT, NUI_SKELETON_POSITION_ELBOW_LEFT);
      drawBone(m,skeleton, NUI_SKELETON_POSITION_ELBOW_LEFT, NUI_SKELETON_POSITION_WRIST_LEFT);
      drawBone(m,skeleton, NUI_SKELETON_POSITION_WRIST_LEFT, NUI_SKELETON_POSITION_HAND_LEFT);

      //Right Arm(brazo derecho)
      drawBone(m,skeleton, NUI_SKELETON_POSITION_SHOULDER_RIGHT, NUI_SKELETON_POSITION_ELBOW_RIGHT);
      drawBone(m,skeleton, NUI_SKELETON_POSITION_ELBOW_RIGHT, NUI_SKELETON_POSITION_WRIST_RIGHT);
      drawBone(m,skeleton, NUI_SKELETON_POSITION_WRIST_RIGHT, NUI_SKELETON_POSITION_HAND_RIGHT);

	 


}

