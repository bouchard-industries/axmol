/****************************************************************************
Copyright (c) 2010-2013 cocos2d-x.org
Copyright (c) Microsoft Open Technologies, Inc.
Copyright (c) 2017-2018 Xiamen Yaji Software Co., Ltd.

https://axmolengine.github.io/

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/
#include "platform/winrt/WinRTUtils.h"
#include <wrl/client.h>
#include <wrl/wrappers/corewrappers.h>
#include <ppl.h>
#include <ppltasks.h>
#include <sstream>
#include "base/Macros.h"
#include "platform/PlatformMacros.h"
#include "platform/FileUtils.h"
#include "base/UserDefault.h"
#include "ntcvt/ntcvt.hpp"

using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;

NS_AX_BEGIN

using namespace Windows::Graphics::Display;
using namespace concurrency;
using namespace Platform;
using namespace Windows::Storage;
using namespace Windows::Storage::Pickers;
using namespace Windows::Storage::Streams;
using namespace Windows::Networking::Connectivity;

bool isWindowsPhone()
{
#if _MSC_VER >= 1900
    if (Windows::Foundation::Metadata::ApiInformation::IsTypePresent("Windows.Phone.UI.Input.HardwareButtons"))
        return true;
    else
        return false;
#elif (WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP)
    return true;
#else
    return false;
#endif
}

std::string PlatformStringToString(Platform::String^ s) {
    return ntcvt::from_chars(std::wstring_view(s->Data(), s->Length()));
}

Platform::String^ PlatformStringFromString(std::string_view s)
{
    std::wstring ws = ntcvt::from_chars(s);
    return ref new Platform::String(ws.data(), static_cast<unsigned int>(ws.length()));
}

#if 0
// Method to convert a length in device-independent pixels (DIPs) to a length in physical pixels.
float ConvertDipsToPixels(float dips)
{
   static const float dipsPerInch = 96.0f;
   return floor(dips * DisplayProperties::LogicalDpi / dipsPerInch + 0.5f); // Round to nearest integer.
}

float getScaledDPIValue(float v) {
	auto dipFactor = DisplayProperties::LogicalDpi / 96.0f;
	return v * dipFactor;
}
#endif

void AX_DLL printIPAddresses()
{
    auto hostnames = NetworkInformation::GetHostNames();
    int length = hostnames->Size;

    for(int i = 0; i < length; i++)
    {
        auto hn = hostnames->GetAt(i);
        if (hn->IPInformation != nullptr)
        {
            std::string s = PlatformStringToString(hn->DisplayName);
            log("IP Address: %s:", s.c_str());
        }
    }
}

std::string AX_DLL getDeviceIPAddresses()
{
    std::stringstream result;

    auto hostnames = NetworkInformation::GetHostNames();
    int length = hostnames->Size;

    for(int i = 0; i < length; i++)
    {
        auto hn = hostnames->GetAt(i);
        if (hn->IPInformation != nullptr)
        {
            result << PlatformStringToString(hn->DisplayName) << std::endl;
        }
    }

    return result.str();
}

Platform::Object^ findXamlElement(Platform::Object^ parent, Platform::String^ name)
{
    if (parent == nullptr || name == nullptr || name->Length() == 0)
    {
        return nullptr;
    }

    FrameworkElement^ element = dynamic_cast<FrameworkElement^>(parent);
    if (element == nullptr)
    {
        return nullptr;
    }

    if (element->Name == name)
    {
        return element;
    }

    Panel^ panel = dynamic_cast<Panel^>(element);
    if (panel == nullptr)
    {
        return nullptr;
    }

    int count = panel->Children->Size;
    for (int i = 0; i < count; i++)
    {
        auto result = findXamlElement(panel->Children->GetAt(i), name);
        if (result != nullptr)
        {
            return result;
        }
    }

    return nullptr;
}


bool removeXamlElement(Platform::Object^ parent, Platform::Object^ element)
{
    Panel^ panel = dynamic_cast<Panel^>(parent);
    if (panel == nullptr)
    {
        return false;
    }

    UIElement^ uiElement = dynamic_cast<UIElement^>(element);
    if (uiElement == nullptr)
    {
        return false;
    }

    unsigned int index;
    if (!panel->Children->IndexOf(uiElement, &index))
    {
        return false;
    }

    panel->Children->RemoveAt(index);

    return true;
}

bool replaceXamlElement(Platform::Object^ parent, Platform::Object^ add, Platform::Object^ remove)
{
    Panel^ panel = dynamic_cast<Panel^>(parent);
    if (panel == nullptr)
    {
        return false;
    }

    UIElement^ addElement = dynamic_cast<UIElement^>(add);
    if (addElement == nullptr)
    {
        return false;
    }

    UIElement^ removeElement = dynamic_cast<UIElement^>(remove);
    if (removeElement == nullptr)
    {
        return false;
    }

    unsigned int index;
    if (!panel->Children->IndexOf(removeElement, &index))
    {
        return false;
    }

    panel->Children->RemoveAt(index);
    panel->Children->InsertAt(index, addElement);

    return true;
}

// Function that reads from a binary file asynchronously.
Concurrency::task<Platform::Array<byte>^> ReadDataAsync(Platform::String^ path)
{
	using namespace Windows::Storage;
	using namespace Concurrency;
		
	return create_task(StorageFile::GetFileFromPathAsync(path)).then([&](StorageFile^ f)
	{
		return FileIO::ReadBufferAsync(f);

	}).then([] (Streams::IBuffer^ fileBuffer) -> Platform::Array<byte>^ 
	{
		auto fileData = ref new Platform::Array<byte>(fileBuffer->Length);
		Streams::DataReader::FromBuffer(fileBuffer)->ReadBytes(fileData);
		return fileData;
	});
}

std::string computeHashForFile(const std::string& filePath)
{
    std::string ret = filePath;
    size_t pos = ret.find_last_of('/');

    if (pos != std::string::npos) {
        ret = ret.substr(pos);
    }

    pos = ret.find_last_of('.');

    if (pos != std::string::npos) {
        ret = ret.substr(0, pos);
    }

    CREATEFILE2_EXTENDED_PARAMETERS extParams = { 0 };
    extParams.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
    extParams.dwFileFlags = FILE_FLAG_RANDOM_ACCESS;
    extParams.dwSecurityQosFlags = SECURITY_ANONYMOUS;
    extParams.dwSize = sizeof(extParams);
    extParams.hTemplateFile = nullptr;
    extParams.lpSecurityAttributes = nullptr;

    Microsoft::WRL::Wrappers::FileHandle file(CreateFile2(std::wstring(filePath.begin(), filePath.end()).c_str(), GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, &extParams));

    if (file.Get() != INVALID_HANDLE_VALUE) {
        FILE_BASIC_INFO  fInfo = { 0 };
        if (GetFileInformationByHandleEx(file.Get(), FileBasicInfo, &fInfo, sizeof(FILE_BASIC_INFO))) {
            std::stringstream ss;
            ss << ret << "_";
            ss << fInfo.CreationTime.QuadPart;
            ss << fInfo.ChangeTime.QuadPart;
            ret = ss.str();
        }
    }

    return ret;
}

bool createMappedCacheFile(const std::string& srcFilePath, std::string& cacheFilePath, const std::string& ext /* = "" */)
{
    bool ret = false;
    auto folderPath = FileUtils::getInstance()->getWritablePath();
    cacheFilePath = folderPath + computeHashForFile(srcFilePath) + ext;
    auto prevFile = UserDefault::getInstance()->getStringForKey(srcFilePath.c_str());

    if (prevFile == cacheFilePath) {
        ret = FileUtils::getInstance()->isFileExist(cacheFilePath);
    }
    else {
        FileUtils::getInstance()->removeFile(prevFile);
    }

    UserDefault::getInstance()->setStringForKey(srcFilePath.c_str(), cacheFilePath);
    return ret;
}

void destroyMappedCacheFile(const std::string& key)
{
    auto value = UserDefault::getInstance()->getStringForKey(key.c_str());
    
    if (!value.empty()) {
        FileUtils::getInstance()->removeFile(value);
    }

    UserDefault::getInstance()->setStringForKey(key.c_str(), "");
}

NS_AX_END
