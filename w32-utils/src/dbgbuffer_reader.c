#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

typedef struct DBWinBuffer
{
  DWORD dwProcessId;
  CHAR  data[4096-sizeof(DWORD)];
} DBWinBuffer;

typedef struct DBWinBufferData
{
  HANDLE hDBWinBuffer;
  DBWinBuffer* pDBWinBuffer;
} DBWinBufferData;

#define DBWIN_BUFFER L"DBWIN_BUFFER"
#define DBWIN_DATA_READY L"DBWIN_DATA_READY"
#define DBWIN_BUFFER_READY L"DBWIN_BUFFER_READY"

HANDLE initBufferReadyEvent()
{
  HANDLE hBufferReadyEvent = OpenEventW(EVENT_ALL_ACCESS,
					FALSE,
					DBWIN_BUFFER_READY);
  if (!hBufferReadyEvent)
    {
      hBufferReadyEvent = CreateEventW(NULL,
				       FALSE,
				       TRUE,
				       DBWIN_BUFFER_READY);
    }
  return hBufferReadyEvent;
}

HANDLE initDataReadyEvent()
{
  HANDLE hDataReadyEvent = OpenEventW(SYNCHRONIZE,
				      FALSE,
				      DBWIN_DATA_READY);
  if (!hDataReadyEvent)
    {
      hDataReadyEvent = CreateEventW(NULL,
				     FALSE,
				     FALSE,
				     DBWIN_DATA_READY);
    }
  return hDataReadyEvent;
}

BOOL initDBWinBufferData(DBWinBufferData* pDBWinBufferData)
{
  HANDLE hDBWinBuffer;
  DBWinBuffer* pDBWinBuffer;

  if (!pDBWinBufferData)
    return FALSE;
  
  hDBWinBuffer = OpenFileMappingW(FILE_MAP_READ,
					 FALSE,
					 DBWIN_BUFFER);
  if (!hDBWinBuffer)
    {
      hDBWinBuffer = CreateFileMappingW(INVALID_HANDLE_VALUE,
					NULL,
					PAGE_READWRITE,
					0,
					sizeof(DBWinBuffer),
					DBWIN_BUFFER);
      if (!hDBWinBuffer)
	return FALSE;
    }

  pDBWinBuffer = (DBWinBuffer*)MapViewOfFile(hDBWinBuffer,
					     SECTION_MAP_READ,
					     0,
					     0,
					     0);
  if (!pDBWinBuffer)
    {
      CloseHandle(hDBWinBuffer);
      return FALSE;
    }

  pDBWinBufferData->hDBWinBuffer = hDBWinBuffer;
  pDBWinBufferData->pDBWinBuffer = pDBWinBuffer;
  
  return TRUE;
}

void closeDBWinBufferData(DBWinBufferData* pDBWinBufferData)
{
  if (!pDBWinBufferData)
    return;

  if (pDBWinBufferData->pDBWinBuffer)
    UnmapViewOfFile(pDBWinBufferData->pDBWinBuffer);
  if (pDBWinBufferData->hDBWinBuffer)
    CloseHandle(pDBWinBufferData->hDBWinBuffer);
}

int main(int argc, char* argv[])
{
  HANDLE hBufferReadyEvent;
  HANDLE hDataReadyEvent;
  DBWinBufferData bufferData;
  
  hBufferReadyEvent = initBufferReadyEvent();
  if (!hBufferReadyEvent)
    {
      fprintf(stderr, "initBufferReadyEvent() failed\n");
      return -1;
    }

  hDataReadyEvent = initDataReadyEvent();
  if (!hDataReadyEvent)
    {
      fprintf(stderr, "initDataReadyEvent() failed\n");
      return -1;
    }

  if (!initDBWinBufferData(&bufferData))
    {
      fprintf(stderr, "initDBWinBufferData() failed\n");
      return -1;
    }

  for (;;)
    {
      if (WaitForSingleObject(hDataReadyEvent, INFINITE) == WAIT_OBJECT_0)
	{
	  fprintf(stdout, "[%d] %s\n",
		  bufferData.pDBWinBuffer->dwProcessId,
		  bufferData.pDBWinBuffer->data);
	  SetEvent(hBufferReadyEvent);
	}
    }

  closeDBWinBufferData(&bufferData);
  CloseHandle(hBufferReadyEvent);
  CloseHandle(hDataReadyEvent);
  return 0;
}
