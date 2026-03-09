#pragma once
#include "BackgroundWorker.hpp"
#include <vector>
#include <windows.h>
#include <winioctl.h>


class USNJournalMonitor : public BackgroundWorker {
public:
  HANDLE hVolume;
  USNJournalMonitor(const std::tvstring &volumePath) {
    hVolume = CreateFile(volumePath.c_str(), GENERIC_READ,
                         FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                         OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
  }
  ~USNJournalMonitor() {
    if (hVolume != INVALID_HANDLE_VALUE)
      CloseHandle(hVolume);
  }
  virtual void run() {
    if (hVolume == INVALID_HANDLE_VALUE)
      return;
    USN_JOURNAL_DATA_V0 ujd;
    DWORD cb;
    if (!DeviceIoControl(hVolume, FSCTL_QUERY_USN_JOURNAL, NULL, 0, &ujd,
                         sizeof(ujd), &cb, NULL)) {
      return;
    }
    READ_USN_JOURNAL_DATA_V0 rujd = {0};
    rujd.StartUsn = ujd.NextUsn;
    rujd.ReasonMask = 0xFFFFFFFF;
    rujd.ReturnOnlyOnClose = FALSE;
    rujd.Timeout = 0;
    rujd.BytesToWaitFor = 0;
    rujd.UsnJournalID = ujd.UsnJournalID;

    char buffer[4096];
    while (!this->get_cancelled()) {
      if (DeviceIoControl(hVolume, FSCTL_READ_USN_JOURNAL, &rujd, sizeof(rujd),
                          buffer, sizeof(buffer), &cb, NULL)) {
        rujd.StartUsn = *(USN *)buffer;
        // Next steps: parse USN_RECORD and patch in-memory NtfsIndex
      } else {
        Sleep(100);
      }
    }
  }
};
