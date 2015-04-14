/*
 *  Copyright (C) 2014 ThingWorx Inc.
 *
 *  Portable ThingWorx File Transfer
 */

#include "twFileManager.h"
#include "twLogger.h"
#include "twInfoTable.h"
#include "twDefinitions.h"
#include "twApi.h"
#include "stringUtils.h"
#include "wildcard.h"
#include "twMD5.h"

/********************************/
/*       Helper Functions       */
/********************************/
void cleanUpPaths(char **path, char **realPath, char **sourcePath, char **realSourcePath, char **targetPath, char **realTargetPath) {
	if (path && *path) {
		TW_FREE(*path);
		*path = NULL;
	}
	if (realPath && *realPath) {
		TW_FREE(*realPath);
		*realPath = NULL;
	}
	if (sourcePath && *sourcePath) {
		TW_FREE(*sourcePath);
		*sourcePath = NULL;
	}
	if (realSourcePath && *realSourcePath) {
		TW_FREE(*realSourcePath);
		*realSourcePath = NULL;
	}
	if (targetPath && *targetPath) {
		TW_FREE(*targetPath);
		*targetPath = NULL;
	}
	if (realTargetPath && *realTargetPath) {
		TW_FREE(*realTargetPath);
		*realTargetPath = NULL;
	}
}

int getPaths(twInfoTable * params, const char * entityName, char ** path, char ** realPath,
			 char ** sourcePath, char ** realSourcePath, char ** targetPath, char ** realTargetPath) {
	if (!params || !entityName) return TW_INVALID_PARAM;
	if (path) {
		if (twInfoTable_GetString(params, "path", 0, path)) return TW_BAD_REQUEST;
		if (realPath) {
			*realPath =  twFileManager_GetRealPath(entityName, *path, NULL);
			if (!realPath) {
				cleanUpPaths(path, realPath, sourcePath, realSourcePath, targetPath, realTargetPath);
				return TW_BAD_REQUEST;
			}
		}
	}
	if (sourcePath) {
		if (twInfoTable_GetString(params, "sourcePath", 0, sourcePath)) {
			cleanUpPaths(path, realPath, sourcePath, realSourcePath, targetPath, realTargetPath);
			return TW_BAD_REQUEST;
		}
		if (realSourcePath) {
			*realSourcePath =  twFileManager_GetRealPath(entityName, *sourcePath, NULL);
			if (!realSourcePath) {
				cleanUpPaths(path, realPath, sourcePath, realSourcePath, targetPath, realTargetPath);
				return TW_BAD_REQUEST;
			}
		}
	}
	if (targetPath) {
		if (twInfoTable_GetString(params, "targetPath", 0, targetPath)) {
			cleanUpPaths(path, realPath, sourcePath, realSourcePath, targetPath, realTargetPath);
			return TW_BAD_REQUEST;
		}
		if (realTargetPath) {
			*realTargetPath =  twFileManager_GetRealPath(entityName, *targetPath, NULL);
			if (!realTargetPath) {
				cleanUpPaths(path, realPath, sourcePath, realSourcePath, targetPath, realTargetPath);
				return TW_BAD_REQUEST;
			}
		}
	}
	return TW_OK;
}

int listVirtualDirsInInfoTable(twInfoTable * it, char fullDetails) {
	twList * list = NULL;
	ListEntry * le = NULL;
	char fileType[8];
	twInfoTableRow * row = NULL;
	list = twFileManager_ListVirtualDirs();
    if (!it || !list) {
		TW_LOG(TW_ERROR, "listVirtualDirsInInfoTable: Invalid input param(s) of could not get list");
		return TW_INVALID_PARAM;
	}
	le = twList_Next(list, NULL);
	while (le && le->value) {
		char * fullPath = NULL;
		twFile * tmp = (twFile *)(le->value);
		strcpy(fileType,"D");
		/* Fill in the info table row for this entry */
		TW_LOG(TW_TRACE,"listVirtualDirsInInfoTable: Adding dir %s%c%s to list.", tmp->realPath, TW_FILE_DELIM, tmp->name);
		row = twInfoTableRow_Create(twPrimitive_CreateFromString(tmp->name, TRUE));
		if (!row) {
			TW_LOG(TW_ERROR, "listVirtualDirsInInfoTable: Error allocating infotable row");
			twInfoTable_Delete(it);
			it = 0;
			twList_Delete(list);
			return TW_ERROR_ALLOCATING_MEMORY;
		}
		fullPath = (char *)TW_CALLOC(strlen(tmp->name) + 2, 1);
		if (fullPath) {
			strcpy(fullPath,"/");
			strcat(fullPath, tmp->name);
		}
		twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString(fullPath, FALSE));
		if (fullDetails) {
			twInfoTableRow_AddEntry(row, twPrimitive_CreateFromNumber(tmp->size));
			twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString(tmp->readOnly ? "D+RO" : "D", TRUE));
			twInfoTableRow_AddEntry(row, twPrimitive_CreateFromDatetime(tmp->lastModified));
		} else {
			twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString("/", TRUE));
		}
		if (twInfoTable_AddRow(it, row)) {
			TW_LOG(TW_ERROR, "listVirtualDirsInInfoTable: Error adding infotable row");
			twInfoTable_Delete(it);
			it = 0;
			twList_Delete(list);
			return TW_ERROR_ALLOCATING_MEMORY;
		}
		le = twList_Next(list, le);
	}
	twList_Delete(list);
	return 0;
}

enum msgCodeEnum twListEntities(const char * entityName, twInfoTable * params, twInfoTable ** content, char files) {
	twDataShapeEntry * dse = NULL;
	twDataShape * ds = NULL;
	/* Inouts */
	char * path = NULL;
	char * realPath = NULL;
	char * nameMask = NULL;
    if (getPaths(params, entityName, &path, &realPath, NULL, NULL, NULL, NULL)) {
		TW_LOG(TW_ERROR, "twListEntities: Invalid input param(s)");
		return TWX_BAD_REQUEST;
	}
	if (files) {
		twInfoTable_GetString(params, "nameMask", 0, &nameMask);
		if (!nameMask) nameMask = duplicateString("*");
	}
	/* Outputs */
	dse = twDataShapeEntry_Create("name","",TW_STRING);
	if (!dse) {
		TW_LOG(TW_ERROR, "twListEntities: Error creating output datashape entry");
		cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
		if (nameMask) TW_FREE(nameMask);
		return TWX_INTERNAL_SERVER_ERROR;
	}
	twDataShapeEntry_AddAspect(dse,"isPrimaryKey", twPrimitive_CreateFromBoolean(TRUE));
	ds = twDataShape_Create(dse);
	if (!ds) {
		TW_LOG(TW_ERROR, "twListEntities: Error creating output datashape");
		twDataShapeEntry_Delete(dse);
		cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
		if (nameMask) TW_FREE(nameMask);
		return TWX_INTERNAL_SERVER_ERROR;
	}
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("path","",TW_STRING));
	if (files) {
		twDataShape_AddEntry(ds, twDataShapeEntry_Create("size","",TW_NUMBER));
		twDataShape_AddEntry(ds, twDataShapeEntry_Create("fileType","",TW_STRING));
		twDataShape_AddEntry(ds, twDataShapeEntry_Create("lastModifiedDate","",TW_DATETIME));
	} else {
		twDataShape_AddEntry(ds, twDataShapeEntry_Create("parentPath","",TW_STRING));
	}
    /* Create the output infotable */
	*content = twInfoTable_Create(ds);
	if (!*content) {
		TW_LOG(TW_ERROR, "twListEntities: Error creating output infotable");
		twDataShape_Delete(ds);
		cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
		if (nameMask) TW_FREE(nameMask);
		return TWX_INTERNAL_SERVER_ERROR;
	}
	/* Perform the function */
	{
		twInfoTableRow * row = NULL;
		TW_DIR hnd = 0;
		char fileType[8];
		twFile tmp;
		int res = 0;
		char * fullPath = NULL;
		memset(&tmp, 0, sizeof(twFile));
		/* Handle the case of "/" */
		if (strlen(path) == 1 && (path[0] == '/' || path[0] == '\\')) {
			if (nameMask) TW_FREE(nameMask);
			if (files) {
				/* There are no files at the virtual root directory */
				cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
				return TWX_SUCCESS;
			}
			res = listVirtualDirsInInfoTable(*content, FALSE);
			if (res) {
				twInfoTable_Delete(*content);
				*content = NULL;
				cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
				return TWX_INTERNAL_SERVER_ERROR;
			}
			cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
			return TWX_SUCCESS;
		}
		if (path[strlen(path) - 1] == '/' || path[strlen(path) - 1] == '\\') path[strlen(path) - 1] = 0;
		hnd = twDirectory_IterateEntries(realPath, hnd, &tmp.name, &tmp.size, 
											&tmp.lastModified, &tmp.isDir, &tmp.readOnly);
		while (hnd && tmp.name) {
			if ((files && !tmp.isDir) || (!files && tmp.isDir)) {
				if ((files && IsWildcardMatch(nameMask, tmp.name, TRUE)) || ((tmp.isDir && strcmp(tmp.name, ".") && strcmp(tmp.name, "..")))) {
					strcpy(fileType,"F");
					/* Fill in the info table row for this entry */
					TW_LOG(TW_TRACE,"twListEntities: Adding file %s%c%s to list.", path, TW_FILE_DELIM, tmp.name);
					row = twInfoTableRow_Create(twPrimitive_CreateFromString(tmp.name, FALSE));
					if (!row) {
						TW_LOG(TW_ERROR, "twListEntities: Error allocating infotable row");
						TW_FREE(tmp.name);
						twInfoTable_Delete(*content);
						*content = NULL;
						cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
						if (nameMask) TW_FREE(nameMask);
						return TWX_INTERNAL_SERVER_ERROR;
					}
					fullPath = (char *)TW_CALLOC(strlen(path) + strlen(tmp.name) + 2, 1);
					if (fullPath) {
						strcpy(fullPath, path);
						strcat(fullPath,"/");
						strcat(fullPath, tmp.name);
					}
					twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString(fullPath, FALSE));
					if (files) {
						twInfoTableRow_AddEntry(row, twPrimitive_CreateFromNumber((double)tmp.size));
						if (tmp.readOnly) strcat(fileType, "+RO");
						twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString(fileType, TRUE));
						twInfoTableRow_AddEntry(row, twPrimitive_CreateFromDatetime(tmp.lastModified));
					} else {
						twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString(path, TRUE));
					}
					if (twInfoTable_AddRow(*content, row)) {
						TW_LOG(TW_ERROR, "twListEntities: Error adding infotable row");
						twInfoTableRow_Delete(row);
						twInfoTable_Delete(*content);
						*content = NULL;
						cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
						if (nameMask) TW_FREE(nameMask);
						return TWX_INTERNAL_SERVER_ERROR;
					}
				} else {
					TW_FREE(tmp.name);
				}
			} else TW_FREE(tmp.name);
			hnd = twDirectory_IterateEntries(realPath, hnd, &tmp.name, &tmp.size, 
				&tmp.lastModified, &tmp.isDir, &tmp.readOnly);
		}
		res = twDirectory_GetLastError();
		if (res != ERROR_NO_MORE_FILES) {
			TW_LOG(TW_ERROR, "twListEntities: Error iterating thorugh %s.  Error: %d", path, res);
		}
	} 
	cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
	if (nameMask) TW_FREE(nameMask);
	return TWX_SUCCESS;
}

/********************************/
/*     Service Callbacks        */
/********************************/
enum msgCodeEnum twBrowseDirectory(const char * entityName, twInfoTable * params, twInfoTable ** content) {
	/* Inouts */
	char * path = NULL;
	char * realPath = NULL;
	twDataShapeEntry * dse = NULL;
	twDataShape * ds = NULL;
    if (getPaths(params, entityName, &path, &realPath, NULL, NULL, NULL, NULL)) {
		/* Handle the case of "/" */
		if (!path || strlen(path) > 1) {
			TW_LOG(TW_ERROR, "twBrowseDirectory: Invalid input param(s)");
			return TWX_BAD_REQUEST;
		}
	}
	/* Outputs */
	dse = twDataShapeEntry_Create("name","",TW_STRING);
	if (!dse) {
		TW_LOG(TW_ERROR, "twBrowseDirectory: Error creating output datashape entry");
		cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
		return TWX_INTERNAL_SERVER_ERROR;
	}
	twDataShapeEntry_AddAspect(dse,"isPrimaryKey", twPrimitive_CreateFromBoolean(TRUE));
	ds = twDataShape_Create(dse);
	if (!ds) {
		TW_LOG(TW_ERROR, "twBrowseDirectory: Error creating output datashape");
		twDataShapeEntry_Delete(dse);
		cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
		return TWX_INTERNAL_SERVER_ERROR;
	}
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("path","",TW_STRING));
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("size","",TW_NUMBER));
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("fileType","",TW_STRING));
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("lastModifiedDate","",TW_DATETIME));
    /* Create the output infotable */
	*content = twInfoTable_Create(ds);
	if (!*content) {
		TW_LOG(TW_ERROR, "twBrowseDirectory: Error creating output infotable");
		twDataShape_Delete(ds);
		cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
		return TWX_INTERNAL_SERVER_ERROR;
	}
	/* Perform the function */
	{
		/* Handle the case of "/" */
		if (strlen(path) == 1 && (path[0] == '/' || path[0] == '\\')) {
			int res = 0;
			res = listVirtualDirsInInfoTable(*content, TRUE);
			if (res) {
				twInfoTable_Delete(*content);
				*content = NULL;
				cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
				return TWX_INTERNAL_SERVER_ERROR;
			}
			cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
			return TWX_SUCCESS;
		} else {
			/* Need to iterate through the directory */
			twInfoTableRow * row = 0;
			TW_DIR hnd = 0;
			char fileType[8];
			twFile * tmp = (twFile *)TW_CALLOC(sizeof(twFile), 1);
			hnd = twDirectory_IterateEntries(realPath, hnd, &tmp->name, &tmp->size, 
											   &tmp->lastModified, &tmp->isDir, &tmp->readOnly);
			while (tmp && hnd) {
				if (!strcmp(tmp->name,".") || !strcmp(tmp->name,"..")) {
					TW_FREE(tmp->name);
					hnd = twDirectory_IterateEntries(realPath, hnd, &tmp->name, &tmp->size, 
								&tmp->lastModified, &tmp->isDir, &tmp->readOnly);
					continue;
				}
				strcpy(fileType,"F");
				/* Fill in the info table row for this entry */
				TW_LOG(TW_TRACE,"twBrowseDirectory: Adding dir %s%c%s to list.", tmp->realPath, TW_FILE_DELIM, tmp->name);
				row = twInfoTableRow_Create(twPrimitive_CreateFromString(tmp->name, FALSE));
				if (!row) {
					TW_LOG(TW_ERROR, "twBrowseDirectory: Error allocating infotable row");
					TW_FREE(tmp->name);
					TW_FREE(tmp);
					twInfoTable_Delete(*content);
					*content = NULL;
					cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
					return TWX_INTERNAL_SERVER_ERROR;
				}
				twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString(path, TRUE));
				twInfoTableRow_AddEntry(row, twPrimitive_CreateFromNumber((double)tmp->size));
				if (tmp->isDir) fileType[0] = 'D';
				if (tmp->readOnly) strcat(fileType, "+RO");
				twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString(fileType, TRUE));
				twInfoTableRow_AddEntry(row, twPrimitive_CreateFromDatetime(tmp->lastModified));
				if (twInfoTable_AddRow(*content, row)) {
					TW_LOG(TW_ERROR, "twBrowseDirectory: Error adding infotable row");
					twInfoTable_Delete(*content);
					*content = NULL;
					cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
					TW_FREE(tmp);
					return TWX_INTERNAL_SERVER_ERROR;
				}
				hnd = twDirectory_IterateEntries(realPath, hnd, &tmp->name, &tmp->size, 
					&tmp->lastModified, &tmp->isDir, &tmp->readOnly);
			}
			TW_FREE(tmp);
		}
	}
	cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
	return TWX_SUCCESS;
}

enum msgCodeEnum twDeleteFile(const char * entityName, twInfoTable * params, twInfoTable ** content) {
	int res;
	/* Inputs */ 
	char * path = NULL;
	char * realPath = NULL;
    if (getPaths(params, entityName, &path, &realPath, NULL, NULL, NULL, NULL)) {
		TW_LOG(TW_ERROR, "twDeleteFile: Invalid input param(s)");
		return TWX_BAD_REQUEST;
	}
	/* Outputs */
	*content = NULL;
	/* Perform the function */
	if (!twDirectory_FileExists(realPath)) {
		TW_LOG(TW_WARN, "twDeleteFile: File %s doesn't exist.  Nothing to do", path);
		cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
		return TWX_SUCCESS;
	}
	TW_LOG(TW_DEBUG, "twDeleteFile: Deleting file %s", realPath);
	res = twDirectory_DeleteFile(realPath);
	if (res) {
		TW_LOG(TW_ERROR, "twDeleteFile: Error deleting file %s.  Error: %d",realPath, res);
		cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
		return TWX_INTERNAL_SERVER_ERROR;
	}
	cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
	return TWX_SUCCESS;
}

enum msgCodeEnum twGetFileInfo(const char * entityName, twInfoTable * params, twInfoTable ** content) {
	int res = 0;
	twInfoTableRow * row = NULL;
	twDataShapeEntry * dse = NULL;
	twDataShape * ds = NULL;
	/* Inputs */ 
	char * path = NULL;
	char * realPath = NULL;
    if (getPaths(params, entityName, &path, &realPath, NULL, NULL, NULL, NULL)) {
		TW_LOG(TW_ERROR, "twGetFileInfo: Invalid input param(s)");
		return TWX_BAD_REQUEST;
	}
	/* Outputs */
	dse = twDataShapeEntry_Create("name","",TW_STRING);
	if (!dse) {
		TW_LOG(TW_ERROR, "twGetFileInfo: Error creating output datashape entry");
		cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
		return TWX_INTERNAL_SERVER_ERROR;
	}
	twDataShapeEntry_AddAspect(dse,"isPrimaryKey", twPrimitive_CreateFromBoolean(TRUE));
	ds = twDataShape_Create(dse);
	if (!ds) {
		TW_LOG(TW_ERROR, "twGetFileInfo: Error creating output datashape");
		cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
		twDataShapeEntry_Delete(dse);
		return TWX_INTERNAL_SERVER_ERROR;
	}
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("path","",TW_STRING));
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("size","",TW_NUMBER));
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("fileType","",TW_STRING));
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("lastModifiedDate","",TW_DATETIME));
    /* Create the output infotable */
	*content = twInfoTable_Create(ds);
	if (!*content) {
		TW_LOG(TW_ERROR, "twGetFileInfo: Error creating output infotable");
		twDataShape_Delete(ds);
		cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
		return TWX_INTERNAL_SERVER_ERROR;
	}
	/* Perform the function */ 
	if (!twDirectory_FileExists(realPath)) {
		TW_LOG(TW_WARN, "twGetFileInfo: File %s doesn't exist.", path);
		cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
		twInfoTable_Delete(*content);
		*content = NULL;
		return TWX_NOT_FOUND;
	}
	{
	/* New scope for these variable declarations */
	uint64_t size;
	char isDirectory;
	char isReadOnly;
	DATETIME lastModified;
	char fileType[8];
	char * tmp;
	memset(fileType, 0, 8);
	TW_LOG(TW_DEBUG, "twGetFileInfo: Getting info for file %s", path);
	res = twDirectory_GetFileInfo(realPath, &size, &lastModified, &isDirectory, &isReadOnly);
	if (res) {
		TW_LOG(TW_ERROR, "twGetFileInfo: Error getting info for file %s.  Error: %d",realPath, res);
		cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
		twInfoTable_Delete(*content);
		*content = NULL;
		return TWX_INTERNAL_SERVER_ERROR;
	}
	/* Get the file name */
	tmp = strrchr(path,'/');
	if (!tmp) tmp = strrchr(path,'\\');

	/* Fill in the info table */
	row = twInfoTableRow_Create(twPrimitive_CreateFromString(tmp, TRUE));
	if (!row) {
		TW_LOG(TW_ERROR, "twGetFileInfo: Error allocating infotable row");
		twInfoTable_Delete(*content);
		*content = NULL;
		cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
		return TWX_INTERNAL_SERVER_ERROR;
	}
	twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString(path, TRUE));
	twInfoTableRow_AddEntry(row, twPrimitive_CreateFromNumber((double)size));
	fileType[0] = 'F';
	if (isDirectory) fileType[0] = 'D';
	if (isReadOnly) strcat(fileType, "+RO");
	twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString(fileType, TRUE));
	twInfoTableRow_AddEntry(row, twPrimitive_CreateFromDatetime(lastModified));
	twInfoTable_AddRow(*content, row);
	}
	cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
	return TWX_SUCCESS;
}

enum msgCodeEnum twListFiles(const char * entityName, twInfoTable * params, twInfoTable ** content) {
	return twListEntities(entityName, params, content, TRUE);
}

enum msgCodeEnum twMoveFile(const char * entityName, twInfoTable * params, twInfoTable ** content) {
	int res;
	/* Inputs */ 
	char * sourcePath = NULL;
	char * targetPath = NULL;
	char overwrite = 0;
	char * realSourcePath = NULL;
	char * realTargetPath = NULL;
	twFile * f = NULL;
	if (getPaths(params, entityName, NULL, NULL, &sourcePath, &realSourcePath, &targetPath, &realTargetPath)) {
		TW_LOG(TW_ERROR, "twCreateBinaryFile: Invalid input param(s)");
		return TWX_BAD_REQUEST;
	}
	twInfoTable_GetBoolean(params, "overwrite", 0, &overwrite);
	if (!overwrite && twDirectory_FileExists(realTargetPath)) {
		TW_LOG(TW_ERROR, "twMoveFile: Target file %s exists and 'overwrite' is FALSE", targetPath);
		cleanUpPaths(NULL, NULL, &sourcePath, &realSourcePath, &targetPath, &realTargetPath);
		return TWX_PRECONDITION_FAILED;
	}
	/* We likely have the source file open, so we need to close it before moving */
	f = twFileManager_GetOpenFile(entityName, sourcePath, NULL, NULL);
	if (f && f->handle) {
		TW_FCLOSE(f->handle);
		f->handle = 0;
	}
	res = twDirectory_MoveFile(realSourcePath,realTargetPath);
	TW_LOG(TW_DEBUG, "twMoveFile: Moving file %s to %s", sourcePath, targetPath);
	if (res) {
		TW_LOG(TW_ERROR, "twMoveFile: Error moving file %s to %s. Error: %d",sourcePath, targetPath, res);
		cleanUpPaths(NULL, NULL, &sourcePath, &realSourcePath, &targetPath, &realTargetPath);
		return TWX_INTERNAL_SERVER_ERROR;
	}
	cleanUpPaths(NULL, NULL, &sourcePath, &realSourcePath, &targetPath, &realTargetPath);
	return TWX_SUCCESS;
}

enum msgCodeEnum twGetTransferInfo(const char * entityName, twInfoTable * params, twInfoTable ** content) {
	/* Generic function across the board for all enitites */
	int res;
	twInfoTableRow * row = NULL;
	/* Inputs */ 

	/* Outputs */
	twDataShape * ds = twDataShape_Create(twDataShapeEntry_Create("blockSize","",TW_INTEGER));
	if (!ds) {
		TW_LOG(TW_ERROR, "GetTransferInfo: Error creating output datashape");
		return TWX_INTERNAL_SERVER_ERROR;
	}
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("maxFileSize","",TW_NUMBER));
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("stagingDir","",TW_STRING));
    /* Create and fill in the output infotable */
	*content = twInfoTable_Create(ds);
	if (!*content) {
		TW_LOG(TW_ERROR, "GetTransferInfo: Error creating output infotable");
		twDataShape_Delete(ds);
		return TWX_INTERNAL_SERVER_ERROR;
	}
	row = twInfoTableRow_Create(twPrimitive_CreateFromInteger(FILE_XFER_BLOCK_SIZE));
	if (!row) {
		TW_LOG(TW_ERROR, "GetTransferInfo: Error creating output infotable row");
		twInfoTable_Delete(*content);
		*content = NULL;
		return TWX_INTERNAL_SERVER_ERROR;
	}
	res = twInfoTableRow_AddEntry(row, twPrimitive_CreateFromNumber((double)FILE_XFER_MAX_FILE_SIZE));
	res |= twInfoTableRow_AddEntry(row, twPrimitive_CreateFromString(TW_VIRTUAL_STAGING_DIR, TRUE));
	res |= twInfoTable_AddRow(*content, row);
	if (res) {
		TW_LOG(TW_ERROR, "GetTransferInfo: Error creating populating output infotable row");
		twInfoTable_Delete(*content);
		*content = NULL;
		return TWX_INTERNAL_SERVER_ERROR;
	}
	return TWX_SUCCESS;
}

enum msgCodeEnum twGetFileChecksum(const char * entityName, twInfoTable * params, twInfoTable ** content) {
	TW_FILE_HANDLE f = 0;
	twDataShape * ds = NULL;
	twInfoTableRow * row = NULL;
	twFile * file = NULL;
	/* Inputs */ 
	char * path = NULL;
	char * realPath = NULL;
    if (getPaths(params, entityName, &path, &realPath, NULL, NULL, NULL, NULL)) {
		TW_LOG(TW_ERROR, "twGetFileChecksum: Invalid input param(s)");
		return TWX_BAD_REQUEST;
	}
	/* Outputs */
	ds = twDataShape_Create(twDataShapeEntry_Create("result","",TW_STRING));
	if (!ds) {
		TW_LOG(TW_ERROR, "twGetFileChecksum: Error creating output datashape");
		cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
		return TWX_INTERNAL_SERVER_ERROR;
	}
    /* Create the output infotable */
	*content = twInfoTable_Create(ds);
	if (!*content) {
		TW_LOG(TW_ERROR, "twGetFileChecksum: Error creating output infotable");
		twDataShape_Delete(ds);
		cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
		return TWX_INTERNAL_SERVER_ERROR;
	}
	/* Perform the function */ 
	/* Check to see if the file is already open */
	file = twFileManager_GetOpenFile(entityName, path, NULL, NULL);
	if (file && file->handle) f = file->handle;
	else if (twDirectory_FileExists(realPath)) f = TW_FOPEN(realPath,"rb");
	if (f) {
		struct md5_state_s md5;
		char hexHash[48];
		char hex[4];
		int i = 0;
		int bytesRead = 0;
		md5_byte_t * buffer = (md5_byte_t *)TW_CALLOC(FILE_XFER_MD5_BLOCK_SIZE,1);
		md5_byte_t digest[16];
		memset(digest, 0, sizeof (digest));
		memset(&md5, 0, sizeof(struct md5_state_s));
		if (!buffer) {
			TW_LOG(TW_ERROR, "twGetFileChecksum: Error allocating buffer");
			twInfoTable_Delete(*content);
			*content = NULL;
			cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
			if (!file) TW_FCLOSE(f);
			return TWX_INTERNAL_SERVER_ERROR;
		}
		/* Start from the beginning of the file */
		TW_FSEEK(f,0,SEEK_SET);
		md5_init(&md5);
		bytesRead = TW_FREAD(buffer, 1, FILE_XFER_MD5_BLOCK_SIZE, f);
		while (bytesRead > 0) {
			md5_append(&md5, buffer, bytesRead);
			bytesRead = TW_FREAD(buffer, 1, FILE_XFER_MD5_BLOCK_SIZE, f);
		}
		md5_finish(&md5, digest);
		/* Make sure we didn't have an error */
		if (TW_FERROR(f)) {
			TW_LOG(TW_ERROR, "twGetFileChecksum: Error reading from file %s. Bytes Read: %d", realPath, bytesRead);
			TW_FREE(buffer);
			twInfoTable_Delete(*content);
			*content = NULL;
			cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
			if (!file) TW_FCLOSE(f);
			return TWX_INTERNAL_SERVER_ERROR;
		}
		/* Only close if we opened it */
		if (!file) TW_FCLOSE(f);
		/* Convert the hash to hex */
		memset(hexHash, 0, 48);
		memset(hex, 0, 4);
		for (i = 0; i <16; i++) {
			snprintf(hex, 3, "%02x", digest[i]);
			strcat(hexHash, hex);
		}
		row = twInfoTableRow_Create(twPrimitive_CreateFromString(hexHash, TRUE));
		if (!row) {
			TW_LOG(TW_ERROR, "twGetFileChecksum: Error creating infotable row");
			twInfoTable_Delete(*content);
			*content = NULL;
			cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);	
			return TWX_INTERNAL_SERVER_ERROR;
		}
		twInfoTable_AddRow(*content, row);
		TW_FREE(buffer);
		cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);	
		return TWX_SUCCESS;
	}
	TW_LOG(TW_WARN, "twGetFileChecksum: twFile for %s not found", path);
	twInfoTable_Delete(*content);
	*content = NULL;
	cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);	
	return TWX_NOT_FOUND;
}

enum msgCodeEnum twCreateBinaryFile(const char * entityName, twInfoTable * params, twInfoTable ** content) {
	int res;
	/* Inputs */ 
	char * path = NULL;
	char * realPath = NULL;
	char overwrite = FALSE;
	twInfoTable_GetBoolean(params, "overwrite", 0, &overwrite);
	if (getPaths(params, entityName, &path, &realPath, NULL, NULL, NULL, NULL)) {
		TW_LOG(TW_ERROR, "twCreateBinaryFile: Invalid input param(s)");
		return TWX_BAD_REQUEST;
	}
	/* Outputs */
	*content = NULL;
	/* Perform the function */
	if (!overwrite && twDirectory_FileExists(realPath)) {
		TW_LOG(TW_WARN, "CreateBinaryFile: File %s exists and 'overwrite' is FALSE.  Nothing to do", path);
		cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
		return TWX_SUCCESS;
	}
	TW_LOG(TW_DEBUG, "CreateBinaryFile: Creating file %s", path);
	res = twDirectory_CreateFile(realPath);
	if (res) {
		TW_LOG(TW_ERROR, "CreateBinaryFile: Error creating file %s. Error: %d", path, res);
		cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
		return TWX_INTERNAL_SERVER_ERROR;
	}
	cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
	return TWX_SUCCESS;
}

enum msgCodeEnum twReadFromBinaryFile(const char * entityName, twInfoTable * params, twInfoTable ** content) {
	twDataShape * ds = NULL;
	/* Inputs */
	char * path = NULL;
	char * realPath = NULL;
	double offset = 0;
	int32_t count = 0;
	twInfoTable_GetNumber(params, "offset", 0, &offset);
	twInfoTable_GetInteger(params, "count", 0, &count);
	if (getPaths(params, entityName, &path, &realPath, NULL, NULL, NULL, NULL)) {
		TW_LOG(TW_ERROR, "twReadFromBinaryFile: Invalid input param(s)");
		return TWX_BAD_REQUEST;
	}
	/* Outputs */
	ds = twDataShape_Create(twDataShapeEntry_Create("count","",TW_INTEGER));
	if (!ds) {
		TW_LOG(TW_ERROR, "twReadFromBinaryFile: Error creating output datashape");
		return TWX_INTERNAL_SERVER_ERROR;
	}
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("data","",TW_BLOB));
	twDataShape_AddEntry(ds, twDataShapeEntry_Create("eof","",TW_BOOLEAN));
    /* Create the output infotable */
	*content = twInfoTable_Create(ds);
	if (!*content) {
		TW_LOG(TW_ERROR, "twReadFromBinaryFile: Error creating output infotable");
		twDataShape_Delete(ds);
		return TWX_INTERNAL_SERVER_ERROR;
	}
	/* Execute the service */
	{
		twFile * f = NULL;
		char * buffer = NULL;
		int res = 0;
		twInfoTableRow * row = NULL;
		buffer = (char *)TW_CALLOC(count, 1);
		if (!buffer) {
			TW_LOG(TW_ERROR, "twReadFromBinaryFile: Error allocating buffer");
			twInfoTable_Delete(*content);
			*content = NULL;
			cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
			twInfoTableRow_Delete(row);
			return TWX_INTERNAL_SERVER_ERROR;
		}
		f = twFileManager_GetOpenFile(entityName, path, NULL, NULL);
		if (!f || !f->handle) {
			TW_LOG(TW_ERROR, "twReadFromBinaryFile: File is not open for reading");
			twInfoTable_Delete(*content);
			*content = NULL;
			cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
			twInfoTableRow_Delete(row);
			TW_FREE(buffer);
			return TWX_PRECONDITION_FAILED;
		}
		res = TW_FSEEK(f->handle, (uint64_t)offset, SEEK_SET);
		if (res) {
			TW_LOG(TW_ERROR, "twReadFromBinaryFile: Error seeking in file %s to %llu.  Error: %d", path, offset, twDirectory_GetLastError());
			twInfoTable_Delete(*content);
			*content = NULL;
			cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
			TW_FREE(buffer);
			return TWX_INTERNAL_SERVER_ERROR;
		}
		res = TW_FREAD(buffer, 1, count, f->handle);
		if (res > 0) {
			char eof = FALSE;
			row = twInfoTableRow_Create(twPrimitive_CreateFromNumber(res));
			if (!row) {
				TW_LOG(TW_ERROR, "twReadFromBinaryFile: Error creating output infotable row");
				twInfoTable_Delete(*content);
				*content = NULL;
				cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
				TW_FREE(buffer);
				return TWX_INTERNAL_SERVER_ERROR;
			}
			twInfoTableRow_AddEntry(row, twPrimitive_CreateFromBlob(buffer, res, FALSE, FALSE));
			/* Check for EOF, including corner case */
			if (res < count && (offset + count == f->size)) eof = TRUE;
			twInfoTableRow_AddEntry(row, twPrimitive_CreateFromBoolean(eof));
			twInfoTable_AddRow(*content, row);
			cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
			return TWX_SUCCESS;
		} else {
			TW_LOG(TW_ERROR, "twReadFromBinaryFile: Error reading from file %s", path);
			twInfoTable_Delete(*content);
			*content = NULL;
			cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
			twInfoTableRow_Delete(row);
			TW_FREE(buffer);
			return TWX_INTERNAL_SERVER_ERROR;
		}
	}
}

enum msgCodeEnum twWriteToBinaryFile(const char * entityName, twInfoTable * params, twInfoTable ** content) {
	/* Inputs */
	char * path = 0;
	char * realPath = 0;
	double offset = 0;
	char * data = 0;
	int32_t count = 0;
	if (getPaths(params, entityName, &path, &realPath, NULL, NULL, NULL, NULL)) {
		TW_LOG(TW_ERROR, "twWriteToBinaryFile: Invalid input param(s)");
		return TWX_BAD_REQUEST;
	}
	twInfoTable_GetNumber(params, "offset", 0, &offset);
	twInfoTable_GetBlob(params, "data", 0, &data, &count);
	if (!data) {
		TW_LOG(TW_ERROR, "twWriteToBinaryFile: Invalid input param(s)");
		cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
		return TWX_BAD_REQUEST;
	}
	/* Outputs */
	*content = NULL;
	/* Execute the service */
	{
		twFile * f = NULL;
		int res = 0;
		f = twFileManager_GetOpenFile(entityName, path, NULL, NULL);
		if (!f || !f->handle) {
			TW_LOG(TW_ERROR, "twWriteToBinaryFile: File is not open for writing");
			cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
			return TWX_PRECONDITION_FAILED;
		}
		/* If the file isn't already open, we need to open it */
		/*
		if (!f->handle) {
			f->handle = TW_FOPEN(f->realPath, "w+b");
			f->openForRead = FALSE;
		}
		*/
		res = TW_FSEEK(f->handle, (uint64_t)offset, SEEK_SET);
		if (res) {
			TW_LOG(TW_ERROR, "twWriteToBinaryFile: Error seeking in file %s to %lli.  Error: %d", path, offset, twDirectory_GetLastError());
			cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
			return TWX_INTERNAL_SERVER_ERROR;
		}
		res = TW_FWRITE(data, 1, count, f->handle);
		if (res > 0) {
			cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
			return TWX_SUCCESS;
		} else {
			TW_LOG(TW_ERROR, "twWriteToBinaryFile: Error writing to file %s", path);
			cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
			return TWX_INTERNAL_SERVER_ERROR;
		}
	}
}

enum msgCodeEnum twListDirectories(const char * entityName, twInfoTable * params, twInfoTable ** content) {
	return twListEntities(entityName, params, content, FALSE);
}

enum msgCodeEnum twMakeDirectory(const char * entityName, twInfoTable * params, twInfoTable ** content) {
	int res;
	/* Inputs */ 
	char * path = NULL;
	char * realPath = NULL;
	if (getPaths(params, entityName, &path, &realPath, NULL, NULL, NULL, NULL)) {
		TW_LOG(TW_ERROR, "twMakeDirectory: Invalid input param(s)");
		return TWX_BAD_REQUEST;
	}
	/* Outputs */
	*content = NULL;
	/* Perform the function */
	if (twDirectory_FileExists(realPath)) {
		TW_LOG(TW_WARN, "twMakeDirectory: Directory %s exists.  Nothing to do", path);
		cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
		return TWX_SUCCESS;
	}
	TW_LOG(TW_DEBUG, "twMakeDirectory: Creating directory %s", path);
	res = twDirectory_CreateDirectory(realPath);
	if (res) {
		TW_LOG(TW_ERROR, "twMakeDirectory: Error creating file %s.  Error: %d", path, res);
		cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
		return TWX_INTERNAL_SERVER_ERROR;
	}
	cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
	return TWX_SUCCESS;
}

enum msgCodeEnum twStartFileTransfer(const char * entityName, twInfoTable * params, twInfoTable ** content) {
	char * tmp = NULL;
	twFile * f = NULL;
	char mode[8];
	/* Inputs */ 
	char * path = NULL;
	char * realPath = NULL;
	if (getPaths(params, entityName, &path, &realPath, NULL, NULL, NULL, NULL)) {
		TW_LOG(TW_ERROR, "twStartFileTransfer: Invalid input param(s)");
		return TWX_BAD_REQUEST;
	}
	twInfoTable_GetString(params, "mode", 0, &tmp);
	TW_LOG(TW_AUDIT, "FILE TRANSFER STARTED.  File: %s, Mode: %s", realPath, tmp ? tmp : "unknown");
	strcpy(mode,"rb");
	if (tmp && !strcmp(tmp,"write")) {
		strcpy(mode,"a+b");
		TW_FREE(tmp);
		tmp = NULL;
		/* 
		If we are opening for a write, we want to prepend the
		staging directory since that is prepended by the server 
		on the CreateBinaryFile and WriteBinaryFile services
		*/
		tmp = (char *)TW_CALLOC(strlen(TW_VIRTUAL_STAGING_DIR) + 1 + strlen(path) + 1, 1);
		if (tmp) {
			strcpy(tmp, TW_VIRTUAL_STAGING_DIR);
			if (path[0] != '/' && path[0] != '\\') tmp[strlen(tmp)] = TW_FILE_DELIM;
			strcat(tmp, path);
			TW_FREE(path);
			path = tmp;
		}
	} else TW_FREE(tmp);
	/* Outputs */
	*content = NULL;
	/* Perform the function */
	f = twFileManager_OpenFile(entityName, path, NULL, mode);
	if (!f) {
		cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
		return TWX_INTERNAL_SERVER_ERROR;
	}
	if (twInfoTable_GetString(params, "tid", 0, &f->tid)) {
		twFileManager_CloseFile(f);
		cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
		return TWX_INTERNAL_SERVER_ERROR;		
	}
	if (!strcmp(tmp, "w+b")) f->openForRead = FALSE;
	cleanUpPaths(&path, &realPath, NULL, NULL, NULL, NULL);
	return TWX_SUCCESS;
}

enum msgCodeEnum twFinishFileTransfer(const char * entityName, twInfoTable * params, twInfoTable ** content) {
	/* Inputs */ 
	twPrimitive * job = NULL;
	twFile * f = NULL;
	twFileTransferInfo * ft = NULL;
	if (twInfoTable_GetPrimitive(params, "job", 0, &job)) {
		TW_LOG(TW_ERROR,"twFinishFileTransfer: Missing 'job' parameter");
		return TWX_BAD_REQUEST;
	}
	if (job->type != TW_INFOTABLE) {
		TW_LOG(TW_ERROR,"twFinishFileTransfer: 'job' parameter is not an infotable");
		return TWX_BAD_REQUEST;
	}
	ft = twFileTransferInfo_Create(job->val.infotable);
	if (!ft) {
		TW_LOG(TW_ERROR,"twFinishFileTransfer: Error getting file transfer info");
		return TWX_BAD_REQUEST;
	}
	/* Outputs */
	*content = NULL;
	/* Perform the function */
	f = twFileManager_GetOpenFile(entityName, NULL, NULL, ft->transferId);
	if (!f){
		twFileTransferInfo_Delete(ft);
		return TWX_INTERNAL_SERVER_ERROR;
	}
	TW_LOG(TW_AUDIT, "FILE TRANSFER %s.  File: %s%c%s", ft->state, f->realPath, TW_FILE_DELIM, f->name);
	twFileManager_CloseFile(f);
	/* Make the call back if defined */
	twFileManager_MakeFileCallback(!f->openForRead, ft);
	return TWX_SUCCESS;
}

char * fileXferServices[] = {
	"BrowseDirectory", "DeleteFile", "GetFileInfo", "ListFiles", "MoveFile",
	"GetFileListing", "GetTransferInfo", "GetFileChecksum", "CreateBinaryFile",
	"ReadFromBinaryFile", "WriteToBinaryFile", "ListDirectories",
	"MakeDirectory", "StartFileTransfer", "CancelFileTransfer", "CompleteFileTransfer",
	"SENTINEL"
};

enum msgCodeEnum fileTransferCallback(const char * entityName, const char * serviceName, twInfoTable * params, twInfoTable ** content) {
	/* GetTransferInfo has no aparams, all others do */
	if (!entityName || !serviceName || (strcmp(serviceName,"GetTransferInfo") && !params)) {
		TW_LOG(TW_ERROR, "fileTransferCallback: missing entityName, serviceName, or input params");
		return TWX_BAD_REQUEST;
	}
	if (!content) {
		TW_LOG(TW_ERROR, "fileTransferCallback: missing content param");
		return TWX_INTERNAL_SERVER_ERROR;
	}
	if (!strcmp(serviceName, "BrowseDirectory")) {
		return twBrowseDirectory(entityName, params, content);
	} else 	if (!strcmp(serviceName, "DeleteFile")) {
		return twDeleteFile(entityName, params, content);
	} else 	if (!strcmp(serviceName, "GetFileInfo")) {
		return twGetFileInfo(entityName, params, content);
	} else 	if (!strcmp(serviceName, "ListFiles")) {
		return twListFiles(entityName, params, content);
	} else 	if (!strcmp(serviceName, "MoveFile")) {
		return twMoveFile(entityName, params, content);
	} else 	if (!strcmp(serviceName, "GetTransferInfo")) {
		return twGetTransferInfo(entityName, params, content);
	} else 	if (!strcmp(serviceName, "GetFileChecksum")) {
		return twGetFileChecksum(entityName, params, content);
	} else 	if (!strcmp(serviceName, "CreateBinaryFile")) {
		return twCreateBinaryFile(entityName, params, content);
	} else 	if (!strcmp(serviceName, "ReadFromBinaryFile")) {
		return twReadFromBinaryFile(entityName, params, content);
	} else 	if (!strcmp(serviceName, "WriteToBinaryFile")) {
		return twWriteToBinaryFile(entityName, params, content);
	} else 	if (!strcmp(serviceName, "ListDirectories")) {
		return twListDirectories(entityName, params, content);
	} else 	if (!strcmp(serviceName, "RenameFile")) {
		return twMoveFile(entityName, params, content);
	} else 	if (!strcmp(serviceName, "StartFileTransfer")) {
		return twStartFileTransfer(entityName, params, content);
	} else 	if (!strcmp(serviceName, "CancelFileTransfer")) {
		return twFinishFileTransfer(entityName, params, content);
	} else  if (!strcmp(serviceName, "CompleteFileTransfer")) {
		return twFinishFileTransfer(entityName, params, content);
	} else {
		TW_LOG(TW_ERROR, "fileTransferCallback: Bad serviceName: %s", serviceName);
		return TWX_BAD_REQUEST;
	}
}

