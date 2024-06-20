
void FileTransfer_BeginUpload(const char* szFilename, int iFirstLine = 0, int iLastLine = -1, bool bAppendSync = false);
void FileTransfer_BeginUploadFromMemory(std::string* str);
int FileTransfer_DoTransfer();