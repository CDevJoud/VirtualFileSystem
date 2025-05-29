#pragma once
#include <Windows.h>
#include <vector>
#include <string>
#include "json.hpp"

////////////////////////////////////////////////////
/// Configurations: 
///		Enable Commandline Output
/// 
#define ENABLE_COMMANDLINE_OUTPUT true
///		Enable IDE Commandline Output
/// 
#define ENABLE_IDECMD_OUTPUT false
////////////////////////////////////////////////////

////////////////////////////////////////////////////
/// 
///			   Begin Class Defintions
/// 
////////////////////////////////////////////////////

class BaseVirtualFileSystemBuilder;
class VirtualFileExplorerInterface;

class InputStream
{
public:
	virtual ~InputStream() = default;
	virtual ULONG_PTR Read(LPVOID data, ULONG_PTR nSize) = 0;
	virtual ULONG_PTR Seek(ULONG_PTR nSize) = 0;
	virtual ULONG_PTR Tell() = 0;
	virtual ULONG_PTR GetSize() = 0;
};

class MemoryInputStream : public InputStream
{
public:
	MemoryInputStream() = default;
	MemoryInputStream(LPCVOID data, ULONG_PTR size);

	bool Open(LPCVOID data, ULONG_PTR size);

	ULONG_PTR Read(LPVOID data, ULONG_PTR nSize);
	ULONG_PTR Seek(ULONG_PTR nSize);
	ULONG_PTR Tell();
	ULONG_PTR GetSize();
private:
	BYTE* data;
	ULONG_PTR size, offset;
};

class FileInputStream : public InputStream
{
public:
	FileInputStream() = default;
	~FileInputStream() override;
	FileInputStream(const FileInputStream&) = delete;
	FileInputStream& operator=(const FileInputStream&) = delete;
	FileInputStream(FileInputStream&&) noexcept;
	FileInputStream& operator=(FileInputStream&&) noexcept;

	bool Open(const std::string& fileName);

	ULONG_PTR Read(LPVOID data, ULONG_PTR nSize) override;
	ULONG_PTR Seek(ULONG_PTR nSize)override;
	ULONG_PTR Tell()override;
	ULONG_PTR GetSize() override;
private:
	HANDLE hFile;
	ULONG_PTR nOldPos;
};

class VirtualFileSystem
{
public:
	VirtualFileSystem();
	VirtualFileSystem(const std::string& _Indata);
	VirtualFileSystem(BaseVirtualFileSystemBuilder* builder, const std::string& _Indata);
	void DLog(LPCSTR msg);
	void WDLog(LPCTSTR msg);

	struct File
	{
		std::string tag;
		ULONG_PTR start, end;
		std::vector<BYTE> data;
	};
	typedef std::vector<File*> Files;
	

	union PackVersion
	{
		PackVersion() : quad(0ui32) {}
		PackVersion(INT nVersion) : quad(nVersion) {};
		struct {
			BYTE major, minor, patch, unused;
		};
		UINT quad;
	};

	PackVersion MakeVersion(BYTE major, BYTE minor, BYTE patch);
	UINT32 MakeVersionToInt(BYTE major, BYTE minor, BYTE patch);
	std::vector<BYTE>& GetBinaries();

	void Get(const std::string& tag, std::vector<BYTE>& buffer);
	std::pair<LPVOID, ULONG_PTR> GetPtrFromSrc(const std::string& tag);

	bool RegisterExplorerInterface(VirtualFileExplorerInterface* Interface);
private:
	bool _Build(const std::string& data);

	void InitHConsole();

	HANDLE hConsole;
	std::vector<BYTE> bin, contentHeader;
	Files files;
	BaseVirtualFileSystemBuilder* builder;
	VirtualFileExplorerInterface* Interface;
	FileInputStream fInputStream;
};

class VirtualFileExplorerInterface
{
public:
	struct Directory
	{
		std::vector<std::string> fileNames;
		std::vector<Directory*> dirs;
	};
	VirtualFileExplorerInterface() = default;
	VirtualFileExplorerInterface(VirtualFileExplorerInterface&&) = delete;
	virtual ~VirtualFileExplorerInterface() = default;
	
	virtual void listFiles();
protected:
	virtual std::vector<std::string> ReadContentHeader() = 0;
	std::vector<BYTE>* contentHeader;
	VirtualFileSystem::Files* binFiles;
	
	friend class VirtualFileSystem;
};

class VirtualFileExplorer : public VirtualFileExplorerInterface
{
public:
	VirtualFileExplorer() = default;

private:
	std::vector<std::string> ReadContentHeader() override;
};

class BaseVirtualFileSystemBuilder
{
public:
	BaseVirtualFileSystemBuilder() = default;
	BaseVirtualFileSystemBuilder(BaseVirtualFileSystemBuilder&&) = delete;
	virtual ~BaseVirtualFileSystemBuilder() = default;
	virtual std::string ReadFromFile(const std::string& fileName) = 0;
	virtual bool Convert(const std::string& data) = 0;
	virtual bool AddFile(VirtualFileSystem::File* f);
private:
	bool SetVirtualFileHandler(VirtualFileSystem::Files* file);
	bool SetContentHeaderSizeCounter(PULONG_PTR nCounter);
	PULONG_PTR nCounter = nullptr;
	VirtualFileSystem::Files* aFiles;
	friend class VirtualFileSystem;
};

class JNativeVirtualFileSystemBuilder : public BaseVirtualFileSystemBuilder
{
public:
	static JNativeVirtualFileSystemBuilder* Create();
	std::string ReadFromFile(const std::string& fileName) override;
	bool Convert(const std::string& data) override;
};

////////////////////////////////////////////////////
/// 
///				End Class Defintions
/// 
////////////////////////////////////////////////////

MemoryInputStream::MemoryInputStream(const void* data, ULONG_PTR size) :
	data((BYTE*)(data)),
	size(size)
{
	
}

////////////////////////////////////////////////////

inline bool MemoryInputStream::Open(LPCVOID data, ULONG_PTR size)
{
	if (data == nullptr)
		return false;
	this->data = (BYTE*)data;
	this->size = size;
}

////////////////////////////////////////////////////

inline ULONG_PTR MemoryInputStream::Read(LPVOID data, ULONG_PTR nSize)
{
	const ULONG_PTR count = min(size, size - offset);
	if (count > 0)
	{
		std::memcpy(data, this->data + offset, static_cast<ULONG_PTR>(count));
		offset += count;
	}
	return count;
}

////////////////////////////////////////////////////

inline ULONG_PTR MemoryInputStream::Seek(ULONG_PTR position)
{
	offset = position < size ? position : size;
	return offset;
}

////////////////////////////////////////////////////

inline ULONG_PTR MemoryInputStream::Tell()
{
	return offset;
}

inline ULONG_PTR MemoryInputStream::GetSize()
{
	return size;
}

////////////////////////////////////////////////////

FileInputStream::~FileInputStream()
{
	if (this->hFile != INVALID_HANDLE_VALUE)
		CloseHandle(this->hFile);
}

////////////////////////////////////////////////////

inline FileInputStream::FileInputStream(FileInputStream&&) noexcept :
	hFile(nullptr),
	nOldPos(0ui64)
{
	
}

////////////////////////////////////////////////////

inline FileInputStream& FileInputStream::operator=(FileInputStream&&) noexcept
{
	
}

////////////////////////////////////////////////////

inline bool FileInputStream::Open(const std::string& fileName)
{
	this->hFile = CreateFileA(fileName.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (this->hFile == INVALID_HANDLE_VALUE)
		return false;

	return true;
}

////////////////////////////////////////////////////

inline ULONG_PTR FileInputStream::Read(LPVOID data, ULONG_PTR nSize)
{
	DWORD r = 0ui32;
	if (!::ReadFile(this->hFile, data, nSize, &r, nullptr))
		return -1;
	return (ULONG_PTR)r;
}

////////////////////////////////////////////////////

inline ULONG_PTR FileInputStream::Seek(ULONG_PTR nSize)
{
	DWORD nPos = this->nOldPos;
	this->nOldPos = ::SetFilePointer(this->hFile, nSize, nullptr, FILE_BEGIN);
	if (this->nOldPos == INVALID_SET_FILE_POINTER)
		return (ULONG_PTR)-1;
	else
		return nPos;
}

////////////////////////////////////////////////////

inline ULONG_PTR FileInputStream::Tell()
{
	return ::SetFilePointer(this->hFile, 0i32, nullptr, FILE_CURRENT);
}

////////////////////////////////////////////////////

inline ULONG_PTR FileInputStream::GetSize()
{
	LARGE_INTEGER nFileSize = { 0i32 };
	if (!GetFileSizeEx(this->hFile, &nFileSize))
		return 0ui64;
	else
		return (ULONG_PTR)nFileSize.QuadPart;
}

////////////////////////////////////////////////////

inline VirtualFileSystem::VirtualFileSystem() :
	builder(JNativeVirtualFileSystemBuilder::Create()),
	hConsole(nullptr)
{

}

////////////////////////////////////////////////////

inline VirtualFileSystem::VirtualFileSystem(const std::string& _Indata) :
	builder(JNativeVirtualFileSystemBuilder::Create()),
	hConsole(nullptr)
{
	size_t size = _Indata.size() - _Indata.find_last_of('.');
	if (size <= 5)
	{
		std::string read = this->builder->ReadFromFile(_Indata);
		if (!read.empty())
			this->_Build(read);
		else
			this->_Build(_Indata);
	}
	else
		this->_Build(_Indata);
}

////////////////////////////////////////////////////

inline VirtualFileSystem::VirtualFileSystem(BaseVirtualFileSystemBuilder* builder, const std::string& _Indata) :
	builder(builder),
	hConsole(nullptr)
{
	size_t size = _Indata.size() - _Indata.find_last_of('.');
	if (size <= 5)
	{
		std::string read = this->builder->ReadFromFile(_Indata);
		if (!read.empty())
			this->_Build(read);
		else
			this->_Build(_Indata);
	}
	else
		this->_Build(_Indata);
}

////////////////////////////////////////////////////

inline void VirtualFileSystem::DLog(LPCSTR lpszMsg)
{
	if (ENABLE_COMMANDLINE_OUTPUT)
	{
		if (this->hConsole == nullptr) this->InitHConsole();
		DWORD w = 0ui32;
		::WriteConsoleA(hConsole, lpszMsg, strlen(lpszMsg), &w, nullptr);
		::WriteConsoleA(hConsole, "\n", 1i32, &w, nullptr);
	}
	else if (ENABLE_IDECMD_OUTPUT)
	{
		::OutputDebugStringA(lpszMsg);
		::OutputDebugStringA("\n");
	}
}

////////////////////////////////////////////////////

inline void VirtualFileSystem::WDLog(LPCTSTR lpszMsg)
{
	if (ENABLE_COMMANDLINE_OUTPUT)
	{
		if (this->hConsole == nullptr) this->InitHConsole();
		DWORD w = 0ui32;
		::WriteConsoleW(hConsole, lpszMsg, lstrlenW(lpszMsg), &w, nullptr);
		::WriteConsoleW(hConsole, L"\n", 1ui32, &w, nullptr);
	}
	else if (ENABLE_IDECMD_OUTPUT)
	{
		::OutputDebugStringW(lpszMsg);
		::OutputDebugStringW(L"\n");
	}
}

////////////////////////////////////////////////////

inline VirtualFileSystem::PackVersion VirtualFileSystem::MakeVersion(BYTE major, BYTE minor, BYTE patch)
{
	PackVersion pVersion;
	pVersion.major = major;
	pVersion.minor = minor;
	pVersion.patch = patch;
	pVersion.unused = 0ui8;
	return pVersion;
}

////////////////////////////////////////////////////

inline UINT32 VirtualFileSystem::MakeVersionToInt(BYTE major, BYTE minor, BYTE patch)
{
	PackVersion pVersion;
	pVersion.major = major;
	pVersion.minor = minor;
	pVersion.patch = patch;
	pVersion.unused = 0ui8;
	return pVersion.quad;
}

////////////////////////////////////////////////////

inline std::vector<BYTE>& VirtualFileSystem::GetBinaries()
{
	return this->bin;
}

////////////////////////////////////////////////////

inline void VirtualFileSystem::Get(const std::string& tag, std::vector<BYTE>& buffer)
{
	for (auto& i : this->files)
	{
		if (strcmp(i->tag.c_str(), tag.c_str()) == 0)
		{
			if (buffer.size() <= i->end - i->start)
				buffer.resize(i->end - i->start + 1);

			ULONG_PTR pos = this->fInputStream.Tell();
			if (pos == INVALID_SET_FILE_POINTER)
			{
				MemoryInputStream mem;
				mem.Open(this->bin.data(), this->bin.size());
				mem.Seek(i->start);
				mem.Read(buffer.data(), buffer.size());
			}
			else
			{
				this->fInputStream.Seek(i->start);
				this->fInputStream.Read(buffer.data(), i->end - i->start + 1);
				this->fInputStream.Seek(pos);
			}
			break;
		}
	}
}

////////////////////////////////////////////////////

inline std::pair<LPVOID, ULONG_PTR> VirtualFileSystem::GetPtrFromSrc(const std::string& tag)
{
	std::pair<LPVOID, ULONG_PTR> p;
	for (auto& i : this->files)
	{
		if (strcmp(i->tag.c_str(), tag.c_str()) == 0)
		{
			ULONG_PTR pos = this->fInputStream.Tell();
			if (pos == INVALID_SET_FILE_POINTER)
			{
				p.first = this->bin.data() + i->start;
				p.second = i->end - i->start;
				return p;
			}
		}
	}
}

////////////////////////////////////////////////////

inline bool VirtualFileSystem::RegisterExplorerInterface(VirtualFileExplorerInterface* Interface)
{
	if (Interface == nullptr)
		return false;
	this->Interface = Interface;
	this->Interface->contentHeader = &this->contentHeader;
	this->Interface->binFiles = &this->files;
	std::vector<std::string> dirs = this->Interface->ReadContentHeader();
	
	return true;
}

////////////////////////////////////////////////////

inline bool VirtualFileSystem::_Build(const std::string& data)
{
	CHAR header[16]{};
	PackVersion version = MakeVersion(1, 0, 0);
	memcpy(header, "BinUgrPack", strlen("BinUgrPack"));
	memcpy(header + 10, &version, 4);

	ULONG_PTR nContentHeaderSize = 16;
	ULONG_PTR nFileCursor = 32;

	if (!this->builder->SetContentHeaderSizeCounter(&nContentHeaderSize))
		return false;
	if (!this->builder->SetVirtualFileHandler(&this->files))
		return false;
	if (!this->builder->Convert(data))
		return false;

	nFileCursor += nContentHeaderSize - 16;

	contentHeader.insert(contentHeader.end(), std::begin(header), std::end(header));
	nContentHeaderSize--;
	const char* bytesContentHeaderSize = (const char*)(&nContentHeaderSize);
	contentHeader.insert(contentHeader.end(), bytesContentHeaderSize, bytesContentHeaderSize + 8);
	char null[8]{};
	contentHeader.insert(contentHeader.end(), std::begin(null), std::end(null));
	for (auto& i : files)
	{
		// Add zero terminating string
		i->tag.push_back(0);
		contentHeader.insert(contentHeader.end(), i->tag.begin(), i->tag.end());
		i->start = nFileCursor;
		i->end = i->start + i->data.size() - 1;
		const char* bytesStart = (const char*)&i->start;
		contentHeader.insert(contentHeader.end(), bytesStart, bytesStart + 8);

		const char* bytesEnd = (const char*)&i->end;
		contentHeader.insert(contentHeader.end(), bytesEnd, bytesEnd + 8);
		nFileCursor = i->end;
		nFileCursor++;
	}
	this->bin.insert(this->bin.end(), this->contentHeader.begin(), this->contentHeader.end());
	for (auto& i : files)
	{
		bin.insert(bin.end(), i->data.begin(), i->data.end());
	}
	for (auto& i : files)
	{
		i->data.clear();
		i->data.shrink_to_fit();
	}
	return true;
}

inline void VirtualFileSystem::InitHConsole()
{
	this->hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	if (this->hConsole == nullptr)
		if (!::AllocConsole())
		{
			::MessageBoxA(nullptr, "Could Not Allocate The Console Window!", "Error", MB_OK | MB_ICONERROR);
			::ExitProcess(EXIT_FAILURE);
		}
		else
			this->hConsole = ::GetStdHandle(STD_OUTPUT_HANDLE);
}

////////////////////////////////////////////////////

inline bool BaseVirtualFileSystemBuilder::AddFile(VirtualFileSystem::File* f)
{
	if (f == nullptr)
		return false;
	*this->nCounter += f->tag.size() + 16 + 1;
	this->aFiles->push_back(f);
	return true;
}

////////////////////////////////////////////////////

inline bool BaseVirtualFileSystemBuilder::SetVirtualFileHandler(VirtualFileSystem::Files* files)
{
	if (files == nullptr)
		return false;
	this->aFiles = files;
	return true;
}

////////////////////////////////////////////////////

inline bool BaseVirtualFileSystemBuilder::SetContentHeaderSizeCounter(PULONG_PTR nCounter)
{
	if (nCounter == nullptr)
		return false;

	this->nCounter = nCounter;
	return true;
}

////////////////////////////////////////////////////

inline void VirtualFileExplorerInterface::listFiles()
{
	
}

////////////////////////////////////////////////////

inline std::vector<std::string> VirtualFileExplorer::ReadContentHeader()
{
	std::vector<std::string> dirs;
	///Get all the directory names from the binary and store them 
	
	std::string tmp;
	for (int i = 32; i < this->contentHeader->size(); i++)
	{
		if ((*this->contentHeader)[i] == 0)
		{
			i += 16;
			dirs.push_back(tmp);
			tmp = "";
			continue;
		}
		tmp.push_back((*this->contentHeader)[i]);
		
	}
	return dirs;
}

////////////////////////////////////////////////////

JNativeVirtualFileSystemBuilder* JNativeVirtualFileSystemBuilder::Create()
{
	return new JNativeVirtualFileSystemBuilder;
}

////////////////////////////////////////////////////

inline std::string JNativeVirtualFileSystemBuilder::ReadFromFile(const std::string& fileName)
{
	std::string str;
	FileInputStream fIn;
	if (fIn.Open(fileName))
	{
		str.resize(fIn.GetSize(), 0);
		fIn.Read((PCHAR)str.data(), str.size());
		return str;
	}
	return str;
}

////////////////////////////////////////////////////

inline bool JNativeVirtualFileSystemBuilder::Convert(const std::string& _data)
{
	nlohmann::json data = nlohmann::json::parse(_data);
	for (auto& i : data.items())
	{
		VirtualFileSystem::File* file = new VirtualFileSystem::File;
		FileInputStream fIn;
		file->tag = i.key();

		fIn.Open(i.value());
		file->data.resize(fIn.GetSize(), 0);
		fIn.Read(file->data.data(), file->data.size());

		this->AddFile(file);
	}
	return true;
}

////////////////////////////////////////////////////
