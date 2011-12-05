//
// test.cpp - A DirectShow console application
// written by Ted Burke on 26-9-2011
//
// Info about building DirectShow applications:
// http://msdn.microsoft.com/en-us/library/windows/desktop/dd377592%28v=vs.85%29.aspx
// 
// 

#include <windows.h>
#include <dshow.h>

// Would the following line suffice to pull in required library?
//#pragma comment(lib, "strmiids")

void main()
{
	HRESULT hr;

	// DirectShow objects
	ICreateDevEnum *pDevEnum = NULL;
	IEnumMoniker *pEnum = NULL;
	IMoniker *pMoniker = NULL;
	IPropertyBag *pPropBag = NULL;

	// More DirectShow objects for graph
	IGraphBuilder *pGraph = NULL;
	ICaptureGraphBuilder2 *pBuilder = NULL;
	IBaseFilter *pCap = NULL;
	IBaseFilter *pCompressor = NULL;
	IMediaControl *pMediaControl = NULL;

	printf("Initialising...\n");

	//CoInitialize(NULL);
	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

	// Create filter graph
	hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&pGraph); // Filter graph manager
	hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL,	CLSCTX_INPROC_SERVER, IID_ICaptureGraphBuilder2, (void **)&pBuilder); // Capture Graph Builder.
	pBuilder->SetFiltergraph(pGraph);

	// Create system device enumerator
	hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDevEnum));

	// Look for video input device	
	hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnum, 0); // Video input device enumerator
	if (hr == VFW_E_NOT_FOUND) {printf("No video input devices found.\n"); return;}
	hr = pEnum->Next(1, &pMoniker, NULL); // Get moniker for next video input device
	pEnum->Release();

	// Get video input device name
	hr = pMoniker->BindToStorage(0, 0, IID_PPV_ARGS(&pPropBag)); // Get properties of this video input device
	VARIANT var;
	VariantInit(&var);
	//hr = pPropBag->Read(L"Description", &var, 0);
	hr = pPropBag->Read(L"FriendlyName", &var, 0);
	printf("%ls\n", var.bstrVal);
	VariantClear(&var);
	pPropBag->Release();
	//hr = pPropBag->Write(L"FriendlyName", &var);
	//hr = pPropBag->Read(L"DevicePath", &var, 0);
	// The device path is not intended for display.
	//printf("Device path: %S\n", var.bstrVal);
	//VariantClear(&var);

	// Build graph
	//pGraph->RenderFile(L"C:\\Documents and Settings\\Administrator\\Desktop\\EI Accreditation - April 2011\\robosumo_evidence\\video evidence\\staging\\robosumo_2008.avi", NULL);

	// Create capture filter and add to graph
	hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)&pCap);
	hr = pGraph->AddFilter(pCap, L"Capture Filter");
	pMoniker->Release();

	// Create an enumerator for video compression devices
	hr = pDevEnum->CreateClassEnumerator(CLSID_VideoCompressorCategory, &pEnum, 0);
	while (S_OK == pEnum->Next(1, &pMoniker, NULL))
	{
		IPropertyBag *pPropBag = NULL;
		pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pPropBag);
		VARIANT var;
		VariantInit(&var);
		hr = pPropBag->Read(L"FriendlyName", &var, 0);
		printf("%ls", var.bstrVal);
		//if (wcscmp(var.bstrVal, L"Microsoft H.263 Video Codec") == 0)
		if (wcscmp(var.bstrVal, L"WMVideo9 Encoder DMO") == 0)
		{
			// This is the desired compression filter
			printf(" ### ");
			hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)&pCompressor);
			if (SUCCEEDED(hr)) printf(" +"); else printf(" -");
			hr = pGraph->AddFilter(pCompressor, L"Compressor");
			if (SUCCEEDED(hr)) printf(" +"); else printf(" -");
		}
		printf("\n");
		VariantClear(&var);
		pPropBag->Release();
		pMoniker->Release();
	}
	pEnum->Release();

	// Prepare to output to file
	IBaseFilter *ppf;
	IFileSinkFilter *pSink;
	hr = pBuilder->SetOutputFileName(&MEDIASUBTYPE_Avi, L"C:\\ted_test.avi", &ppf, &pSink);

	// Connect up the filter graph - preview and capture streams
	printf("Connecting up the filter graph... ");
	hr = pBuilder->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, pCap, pCompressor, ppf);
	if (SUCCEEDED(hr)) printf(" +"); else printf(" -");
	hr = pBuilder->RenderStream(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video, pCap, NULL, NULL);
	if (SUCCEEDED(hr)) printf(" +"); else printf(" -");
	printf("\n");

	// Get media control interfaces to graph builder object
	hr = pGraph->QueryInterface(IID_IMediaControl, (void**)&pMediaControl);
	
	// Run graph
	printf("Previewing and capturing video...");
	pMediaControl->Run();
	Sleep(5000); // 5 second delay
	pMediaControl->Stop();
	printf("Done\n");

	// Release DirectShow COM objects
	pMediaControl->Release();
	pCompressor->Release();
	pCap->Release();
	pBuilder->Release();
	pGraph->Release();
	pDevEnum->Release();

	CoUninitialize();

	// Wait for a key press before exiting
	printf("Press enter to exit...\n");
	getchar();
}
