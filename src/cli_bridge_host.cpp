#include "long_term_memory_benchmark/cli_bridge_host.hpp"
#include "long_term_memory_benchmark/dashboard_windows_launcher.hpp"
#include <windows.h>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>
#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace ltm::cli_bridge {
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
    ~Handle() { reset(); }
    HANDLE get() const { return h_; }
    void reset() { if (h_) CloseHandle(std::exchange(h_, nullptr)); }
    explicit operator bool() const { return h_ != nullptr; }
  private:
    HANDLE h_;
};
class TemporaryInstructions {
  public:
    explicit TemporaryInstructions(const std::string& content) {
        wchar_t folder[MAX_PATH + 1]{};
        const auto length = GetTempPathW(MAX_PATH + 1, folder);
        if (!length || length > MAX_PATH) fail("GetTempPathW");
        wchar_t file[MAX_PATH + 1]{};
        if (!GetTempFileNameW(folder, L"ltm", 0, file)) fail("GetTempFileNameW");
        path_ = file;
        try {
            Handle out(CreateFileW(path_.c_str(), GENERIC_WRITE, 0, nullptr, TRUNCATE_EXISTING,
                FILE_ATTRIBUTE_TEMPORARY, nullptr));
            if (!out) fail("Open instructions file");
            std::size_t offset = 0;
            while (offset < content.size()) {
                DWORD written = 0;
                const auto amount = static_cast<DWORD>(std::min<std::size_t>(content.size() - offset, 65536));
                if (!WriteFile(out.get(), content.data() + offset, amount, &written, nullptr) || written == 0)
                    fail("Write instructions file");
                offset += written;
            }
        } catch (...) {
            DeleteFileW(path_.c_str());
            throw;
        }
    }
    ~TemporaryInstructions() { DeleteFileW(path_.c_str()); }
    TemporaryInstructions(const TemporaryInstructions&) = delete;
    TemporaryInstructions& operator=(const TemporaryInstructions&) = delete;
    std::string toml_path() const {
        const auto path = std::filesystem::path(path_).generic_u8string();
        std::string result = "\"";
        for (const unsigned char ch : path) {
            if (ch < 0x20 || ch == 0x7f) throw std::invalid_argument("Invalid instructions path");
            if (ch == '\\' || ch == '"') result += '\\';
            result += static_cast<char>(ch);
        }
        return result + "\"";
    }
  private:
    std::wstring path_;
};
struct Pipe {
    Handle read, write;
};
// The child's end is inheritable; ours is not.
Pipe pipe(bool child_reads) {
    SECURITY_ATTRIBUTES inheritable{sizeof inheritable, nullptr, TRUE};
    HANDLE r = nullptr, w = nullptr;
    if (!CreatePipe(&r, &w, &inheritable, 0)) fail("CreatePipe");
    Pipe p{Handle(r), Handle(w)};
    if (!SetHandleInformation(child_reads ? p.write.get() : p.read.get(), HANDLE_FLAG_INHERIT, 0))
        fail("SetHandleInformation");
    return p;
}
std::wstring wide(const std::string& text) {
    if (text.empty()) return {};
    const int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (size <= 0) throw std::invalid_argument("Path is not valid UTF-8");
    std::wstring result(static_cast<std::size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), result.data(), size);
    return result;
}
class AttributeList {
  public:
    AttributeList(HANDLE* inherited, std::size_t count) {
        SIZE_T size = 0;
        InitializeProcThreadAttributeList(nullptr, 1, 0, &size);
        buffer_.resize(size);
        if (!InitializeProcThreadAttributeList(get(), 1, 0, &size)) fail("InitializeProcThreadAttributeList");
        if (!UpdateProcThreadAttribute(get(), 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, inherited, count * sizeof(HANDLE),
                nullptr, nullptr)) {
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
class FileLock {
  public:
    explicit FileLock(HANDLE file) : file_(file) {
        if (!LockFileEx(file_, LOCKFILE_EXCLUSIVE_LOCK, 0, MAXDWORD, MAXDWORD, &overlap_)) fail("Lock budget ledger");
    }
    ~FileLock() { UnlockFileEx(file_, 0, MAXDWORD, MAXDWORD, &overlap_); }
    FileLock(const FileLock&) = delete;
    FileLock& operator=(const FileLock&) = delete;
  private:
    HANDLE file_;
    OVERLAPPED overlap_{};
};
}

WindowsPersistentBudget::WindowsPersistentBudget(std::string ledger_path, std::uint64_t limit)
    : ledger_path_(std::move(ledger_path)), limit_(limit) {
    if (ledger_path_.empty() || !limit_ || limit_ > 1000000) throw std::invalid_argument("Invalid durable budget");
}

bool WindowsPersistentBudget::try_acquire() {
    std::lock_guard<std::mutex> local_lock(mutex_);
    const auto path = wide(ledger_path_);
    Handle file(CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr));
    if (!file) fail("Open budget ledger");
    FileLock lock(file.get());
    const auto header = std::string("ltm-budget-v1 ") + std::to_string(limit_) + "\n";
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(file.get(), &size) || size.QuadPart < 0 ||
        static_cast<std::uint64_t>(size.QuadPart) > header.size() + limit_ * 2)
        throw std::runtime_error("Invalid budget ledger");
    std::string records(static_cast<std::size_t>(size.QuadPart), '\0');
    DWORD read = 0;
    if (!records.empty() && (!ReadFile(file.get(), records.data(), static_cast<DWORD>(records.size()), &read, nullptr) ||
        read != records.size())) throw std::runtime_error("Cannot read budget ledger");
    if (!records.empty() && (records.rfind(header, 0) != 0 || (records.size() - header.size()) % 2))
        throw std::runtime_error("Invalid budget ledger or changed cap");
    const auto record_start = records.empty() ? 0 : header.size();
    for (std::size_t i = record_start; i < records.size(); i += 2)
        if (records[i] != '1' || records[i + 1] != '\n') throw std::runtime_error("Invalid budget ledger");
    if ((records.size() - record_start) / 2 >= limit_) return false;
    const auto entry = (records.empty() ? header : std::string{}) + "1\n";
    DWORD written = 0;
    if (!WriteFile(file.get(), entry.data(), static_cast<DWORD>(entry.size()), &written, nullptr) || written != entry.size() ||
        !FlushFileBuffers(file.get())) throw std::runtime_error("Cannot persist budget attempt");
    return true;
}

RunResult WindowsCommandRunner::run(const Invocation& invocation) {
    if (invocation.command.empty()) throw std::invalid_argument("Command is empty");
    auto command = invocation.command;
    std::unique_ptr<TemporaryInstructions> instructions;
    if (invocation.system_prompt) {
        for (auto& arg : command) {
            if (arg.rfind("model_instructions_file=", 0) == 0) {
                instructions = std::make_unique<TemporaryInstructions>(*invocation.system_prompt);
                arg = "model_instructions_file=" + instructions->toml_path();
                break;
            }
        }
    }
    auto line = ltm::dashboard::windows_command_line(command);
    const auto folder = wide(invocation.working_directory);
    auto job = kill_on_close_job();
    auto input = pipe(true);
    auto output = pipe(false);
    SECURITY_ATTRIBUTES inheritable{sizeof inheritable, nullptr, TRUE};
    Handle nul(CreateFileW(L"NUL", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &inheritable, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, nullptr));
    if (!nul) fail("CreateFileW for NUL");
    HANDLE inherited[] = {input.read.get(), output.write.get(), nul.get()};
    AttributeList attributes(inherited, 3);
    STARTUPINFOEXW info{};
    info.StartupInfo.cb = sizeof info;
    info.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    info.StartupInfo.hStdInput = input.read.get();
    info.StartupInfo.hStdOutput = output.write.get();
    info.StartupInfo.hStdError = nul.get();
    info.lpAttributeList = attributes.get();
    PROCESS_INFORMATION created{};
    if (!CreateProcessW(nullptr, line.data(), nullptr, nullptr, TRUE,
            CREATE_SUSPENDED | CREATE_NO_WINDOW | EXTENDED_STARTUPINFO_PRESENT, nullptr,
            folder.empty() ? nullptr : folder.c_str(), &info.StartupInfo, &created))
        fail("CreateProcessW");
    Handle process(created.hProcess), thread(created.hThread);
    if (!AssignProcessToJobObject(job.get(), process.get())) {
        TerminateProcess(process.get(), 1);
        fail("AssignProcessToJobObject");
    }
    input.read.reset();
    output.write.reset();
    if (ResumeThread(thread.get()) == static_cast<DWORD>(-1)) fail("ResumeThread");

    // Write and read on their own threads so neither pipe can fill up and stall the child.
    std::thread writer([&] {
        std::size_t sent = 0;
        while (sent < invocation.input.size()) {
            DWORD written = 0;
            const auto chunk = static_cast<DWORD>(std::min<std::size_t>(invocation.input.size() - sent, 1 << 16));
            if (!WriteFile(input.write.get(), invocation.input.data() + sent, chunk, &written, nullptr)) break;
            sent += written;
        }
        input.write.reset();
    });
    RunResult result;
    std::thread reader([&] {
        char buffer[1 << 16];
        DWORD got = 0;
        while (ReadFile(output.read.get(), buffer, sizeof buffer, &got, nullptr) && got) result.output.append(buffer, got);
    });
    result.timed_out = WaitForSingleObject(process.get(), invocation.timeout_ms) == WAIT_TIMEOUT;
    // Ends the child on timeout, and any helper it left behind either way.
    TerminateJobObject(job.get(), 1);
    writer.join();
    reader.join();
    DWORD code = 0;
    if (!result.timed_out && !GetExitCodeProcess(process.get(), &code)) fail("GetExitCodeProcess");
    result.exit_code = result.timed_out ? -1 : static_cast<int>(code);
    return result;
}

namespace {
namespace beast = boost::beast;
namespace http = beast::http;
using tcp = boost::asio::ip::tcp;
constexpr std::size_t body_limit = 16 * 1024 * 1024;
}

struct Server::State {
    Bridge& bridge;
    boost::asio::io_context io;
    tcp::acceptor acceptor;
    std::atomic<bool> stopping{false};
    std::mutex mutex;
    std::condition_variable idle;
    std::size_t active = 0;
    State(Bridge& b, unsigned short port) : bridge(b), acceptor(io, {boost::asio::ip::make_address("127.0.0.1"), port}) {}

    Response route(const http::request<http::string_body>& request) const {
        const auto port = std::to_string(acceptor.local_endpoint().port());
        const std::string host(request[http::field::host]);
        if (request.count(http::field::origin) || (host != "127.0.0.1:" + port && host != "localhost:" + port))
            return {403, R"({"error":{"code":"forbidden"}})"};
        if (request.method() == http::verb::get && request.target() == "/health") return {200, R"({"status":"ok"})"};
        if (request.method() == http::verb::get && request.target() == "/metrics") {
            const auto metrics = bridge.metrics();
            return {200, nlohmann::json{{"instance_id", metrics.instance_id}, {"attempts", metrics.attempts}, {"completed", metrics.completed},
                {"input_tokens", metrics.input_tokens}, {"output_tokens", metrics.output_tokens},
                {"missing_usage", metrics.missing_usage}}.dump()};
        }
        if (request.method() == http::verb::post && request.target() == "/v1/chat/completions")
            return bridge.handle(request.body());
        return {404, R"({"error":{"code":"not_found"}})"};
    }
    void serve(tcp::socket socket) {
        beast::error_code ec;
        beast::flat_buffer buffer;
        http::request_parser<http::string_body> parser;
        parser.body_limit(body_limit);
        http::read(socket, buffer, parser, ec);
        Response response = ec == http::error::body_limit ? Response{413, R"({"error":{"code":"too_large"}})"}
                          : ec ? Response{} : route(parser.get());
        if (!response.status) return;
        http::response<http::string_body> reply{static_cast<http::status>(response.status), 11};
        reply.set(http::field::content_type, "application/json");
        reply.keep_alive(false);
        reply.body() = std::move(response.body);
        reply.prepare_payload();
        http::write(socket, reply, ec);
        socket.shutdown(tcp::socket::shutdown_both, ec);
    }
};

Server::Server(Bridge& bridge, unsigned short port) : state_(std::make_unique<State>(bridge, port)) {}
Server::~Server() {
    stop();
    std::unique_lock<std::mutex> lock(state_->mutex);
    state_->idle.wait(lock, [this] { return state_->active == 0; });
}
unsigned short Server::port() const { return state_->acceptor.local_endpoint().port(); }
void Server::run() {
    while (!state_->stopping) {
        tcp::socket socket(state_->io);
        beast::error_code ec;
        state_->acceptor.accept(socket, ec);
        if (ec) continue;
        {
            std::lock_guard<std::mutex> lock(state_->mutex);
            ++state_->active;
        }
        std::thread([state = state_.get(), s = std::move(socket)]() mutable {
            try { state->serve(std::move(s)); } catch (...) {}
            std::lock_guard<std::mutex> lock(state->mutex);
            --state->active;
            state->idle.notify_all();
        }).detach();
    }
}
void Server::stop() {
    state_->stopping = true;
    beast::error_code ignored;
    state_->acceptor.close(ignored);
}
} // namespace ltm::cli_bridge
