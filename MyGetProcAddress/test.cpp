// MyGetProcAddress.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#include <iostream>
#include "MyGetProcAddress.h"
using namespace std;

int _tmain(int argc, _TCHAR* argv[])
{
	FARPROC tem, tem2;
	tem = GetProcAddress(GetModuleHandle(L"kernel32.dll"), "OpenProcess");
	tem2 = MyGetProcAddress(GetModuleHandle(L"api-ms-win-core-processthreads-l1-1-2.dll"), "OpenProcess");
	cout << tem << endl;
	cout << tem2 << endl;
	system("pause");
	return 0;
}

