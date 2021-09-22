/* #define DEBUG */
#define WASM_ENABLED

#define RARDLL // needed for include
#include "wasm.hpp"
#include <rar.hpp>
#include <stdio.h>

#ifdef WASM_ENABLED
#include <emscripten.h>
#include <emscripten/bind.h>
#endif

#ifdef WASM_ENABLED
using namespace emscripten;
#endif

int handle_volume_change(long str_ptr, long fn_call_mode) {
#ifdef DEBUG
  printf("Volume changed (%s)\n", (char *)str_ptr);
#endif
  return 1;
}

int handle_process_data(long unpacked_data_ptr, long size) {
#ifdef DEBUG
  printf("Got unpacked data (size=%ld)\n", size);
#endif
  return 1;
}

int callback(UINT msg, LPARAM data, LPARAM p1, LPARAM p2) {
  switch (msg) {
  case UCM_CHANGEVOLUME:
    return handle_volume_change(p1, p2);
  case UCM_PROCESSDATA:
    return handle_process_data(p1, p2);
  }

  return -1;
}

int unarchive(uintptr_t archive_path_ptr, uintptr_t dest_path_ptr) {
  char *archive_path = (char *)archive_path_ptr;
  char *dest_path = (char *)dest_path_ptr;
  struct RAROpenArchiveData data = {.ArcName = archive_path,
                                    .OpenMode = RAR_OM_EXTRACT,
                                    .CmtBuf = NULL,
                                    .CmtBufSize = 0,
                                    // Set by library
                                    .OpenResult = 0,
                                    .CmtSize = 0,
                                    .CmtState = 0};
  HANDLE handle = RAROpenArchive(&data);
  if (handle == NULL) {
    return 1;
  }

  char *next_path = (char *)calloc(1024, sizeof(char));
  RARSetCallback(handle, callback, (long)next_path);

  RARHeaderData header = {};

  int result = 0;
  int process_result = 0;
  int count = 0;
  while (result == 0) {
    result = RARReadHeader(handle, &header);
    process_result = RARProcessFile(handle, RAR_EXTRACT, dest_path, NULL);
  }

  return 0;
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("Please supply an input archive and output destination\n");
    return 1;
  }

  unarchive((uintptr_t)argv[1], (uintptr_t)argv[2]);
  return 0;
}

#ifdef WASM_ENABLED
EMSCRIPTEN_BINDINGS(unrar) {
  function("__unrar_unarchive", &unarchive, allow_raw_pointers());
}
#endif
