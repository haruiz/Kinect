// ColorC++.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#include <NuiApi.h>
#include <opencv2\opencv.hpp>
/*
OpenCV:
Cv: contiene las funciones principales de la biblioteca
Cvaux: contiene las funciones auxiliares (experimentales)
Cxcore: contiene las estructuras de datos y funciones de soporte para algebra lineal
Highgui: funciones para el manejo de GUI

*/
using namespace cv;

int _tmain(int argc, _TCHAR* argv[])
{
	// Usando la función NuiGetSensorCount podemos obtener la cantidad de sensores conectados
	int sensorCount;
	HRESULT hResult = S_OK;
	hResult = NuiGetSensorCount(&sensorCount);
	if( SUCCEEDED( hResult ) && sensorCount <= 0){
		std::cerr << "Error : No hay ningun sensor conectado" << std::endl;
		std::system("pause");
		return EXIT_FAILURE;
	}
	
	/*
	La función NuiCreateSensorByIndex crea una nueva Instancia de la clase INUISensor, 
	el primer parámetro corresponde al índex del sensor con el que vamos a trabajar, 
	en caso de que haya más de un sensor conectado
	*/
	INuiSensor* kinect;
	hResult = NuiCreateSensorByIndex( 0, &kinect );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiCreateSensorByIndex" << std::endl;
		std::system("pause");
		return EXIT_FAILURE;
	}

	//inicializamos el sensor usando la función NuiInitialize,especificando 
	//que características queremos habilitar usando las banderas
	//NUI_INITIALIZE_FLAG_USES_COLOR: color stream
	//NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX: depth stream
	//NUI_INITIALIZE_FLAG_USES_SKELETON: Sekeleton Tracking		
	
	hResult = kinect->NuiInitialize(  NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiInitialize" << std::endl;
		std::system("pause");
		return -1;
	}

	/*
	NuiImageStreamOpen() esta función es un poco confusa, superficialmente lo que hace es inicializar un 
	HANDLE Object que podemos usar luego para obtener cada frame de video (RGB Stream/DEPTH Stream, de acuerdo a lo que hayamos definido en  el primer argumento,
	NUI_IMAGE_TYPE_COLOR para el RGB color image stream, o NUI_IMAGE_TYPE_DEPTH para el Depth image Stream) a una 
	Resolución de NUI_IMAGE_RESOLUTION_640x480.
	*/

	HANDLE hDepthEvent = INVALID_HANDLE_VALUE;
	HANDLE hDepthHandle = INVALID_HANDLE_VALUE;
	hDepthEvent = CreateEvent( nullptr, true, false, nullptr );
	hResult = kinect->NuiImageStreamOpen(  NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX, NUI_IMAGE_RESOLUTION_640x480, 0, 2, hDepthEvent, &hDepthHandle );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiImageStreamOpen( COLOR )" << std::endl;
		return -1;
	}
	//array de colores, para cada player un color diferente (maximo 6)
	cv::Vec3b color[7];
	color[0] = cv::Vec3b(   0,   0,   0 );
	color[1] = cv::Vec3b( 255,   0,   0 );
	color[2] = cv::Vec3b(   0, 255,   0 );
	color[3] = cv::Vec3b(   0,   0, 255 );
	color[4] = cv::Vec3b( 255, 255,   0 );
	color[5] = cv::Vec3b( 255,   0, 255 );
	color[6] = cv::Vec3b(   0, 255, 255 );
	//crea una ventana 
	cv::namedWindow("depth", CV_WINDOW_NORMAL);
	cv::namedWindow( "Player" , CV_WINDOW_NORMAL);

	while (true){		
		ResetEvent( hDepthEvent );//cambia el estado del objeto pasado como parametro a nonsignaled 
		/*WaitForSingleObject: esta función Detiene el flujo de ejecución de nuestra app (espera) hasta que el estado del objeto hColorEvent cambie a signaled.
		Para mas info: https://msdn.microsoft.com/en-us/library/windows/desktop/ms687032%28v=vs.85%29.aspx
		*/
		WaitForSingleObject( hDepthEvent, INFINITE );
				
		// Obtenemos el frame actual
		NUI_IMAGE_FRAME pDepthImageFrame = { 0 };
		hResult = kinect->NuiImageStreamGetNextFrame( hDepthHandle, 0, &pDepthImageFrame );
		if( FAILED( hResult ) ){
			std::cerr << "Error : NuiImageStreamGetNextFrame( COLOR )" << std::endl;
			return EXIT_FAILURE;
		}
			
		/*
		Luego de obtener el frame de video (Depth/Color) actual, existen tres estructuras muy importantes:
		NUI_IMAGE_FRAME: Contiene la data de alto nivel (metadata) del frame de video capturado ( index,resolution, etc.)
		NUI_LOCKED_RECT: Contiene la data de bajo nivel del frame (bytes)  de video capturado 
		INuiFrameTexture me permite acceder a la data del frame de video capturado.
		*/		

		
		INuiFrameTexture* pDepthPlayerFrameTexture = pDepthImageFrame.pFrameTexture;
		NUI_LOCKED_RECT sDepthPlayerLockedRect;
		pDepthPlayerFrameTexture->LockRect( 0, &sDepthPlayerLockedRect, nullptr, 0 );

		
		LONG registX = 0;
		LONG registY = 0;
		//obtenemos los bits  de la imagen
		ushort* pBuffer = reinterpret_cast<ushort*>( sDepthPlayerLockedRect.pBits );
		cv::Mat bufferMat = cv::Mat::zeros( 480, 640, CV_16UC1 );//matriz depth pixels (1 channel - Gray)
		cv::Mat playerMat = cv::Mat::zeros( 480, 640, CV_8UC3 );//matriz player pixels (3 channels - Color)
		for( int y = 0; y < 480; y++ ){
			for( int x = 0; x < 640; x++ ){
				kinect->NuiImageGetColorPixelCoordinatesFromDepthPixelAtResolution( NUI_IMAGE_RESOLUTION_640x480, NUI_IMAGE_RESOLUTION_640x480, nullptr, x, y, *pBuffer, &registX, &registY );
				if( ( registX >= 0 ) && ( registX < 640 ) && ( registY >= 0 ) && ( registY < 480 ) ){
					bufferMat.at<ushort>( registY, registX ) = *pBuffer & 0xFFF8;
					playerMat.at<cv::Vec3b>( registY, registX ) = color[*pBuffer & 0x7];
				}
				pBuffer++;
			}
		}
		cv::Mat depthMat( 480, 640, CV_8UC1 );
		bufferMat.convertTo( depthMat, CV_8UC3, -255.0f / NUI_IMAGE_DEPTH_MAXIMUM, 255.0f );
		
		/*Existen diferentes formas de adquirir una imagen del mundo real, usando una cámara fotográfica, 
		usando el Kinect, usando un Smartphone etc. sin embargo al final, luego de que estas imágenes son digitalizadas 
		por el sensor, terminan convirtiéndose en matrices de valores, en donde cada valor f(x,y) representa el valor 
		de intensidad de color de un pixel. 
		OpenCV	usa dos estructuras de datos para representar estas imágenes 
		la estructura Mat(bajo nivel) y la IPlIMage(alto nivel)	
		*/

		
		//esta función despliega o muestra una imagen sobre una ventana específica 
		cv::imshow("depth",depthMat);
		cv::imshow( "Player", playerMat );
		//Unlocks the buffer.
		pDepthPlayerFrameTexture->UnlockRect( 0 );
		kinect->NuiImageStreamReleaseFrame( hDepthHandle, &pDepthImageFrame );
		
		if( cv::waitKey( 30 ) == VK_ESCAPE ){
			break;
		}
	}

	//Apagamos el sensor, liberamos memoria
	kinect->NuiShutdown();
	CloseHandle( hDepthEvent );
	CloseHandle( hDepthHandle );
	//Destruimos todas las ventanas creadas
	cv::destroyAllWindows();
	return EXIT_SUCCESS;
}

