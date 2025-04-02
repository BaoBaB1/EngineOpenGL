#pragma once

#include <Windows.h>
#include <crtdbg.h>

#define _CRTDBG_MAP_ALLOC
#ifdef _DEBUG
#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif

class MemoryProfiler {
public:
  enum DumpResult {
    eNoMemoryLeaks = 1, 
    eExistMemoryLeaks,
    eNoMemorySnapshot
  };
  static void make_memory_snapshot();
  static DumpResult dump();
private:
  static void init();
  static _CrtMemState m_first;
  static _CrtMemState m_second;
};