#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <list>
#include <vector>
#include <map>
#include <tuple>
#include <thread>
#include <functional>
using namespace std;

#include <Windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <comdef.h>

#include <wrl\client.h>
using Microsoft::WRL::ComPtr;
using Microsoft::WRL::WeakRef;

#define THROW_ON_FAIL(x) {HRESULT hr = x; if (FAILED(hr)) { OutputDebugString(_com_error(hr).ErrorMessage()); throw std::exception("D3D11 Error. See debug log.");}}