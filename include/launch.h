typedef struct stDataLaunch
{
      unsigned int  dwLaunchDataType;
      char *        TitleID;
      char          szLaunchPath[207];
      unsigned int  dwFlags;
      char          pad[0x1eb];
      char          LaunchData[0xBFF];
} stLauncher;// End stDataLaunch struct
typedef unsigned int wchar_t;
typedef struct stUNICODE_STRING
{

  unsigned short Length;
  unsigned short MaximumLength;
  wchar_t *  Buffer;

} UNICODE_STRING;// End stUNICODE_STRING struct

void LaunchXBE(char * szDevice, char * szDir, char * szTitle, void * pDashData );
void runXBE (char* szDevice, char* szDir, char* szFile);
void UnmountD();
void MountD(char* szDevice, char* szDir);
