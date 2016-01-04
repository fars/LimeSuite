/**
    @file ConnectionSTREAM.h
    @author Lime Microsystems
    @brief Implementation of STREAM board connection.
*/

#pragma once
#include <ConnectionRegistry.h>
#include <IConnection.h>
#include <LMS64CProtocol.h>
#include <vector>
#include <string>

#ifndef __unix__
#include "windows.h"
#include "CyAPI.h"
#else
#include <libusb-1.0/libusb.h>
#include <mutex>
#include <condition_variable>
#include <chrono>
#endif

#define USB_MAX_CONTEXTS 64 //maximum number of contexts for asynchronous transfers

/** @brief Wrapper class for holding USB asynchronous transfers contexts
*/
class USBTransferContext
{
public:
	USBTransferContext() : used(false)
	{
		#ifndef __unix__
		inOvLap = new OVERLAPPED;
		memset(inOvLap, 0, sizeof(OVERLAPPED));
		inOvLap->hEvent = CreateEvent(NULL, false, false, NULL);
		context = NULL;
		#else
		transfer = libusb_alloc_transfer(0);
		bytesXfered = 0;
		bytesExpected = 0;
		done = 0;
		#endif
	}
	~USBTransferContext()
	{
		#ifndef __unix__
		CloseHandle(inOvLap->hEvent);
		delete inOvLap;
		#else
		libusb_free_transfer(transfer);
		#endif
	}
	bool reset()
	{
        if(used)
            return false;
        #ifndef __unix__
        CloseHandle(inOvLap->hEvent);
        memset(inOvLap, 0, sizeof(OVERLAPPED));
        inOvLap->hEvent = CreateEvent(NULL, false, false, NULL);
        #endif
        return true;
	}
	bool used;
	#ifndef __unix__
	PUCHAR context;
	OVERLAPPED *inOvLap;
	#else
	libusb_transfer* transfer;
	long bytesXfered;
	long bytesExpected;
	bool done;
	std::mutex m_lock;
    std::condition_variable mPacketProcessed;
	#endif
};

class ConnectionSTREAM : public LMS64CProtocol
{
public:
    ConnectionSTREAM(void *ctx, const unsigned index, const int vid=-1, const int pid=-1);

    ~ConnectionSTREAM(void);

	DeviceStatus Open(const unsigned index, const int vid, const int pid);
	void Close();
	bool IsOpen();
	int GetOpenedIndex();

	int Write(const unsigned char *buffer, int length, int timeout_ms = 0);
	int Read(unsigned char *buffer, int length, int timeout_ms = 0);

	virtual int BeginDataReading(char *buffer, long length);
	virtual int WaitForReading(int contextHandle, unsigned int timeout_ms);
	virtual int FinishDataReading(char *buffer, long &length, int contextHandle);
	virtual void AbortReading();
    virtual int ReadDataBlocking(char *buffer, long &length, int timeout_ms);

	virtual int BeginDataSending(const char *buffer, long length);
	virtual int WaitForSending(int contextHandle, unsigned int timeout_ms);
	virtual int FinishDataSending(const char *buffer, long &length, int contextHandle);
	virtual void AbortSending();
private:

    eConnectionType GetType(void)
    {
        return USB_PORT;
    }

    std::string DeviceName();

    std::string m_hardwareName;
    int m_hardwareVer;

	USBTransferContext contexts[USB_MAX_CONTEXTS];
	USBTransferContext contextsToSend[USB_MAX_CONTEXTS];

	bool isConnected;

	#ifndef __unix__
	CCyUSBDevice *USBDevicePrimary;
	//control endpoints for DigiRed
	CCyControlEndPoint *InCtrlEndPt3;
	CCyControlEndPoint *OutCtrlEndPt3;

    //control endpoints for DigiGreen
	CCyUSBEndPoint *OutCtrEndPt;
	CCyUSBEndPoint *InCtrEndPt;

    //end points for samples reading and writing
	CCyUSBEndPoint *InEndPt;
	CCyUSBEndPoint *OutEndPt;

	#else
    libusb_device **devs; //pointer to pointer of device, used to retrieve a list of devices
    libusb_device_handle *dev_handle; //a device handle
    libusb_context *ctx; //a libusb session
	#endif
};



class ConnectionSTREAMEntry : public ConnectionRegistryEntry
{
public:
    ConnectionSTREAMEntry(void);

    ~ConnectionSTREAMEntry(void);

    std::vector<ConnectionHandle> enumerate(const ConnectionHandle &hint);

    IConnection *make(const ConnectionHandle &handle);

private:
    #ifndef __unix__
    CCyUSBDevice *USBDevicePrimary;
    #else
    libusb_context *ctx; //a libusb session
    #endif
};