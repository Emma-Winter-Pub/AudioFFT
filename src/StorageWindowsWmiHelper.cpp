#include "StorageWindowsWmiHelper.h"

#include <windows.h>
#include <wbemidl.h>
#include <QString>
#include <QStringList>

#pragma comment(lib, "wbemuuid.lib")

struct ComInitWrapper {
    bool needUninitialize = false;
    ComInitWrapper() {
        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        needUninitialize = (hr == S_OK || hr == S_FALSE);
    }
    ~ComInitWrapper() {
        if (needUninitialize) {
            CoUninitialize();
        }
    }
};

class ScopedBSTR {
    BSTR bstr;
public:
    explicit ScopedBSTR(const wchar_t* str) : bstr(SysAllocString(str)) {}
    ~ScopedBSTR() { if (bstr) SysFreeString(bstr); }
    operator BSTR() const { return bstr; }
};

class ScopedVariant {
    VARIANT var;
public:
    ScopedVariant() { VariantInit(&var); }
    ~ScopedVariant() { VariantClear(&var); }
    VARIANT* get() { return &var; }
    VARIANT& ref() { return var; }
};

class WmiServiceConnector {
public:
    IWbemLocator* locator = nullptr;
    IWbemServices* services = nullptr;
    bool connect(const wchar_t* namespacePath) {
        HRESULT hr = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
                                      IID_IWbemLocator, (LPVOID*)&locator);
        if (FAILED(hr)) return false;
        ScopedBSTR path(namespacePath);
        hr = locator->ConnectServer(path, NULL, NULL, 0, NULL, 0, 0, &services);
        if (FAILED(hr)) return false;
        hr = CoSetProxyBlanket(services, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                               RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
        return SUCCEEDED(hr);
    }
    ~WmiServiceConnector() {
        if (services) services->Release();
        if (locator) locator->Release();
    }
};

static void detectBitLocker(VolumeTopology& topology) {
    if (topology.volumePath.length() < 6) return;
    QString driveLetter = topology.volumePath.mid(4, 2);
    WmiServiceConnector wmi;
    if (!wmi.connect(L"ROOT\\CIMV2\\Security\\MicrosoftVolumeEncryption")) return;
    QString wqlStr = QString("SELECT ProtectionStatus FROM Win32_EncryptableVolume WHERE DriveLetter = '%1'").arg(driveLetter);
    ScopedBSTR query(wqlStr.toStdWString().c_str());
    ScopedBSTR wql(L"WQL");
    IEnumWbemClassObject* enumerator = nullptr;
    if (FAILED(wmi.services->ExecQuery(wql, query, WBEM_FLAG_FORWARD_ONLY, NULL, &enumerator)) || !enumerator) return;
    IWbemClassObject* pclsObj = nullptr;
    ULONG uReturn = 0;
    while (enumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn) == S_OK && uReturn > 0) {
        ScopedVariant vtStatus;
        if (SUCCEEDED(pclsObj->Get(L"ProtectionStatus", 0, vtStatus.get(), 0, 0)) && vtStatus.ref().vt == VT_I4) {
            if (vtStatus.ref().lVal == 1) topology.isEncrypted = true;
        }
        pclsObj->Release();
    }
    enumerator->Release();
}

static void detectDiskModelsMerged(VolumeTopology& topology) {
    if (topology.devices.isEmpty()) return;
    WmiServiceConnector wmi;
    if (!wmi.connect(L"ROOT\\CIMV2")) return;
    QStringList conditions;
    for (auto it = topology.devices.constBegin(); it != topology.devices.constEnd(); ++it) {
        if (it.key() >= 0) conditions << QString("Index = %1").arg(it.key());
    }
    if (conditions.isEmpty()) return;
    QString wqlStr = "SELECT Index, Model, PNPDeviceID FROM Win32_DiskDrive WHERE " + conditions.join(" OR ");
    ScopedBSTR query(wqlStr.toStdWString().c_str());
    ScopedBSTR wql(L"WQL");
    IEnumWbemClassObject* enumerator = nullptr;
    if (FAILED(wmi.services->ExecQuery(wql, query, WBEM_FLAG_FORWARD_ONLY, NULL, &enumerator)) || !enumerator) return;
    IWbemClassObject* pclsObj = nullptr;
    ULONG uReturn = 0;
    while (enumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn) == S_OK && uReturn > 0) {
        ScopedVariant vtIndex;
        if (SUCCEEDED(pclsObj->Get(L"Index", 0, vtIndex.get(), 0, 0)) && vtIndex.ref().vt == VT_I4) {
            int diskIdx = vtIndex.ref().lVal;
            if (topology.devices.contains(diskIdx)) {
                PhysicalDevice& device = topology.devices[diskIdx];
                ScopedVariant vtModel;
                if (SUCCEEDED(pclsObj->Get(L"Model", 0, vtModel.get(), 0, 0)) && vtModel.ref().vt == VT_BSTR) {
                    QString modelName = QString::fromWCharArray(vtModel.ref().bstrVal).toUpper();
                    if (modelName.contains("STORAGE SPACE")) {
                        topology.isStorageSpaces = true;
                        device.busType = StorageBusType::StorageSpaces;
                    }
                    if (modelName.contains("VIRTUAL") || modelName.contains("VHD") || modelName.contains("RAMDISK")) {
                        device.isVirtual = true;
                        topology.isVirtualDisk = true;
                    }
                    if (device.model.isEmpty()) {
                        device.model = QString::fromWCharArray(vtModel.ref().bstrVal);
                    }
                }
                ScopedVariant vtPnp;
                if (SUCCEEDED(pclsObj->Get(L"PNPDeviceID", 0, vtPnp.get(), 0, 0)) && vtPnp.ref().vt == VT_BSTR) {
                    device.pnpId = QString::fromWCharArray(vtPnp.ref().bstrVal);
                }
            }
        }
        pclsObj->Release();
    }
    enumerator->Release();
}

void StorageWindowsWmiHelper::enhanceTopology(VolumeTopology& topology) {
    ComInitWrapper comInit;
    detectDiskModelsMerged(topology);
    detectBitLocker(topology);
}