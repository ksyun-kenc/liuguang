/*
 * Copyright 2020-present Ksyun
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <filesystem>
#include <iostream>

#include <boost/program_options.hpp>

#include "process.h"

#include "../EasyHook/Public/easyhook.h"
#include "umu/shellapi.h"

namespace fs = std::filesystem;
namespace po = boost::program_options;

template <typename T, typename U>
T ArrayToString(std::vector<T>& _array, U open, U _seperator, U close) {
  T str(open);
  T seperator(_seperator);
  for (const auto& e : _array) {
    str.append(e);
    str.append(seperator);
  }
  str.erase(str.size() - seperator.size());
  str.append(close);
  return str;
}

void ChechExec(fs::path& exec, fs::path& cd, std::wstring& arguments) {
  if (exec.empty()) {
    throw std::invalid_argument("No exec specifed!");
  }
  if (!fs::is_regular_file(exec)) {
    throw std::invalid_argument("Invalid exec: " + exec.string());
  }

  if (cd.empty()) {
    cd = exec.parent_path();
  } else {
    if (!fs::is_directory(cd)) {
      throw std::invalid_argument("Invalid cd: " + cd.string());
    }
  }

  std::wcout << L"exec = " << exec << L'\n';
  std::wcout << L"arguments = " << arguments << L'\n';
  std::wcout << L"cd = " << cd << L'\n';
}

int wmain(int argc, wchar_t* argv[]) {
  std::ios::sync_with_stdio(false);
  std::cin.tie(nullptr);
  std::locale::global(std::locale(""));

  int result = 0;
  try {
    bool dynamic = false;
    fs::path exec;
    std::wstring arguments;
    std::vector<std::wstring> image_names;
    fs::path cd;
    fs::path lx86;
    fs::path lx64;
    uint32_t wait;

    po::options_description desc("Allowed options");
    desc.add_options()("help,h", "Produce help message")(
        "dynamic,d", po::value<bool>(&dynamic), "Use dynamic injecting")(
        "exec,e", po::value<fs::path>(&exec), "Path of the executable")(
        "arg,a", po::wvalue<std::wstring>(&arguments),
        "Arguments of the executable")("cd,c", po::value<fs::path>(&cd),
                                       "Current directory for the executable")(
        "imagename,i", po::wvalue<std::vector<std::wstring>>(&image_names),
        "Image name of the process being injected.")(
        "wait,w", po::value<uint32_t>(&wait)->default_value(1000),
        "Wait before injecting. unit: ms")("lx86", po::value<fs::path>(&lx86),
                                           "Path of x86 library to inject into process")(
        "lx64", po::value<fs::path>(&lx64), "Path of x64 library to inject into process");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
      std::cout << desc;
      return 0;
    }

    // sanity check
    if (0 == vm.count("lx86") && 0 == vm.count("lx64")) {
      throw std::invalid_argument("Neither lx86 nor lx64 specifed!");
    }
    if (0 < vm.count("lx86") && !fs::is_regular_file(lx86)) {
      throw std::invalid_argument("Invalid lx86: " + lx86.string());
    }
    if (0 < vm.count("lx64") && !fs::is_regular_file(lx64)) {
      throw std::invalid_argument("Invalid lx64: " + lx64.string());
    }

    std::wcout << L"dynamic = " << std::boolalpha << dynamic << L'\n';
    std::wcout << L"lx86 = " << lx86 << L'\n';
    std::wcout << L"lx64 = " << lx64 << L'\n';

    if (dynamic) {
      // sanity check
      if (image_names.empty()) {
        throw std::invalid_argument("No imagename specifed.");
      }

      std::vector<DWORD> pids;

      if (!ProcessNamesToProcessIds(image_names, pids)) {
        // sanity check
        ChechExec(exec, cd, arguments);

        // run it then retry
        HANDLE process;
        DWORD error_code = umu::shellapi::Execute(
            exec.wstring().data(), arguments.data(), cd.wstring().data(),
            nullptr, 0, SW_NORMAL, &process);
        if (ERROR_SUCCESS != error_code) {
          std::wcerr << L"Execute(" << exec << L") = " << error_code << '\n';
          return error_code;
        }
        std::wcout << L"Wait process(" << exec << L") for " << wait
                   << "ms...\n";
        DWORD r = WaitForSingleObject(process, wait);
        ::CloseHandle(process);
        if (WAIT_OBJECT_0 == r) {
          std::wcout << L"Process(" << exec << L") finished...\n";
        }

        if (!ProcessNamesToProcessIds(image_names, pids)) {
          std::wcerr << L"Can't find target process: "
                     << ArrayToString(image_names, L"[", L", ", L"]") << L'\n';
          return ERROR_NOT_FOUND;
        }
      }

      for (auto& pid : pids) {
        NTSTATUS status = RhInjectLibrary(pid, 0, EASYHOOK_INJECT_DEFAULT,
                                          lx86.wstring().data(),
                                          lx64.wstring().data(), nullptr, 0);
        if (NT_SUCCESS(status)) {
          std::wcout << L"RhInjectLibrary(PID = " << std::dec << pid << ").\n";
        } else {
          std::wcout << L"RhInjectLibrary(PID = " << std::dec << pid
                     << L") failed with 0x" << std::hex << std::setw(8)
                     << status << ": " << RtlGetLastErrorString() << L'\n';
        }
      }
    } else {
      // sanity check
      ChechExec(exec, cd, arguments);

      ULONG pid;
      result = RhCreateAndInjectEx(cd.wstring().data(), exec.wstring().data(),
                                   arguments.data(), 0, EASYHOOK_INJECT_DEFAULT,
                                   lx86.wstring().data(), lx64.wstring().data(),
                                   nullptr, 0, &pid);
      if (NT_SUCCESS(result)) {
        std::wcout << L"CreateAndInject(" << exec << L", `" << arguments
                   << L"'), PID: " << pid << '\n';
      } else {
        std::wcout << L"RhCreateAndInject() failed with 0x" << std::hex
                   << std::setw(8) << result << ": " << RtlGetLastErrorString()
                   << L'\n';
      }
    }
  } catch (const fs::filesystem_error& e) {
    std::cerr << "\nFilessytem error: " << e.what() << '\n';
    result = -1;
  } catch (const std::exception& e) {
    std::cerr << "\nException: " << e.what() << '\n';
    result = -2;
  } catch (...) {
    std::cerr << "\nError: Caught an unknown exception!" << '\n';
    result = -3;
  }
  return result;
}
