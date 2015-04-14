/*
 *  Copyright (C) 2014 ThingWorx Inc.
 *
 *  Portable ThingWorx File Manager Layer
 */

#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include "twOSPort.h"
#include "twDefinitions.h"
#include "twDefaultSettings.h"
#include "list.h"
#include "twInfoTable.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************/
/*  File Transfer Information struct   */
/***************************************/
typedef struct twFileTransferInfo {
	char * sourceRepository;
	char * sourcePath;
	char * sourceFile;
	char * sourceChecksum;
	char * targetRepository;
	char * targetPath;
	char * targetFile;
	char * targetChecksum;
	DATETIME startTime;
	DATETIME endTime;
	int32_t duration;
	char * state;
	char isComplete;
	double size;
	char * transferId;
	char * user;
	char * message;
} twFileTransferInfo;

/*
twFileTransferInfo_Create - Creates a file transfer info structure
Parameters:
	it - infotable that contains the data
Return:
	twFileTransferInfo * - pointer to the allocated structure, 0 if an error occurred
*/
twFileTransferInfo * twFileTransferInfo_Create(twInfoTable * it);

/*
twFileTransferInfo_Delete - Delete the file transfer info structure
Parameters:
	transferInfo - pointer to the structure to delete
Return:
	Nothing
*/
void twFileTransferInfo_Delete(void * transferInfo);

/*
file_cb - signature of a callback function that is registered to be called when a file transfer completes either successfully
               or unsuccessfully.
Parameters:
    fileRcvd - (Input) Boolean.  TRUE is the file was received, FALSE if it was being sent.
	info - (Input) Pointer to the file transfer info structure.  Calling function retains ownership of this pointer
	               so the callback funciton must not delte it.
    userdata - (Input) Opaque pointer that was passed in during registration
Return:
	none
*/
typedef void (*file_cb) (char fileRcvd, twFileTransferInfo * info, void * userdata);

/***************************************/
/* Portable File/Directory data struct */
/***************************************/
typedef struct twFile {
	char * name;
	char * realPath;
	char * virtualPath;
	DATETIME  lastModified; 
	char readOnly;
	uint64_t size;
	TW_FILE_HANDLE handle;
	char isDir;
	uint64_t lastFileXferActivity;
	char * tid;
	char openForRead;
} twFile;

/*
twFileManager_Create - Creates a file manager singleton
Parameters:
	None
Return:
	int - 0 if successful, positive integral error code (see twErrors.h) if an error was encountered
*/
int twFileManager_Create();

/*
twFileManager_Delete - Delete the file manager singleton
Parameters:
	None
Return:
	int - 0 if successful, positive integral error code (see twErrors.h) if an error was encountered
*/
int twFileManager_Delete();

/*
twFileManager_AddVirtualDir - Adds a virtual directory to the fileManager.  A named virutal directory is
          asscoiated with a specific Thing and has a specific path in the underlying filesystem. There can
		  be more than one virtual directory assigned to a single Thing
Parameters:
	thingName - (Input) name of the Thing that this virtual directory is associated with.  
	dirName - (Input) name to apply to this virtual directory.  
	path - (Input) the abolute path to the underlying directory in the filesystem.  
Return:
	int - 0 if successful, positive integral error code (see twErrors.h) if an error was encountered
*/
int twFileManager_AddVirtualDir(const char * thingName, char * dirName, char * path);

/*
twFileManager_RemoveVirtualDir - Removes a virtual directory from the fileManager. 
Parameters:
	thingName - (Input) name of the Thing that this virtual directory is associated with.  
	dirName - (Input) name of the virtual directory to remove.  
Return:
	int - 0 if successful, positive integral error code (see twErrors.h) if an error was encountered
*/
int twFileManager_RemoveVirtualDir(const char * thingName, char * dirName);

/*
twFileManager_ListVirtualDirs - Returns a list of virtual directories from the fileManager. 
Parameters:
	None 
Return:
	twList * - a list of twFile * representing the the virutal directories or 0 if an error occured.
	           The calling function owns this pointer and must delete it
*/
twList * twFileManager_ListVirtualDirs();


/*
twFileManager_OpenFile - Gets a file/directory info and populates the structure. Checks to make sure
              the path is allowed - either in the root or in a virutal dir.  This also
			  opens the file to provide a OS specific reference or handle for any I/O
Parameters:
	thingName - (Input) name of the Thing this file is associated with.  
	path - (Input) Virutal path to the file or directory
	filename - (Input) name of the file or directory.  
	mode - (Inpput) mode to open the file in (see C fopen defintion)
Return:
	twFile * - pointer to the allocated structure or NULL if error
*/
twFile * twFileManager_OpenFile(const char * thingName,  const char * path, const char * filename, char * mode);

/*
twFileManager_CloseFile - Closes the file and frees all memory associated with a the structure.  
Parameters:
	f - (Input) pointer the twFile to close.  
Return:
	Nothing
*/
void twFileManager_CloseFile(void * file);

/*
twFileManager_GetOpenFile - Checks to see if the file specified is open already.  If it is, return
                            the pointer for the twFile ptr.  If not return NULL.
Parameters:
	thingName - (Input) name of the Thing this file is associated with.  
	path - (Input) full path to the file or directory. May be NULL if tid is used. 
	filename - (Input) name of the file
	tid - (Input) the tid of an open transaction. May be NULL if path is used.
Return:
	twFile * - pointer to the already opened file structure or NULL if error
*/
twFile * twFileManager_GetOpenFile(const char * thingName, const char * path, const char * filename, const char * tid);

/*
twFileManager_GetRealPath - Gets the native file system path for a file.
Parameters:
	thingName - (Input) name of the Thing that this file is associated with.  
	path - (Input) virtual path of the file in question
	filename - (Input) name of the file in question. If NULL, the file name is already appended to the path.
Return:
	char * - the native filesystem path ot this file.  The calling function retains ownership of this pointer.
*/
char * twFileManager_GetRealPath(const char * thingName, const char * path, const char * filename);

/*
twFileManager_MakeFileCallback - Makes the file complete callback
Parameters:
	rcvd - (Input) Boolean.  TRUE if the file was received, FALSE if it was sent.
	fti - (Input) Pointer to file transfer information structure.
Return:
	Nothing
*/
void twFileManager_MakeFileCallback(char rcvd, twFileTransferInfo * fti);

/*
twFileManager_RegisterFileCallback - Registers a function to be called whan a file transfer completes
Parameters:
	cb - (Input) the function to call when a file is received
	filter - (Input) a wildcard supprting filter to apply to the filename.  Transfers of file that 
	                 match the filter will result in a callback
    onceOnly - (Input) If TRUE, this registration will be deletted after it is called the first time
    userdata - (Input) an opaque void pointer that can be used for any purpose
Return:
	int - 0 if successful, positive integral error code (see twErrors.h) if an error was encountered
*/
int twFileManager_RegisterFileCallback(file_cb cb, char * filter, char onceOnly, void * userdata);

/*
twFileManager_UnregisterFileCallback - Unregisters a file callback function
Parameters:
	cb - (Input) the function to call when a file is received
	filter - (Input) a wildcard supprting filter to apply to the filename. 
    userdata - (Input) an opaque void pointer that can be used for any purpose
Return:
	int - 0 if successful, positive integral error code (see twErrors.h) if an error was encountered
*/
int twFileManager_UnregisterFileCallback(file_cb cb, char * filter, void * userdata);

/*
twFileManager_SendFile - Send a file to the server
Parameters:
    sourceRepo - (Input) the entityName to use when sending the file (and looking up the virtual path)
	sourcePath - (Input) the VIRTUAL path to the file to send (not including the file name)
	sourceFile - (Input) name of the file to send
	targetRepo - (Input) target respostiory of the file
	targetPath - (Input) path of the resulting file in the target repo (not including the file name)
	targetFile - (Input) name of the resulting file at in the target directory
	timeout - (Input) timeout, in seconds, for the transfer.  A zero will use the systems default timeout.
	asynch - (Input) Boolean - if TRUE return immediately and call a callback function when the transfer is complete
	              if FALSE, block until the transfer is complete. Note that the file callback function will be called 
				  in any case.  Note that it is up to the caller to pre-register and unregister the callback.
    tid - (Output) pointer to the Transfer ID that is being used for this transfer
Return:
	int - 0 if successful, positive integral error code (see twErrors.h) if an error was encountered
*/
int twFileManager_SendFile(const char * sourceRepo, const char * sourcePath, const char * sourceFile,
						   const char * targetRepo, const char * targetPath, const char * targetFile,
						   uint32_t timeout, char asynch, char ** tid);

/*
twFileManager_GetFile - Get a file from the server
Parameters:
    sourceRepo - (Input) the entityName to get the file from
	sourcePath - (Input) the path to the file to get
	sourceFile - (Input) name of the file to get
	targetRepo - (Input) target respostiory (usually a ThingName) of the file
	targetPath - (Input) the local VIRTUAL path of the resulting file  (not including the file name)
	targetFile - (Input) name of the resulting file at in the target directory
	timeout - (Input) timeout, in seconds, for the transfer.  A zero will use the systems default timeout.
	asynch - (Input) Boolean - if TRUE return immediately and call a callback function when the transfer is complete
	              if FALSE, block until the transfer is complete. Note that the file callback function will be called 
				  in any case.  Note that it is up to the caller to pre-register and unregister the callback.
    tid - (Output) pointer to the Transfer ID that is being used for this transfer
Return:
	int - 0 if successful, positive integral error code (see twErrors.h) if an error was encountered
*/
int twFileManager_GetFile(const char * sourceRepo, const char * sourcePath, const char * sourceFile,
						   const char * targetRepo, const char * targetPath, const char * targetFile,
						   uint32_t timeout, char asynch, char ** tid);

/*
twFileManager_CheckStalledTransfers - Checks for any stalled transfers and deletes them.
Parameters:
	None
Return:
	Nothing
*/
void twFileManager_CheckStalledTransfers();


/*
twFileManager_ListEntities - List the files or subdirectories in a directory
Parameters:
    entityName - (Input) the entityName to use when looking up a virtual directory path
	path - (Input) the VIRTUAL path to the directory to list
	namemask - (Input) A wildcard enable mask to list only files that match the mask
	returnType - (Input) LIST_ALL (0) - Return all, LIST_FILES (1) - Return files only, LIST_DIRS (2) - Return directories only
Return:
	twList * - a list of twFile * representing the the files or subdirectories or 0 if an error occured.
	           The calling function owns this pointer and must delete it
*/
twList * twFileManager_ListEntities(const char * entityName, const char * path, const char * namemask, char returnType);

#ifdef __cplusplus
}
#endif

#endif
