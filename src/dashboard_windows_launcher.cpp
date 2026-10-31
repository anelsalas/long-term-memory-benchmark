#include "long_term_memory_benchmark/dashboard_windows_launcher.hpp"
#include <windows.h>
#include <stdexcept>
#include <utility>

namespace ltm::dashboard {
namespace {
[[noreturn]] void fail(const char* step) {
    throw std::runtime_error(std::string(step) + " failed with Windows error " + std::to_string(GetLastError()));
}
class Handle {
  public:
    explicit Handle(HANDLE h = nullptr) : h_(h == INVALID_HANDLE_VALUE ? nullptr : h) {}
    Handle(Handle&& other) noexcept : h_(std::exchange(other.h_, nullptr)) {}
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    ~Handle() { if (h_) CloseHandle(h_); }
    HANDLE get() const { return h_; }
    explicit operator bool() const { return h_ != nullptr; }
  private:
    HANDLE h_;
};
class WindowsProcess : public Process {
  public:
    WindowsProcess(Handle job, Handle process) : job_(std::move(job)), process_(std::move(process)) {}
    std::optional<int> exit_code() override {
        if (WaitForSingleObject(process_.get(), 0) != WAIT_OBJECT_0) return std::nullopt;
        DWORD code = 0;
        if (!GetExitCodeProcess(process_.get(), &code)) fail("GetExitCodeProcess");
        return static_cast<int>(code);
    }
    void stop() override {
        if (!TerminateJobObject(job_.get(), 1) && !exit_code()) fail("TerminateJobObject");
    }
  private:
    Handle job_;  // closing the job kills anything still running in it
    Handle process_;
};
class AttributeList {
  public:
    explicit AttributeList(HANDLE* inherited) {
        SIZE_T size = 0;
        InitializeProcThreadAttributeList(nullptr, 1, 0, &size);
        buffer_.resize(size);
        if (!InitializeProcThreadAttributeList(get(), 1, 0, &size)) fail("InitializeProcThreadAttributeList");
        if (!UpdateProcThreadAttribute(get(), 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, inherited, sizeof(HANDLE), nullptr, nullptr)) {
            DeleteProcThreadAttributeList(get());
            fail("UpdateProcThreadAttribute");
        }
    }
    ~AttributeList() { DeleteProcThreadAttributeList(get()); }
    LPPROC_THREAD_ATTRIBUTE_LIST get() { return reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(buffer_.data()); }
  private:
    std::vector<char> buffer_;
};
Handle kill_on_close_job() {
    Handle job(CreateJobObjectW(nullptr, nullptr));
    if (!job) fail("CreateJobObjectW");
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!SetInformationJobObject(job.get(), JobObjectExtendedLimitInformation, &limits, sizeof limits))
        fail("SetInformationJobObject");
    return job;
}
std::wstring wide(const std::string& text) {
    if (text.empty()) return {};
    const int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (size <= 0) throw std::invalid_argument("Command argument is not valid UTF-8");
    std::wstring result(static_cast<std::size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), result.data(), size);
    return result;
}
void append_quoted(std::wstring& line, const std::wstring& arg) {
    if (!arg.empty() && arg.find_first_of(L" \t\n\v\"") == std::wstring::npos) { line += arg; return; }
    line += L'"';
    std::size_t backslashes = 0;
    for (const wchar_t c : arg) {
        if (c == L'\\') { ++backslashes; continue; }
        line.append(c == L'"' ? backslashes * 2 + 1 : backslashes, L'\\');
        line += c;
        backslashes = 0;
    }
    line.append(backslashes * 2, L'\\');
    line += L'"';
}
}

std::wstring windows_command_line(const std::vector<std::string>& command) {
    std::wstring line;
    for (const auto& arg : command) {
        if (!line.empty()) line += L' ';
        append_quoted(line, wide(arg));
    }
    return line;
}

std::unique_ptr<Process> WindowsLauncher::start(const std::vector<std::string>& command,
        const std::filesystem::path& output) {
    if (command.empty()) throw std::invalid_argument("Benchmark command is empty");
    auto line = windows_command_line(command);
    auto job = kill_on_close_job();
    SECURITY_ATTRIBUTES inheritable{sizeof inheritable, nullptr, TRUE};
    Handle out(CreateFileW(output.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_DELETE, &inheritable,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr));
    if (!out) fail("CreateFileW for run output");
    HANDLE inherited[] = {out.get()};
    AttributeList attributes(inherited);
    STARTUPINFOEXW info{};
    info.StartupInfo.cb = sizeof info;
    info.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    info.StartupInfo.hStdOutput = out.get();
    info.StartupInfo.hStdError = out.get();
    info.lpAttributeList = attributes.get();
    PROCESS_INFORMATION created{};
    if (!CreateProcessW(nullptr, line.data(), nullptr, nullptr, TRUE,
            CREATE_SUSPENDED | CREATE_NO_WINDOW | EXTENDED_STARTUPINFO_PRESENT, nullptr, nullptr,
            &info.StartupInfo, &created))
        fail("CreateProcessW");
    Handle process(created.hProcess), thread(created.hThread);
    if (!AssignProcessToJobObject(job.get(), process.get())) {
        TerminateProcess(process.get(), 1);
        fail("AssignProcessToJobObject");
    }
    if (ResumeThread(thread.get()) == static_cast<DWORD>(-1)) fail("ResumeThread");
    return std::make_unique<WindowsProcess>(std::move(job), std::move(process));
}
}
