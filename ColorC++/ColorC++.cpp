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
		return EXIT_FAILURE;
	}

	//inicializamos el sensor usando la función NuiInitialize,especificando 
	//que características queremos habilitar usando las banderas
	//NUI_INITIALIZE_FLAG_USES_COLOR: color stream
	//NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX: depth stream
	//NUI_INITIALIZE_FLAG_USES_SKELETON: Sekeleton Tracking		
	
	hResult = kinect->NuiInitialize( NUI_INITIALIZE_FLAG_USES_COLOR | NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX | NUI_INITIALIZE_FLAG_USES_SKELETON );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiInitialize" << std::endl;
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
	hResult = kinect->NuiImageStreamOpen( NUI_IMAGE_TYPE_COLOR, NUI_IMAGE_RESOLUTION_640x480, 0, 2, hColorEvent, &hColorHandle );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiImageStreamOpen( COLOR )" << std::endl;
		return -1;
	}
	//crea una ventana 
	cv::namedWindow("color", CV_WINDOW_NORMAL);

	while (true){		
		ResetEvent( hColorEvent );//cambia el estado del objeto pasado como parametro a nonsignaled 
		/*WaitForSingleObject: esta función Detiene el flujo de ejecución de nuestra app (espera) hasta que el estado del objeto hColorEvent cambie a signaled.
		Para mas info: https://msdn.microsoft.com/en-us/library/windows/desktop/ms687032%28v=vs.85%29.aspx
		*/
		WaitForSingleObject( hColorEvent, INFINITE );
				
		// Obtenemos el frame actual
		NUI_IMAGE_FRAME pColorImageFrame = { 0 };
		hResult = kinect->NuiImageStreamGetNextFrame( hColorHandle, 0, &pColorImageFrame );
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

		
		INuiFrameTexture* pColorFrameTexture = pColorImageFrame.pFrameTexture;
		NUI_LOCKED_RECT sColorLockedRect;
		//pColorFrameTexture->LockRect:Locks el buffer for read and write access.
		
		pColorFrameTexture->LockRect( 0, &sColorLockedRect, nullptr, 0 );
		
		/*Existen diferentes formas de adquirir una imagen del mundo real, usando una cámara fotográfica, 
		usando el Kinect, usando un Smartphone etc. sin embargo al final, luego de que estas imágenes son digitalizadas 
		por el sensor, terminan convirtiéndose en matrices de valores, en donde cada valor f(x,y) representa el valor 
		de intensidad de color de un pixel. 
		OpenCV	usa dos estructuras de datos para representar estas imágenes 
		la estructura Mat(bajo nivel) y la IPlIMage(alto nivel)	
		*/

		//En esta línea creamos un Objeto Mat a partir de los bits de cada frame de video capturado por Kinect
		cv::Mat MatrizDeColor( 480, 640, CV_8UC4, reinterpret_cast<uchar*>( sColorLockedRect.pBits ) );

		//esta función despliega o muestra una imagen sobre una ventana específica 
		cv::imshow("color",MatrizDeColor);

		//Unlocks the buffer.
		pColorFrameTexture->UnlockRect( 0 );
		kinect->NuiImageStreamReleaseFrame( hColorHandle, &pColorImageFrame );
		
		if( cv::waitKey( 30 ) == VK_ESCAPE ){
			break;
		}
	}

	//Apagamos el sensor, liberamos memoria
	kinect->NuiShutdown();
	kinect->NuiSkeletonTrackingDisable();
	CloseHandle( hColorEvent );
	CloseHandle( hColorHandle );
	//Destruimos todas las ventanas creadas
	cv::destroyAllWindows();
	return EXIT_SUCCESS;
}

