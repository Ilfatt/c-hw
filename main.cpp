#include <iostream>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>

using boost::asio::ip::tcp;

std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

std::string get_username() {
    return exec("whoami");
}

std::string get_hostname() {
    return exec("hostname");
}

std::string get_server_datetime() {
    time_t now = time(0);
    return ctime(&now);
}

std::string get_ls(const std::string& folder) {
    std::string command = "ls -al ";
    command += folder;
    return exec(command.c_str());
}

bool create_file(const std::string& filename) {
    std::ofstream file(filename);
    return file.good();
}

bool create_directory(const std::string& dirname) {
    return boost::filesystem::create_directory(dirname);
}

bool remove_file(const std::string& filename) {
    return boost::filesystem::remove(filename);
}

bool remove_directory(const std::string& dirname) {
    return boost::filesystem::remove_all(dirname);
}

std::string read_file(const std::string& filename, std::size_t size) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return "File not found";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str().substr(0, size);
}

void handle_command(const std::string& command, tcp::socket& socket) {
    std::string output;
    if (command == "username") {
        output = get_username();
    } else if (command == "hostname") {
        output = get_hostname();
    } else if (command == "serverdatetime") {
        output = get_server_datetime();
    } else if (command.find("ls ") == 0) {
        std::string folder = command.substr(3);
        output = get_ls(folder);
    } else if (command.find("mkfile ") == 0) {
        std::string filename = command.substr(7);
        if (create_file(filename)) {
            output = "File created successfully";
        } else {
            output = "Error creating file";
        }
    } else if (command.find("mkdir ") == 0) {
        std::string dirname = command.substr(6);
        if (create_directory(dirname)) {
            output = "Directory created successfully";
        } else {
            output = "Error creating directory";
        }
    } else if (command.find("rmfile ") == 0) {
        std::string filename = command.substr(7);
        if (remove_file(filename)) {
            output = "File removed successfully";
        } else {
            output = "Error removing file";
        }
    } else if (command.find("rmdir ") == 0) {
        std::string dirname = command.substr(6);
        if (remove_directory(dirname)) {
            output = "Directory removed successfully";
        } else {
            output = "Error removing directory";
        }
    } else if (command.find("readfile ") == 0) {
        std::string filename = command.substr(9);
        std::size_t size = std::string::npos;
        if (filename.find_last_of(' ') != std::string::npos) {
            size = std::stoul(filename.substr(filename.find_last_of(' ') + 1));
            filename = filename.substr(0, filename.find_last_of(' '));
        }
        output = read_file(filename, size);
    } else if (command == "quit") {
        output = "Goodbye!";
    } else {
        output = "Unknown command";
    }
    boost::asio::write(socket, boost::asio::buffer(output));
}

void start_session(tcp::socket socket) {
    try {
        while (true) {
            boost::asio::streambuf buf;
            boost::asio::read_until(socket, buf, "n");
            std::string command(boost::asio::buffer_cast<const char*>(buf.data()), buf.size());
            command.erase(command.find_last_not_of(" nrt") + 1);
            handle_command(command, socket);
            if (command == "quit") {
                break;
            }
        }
    } catch (std::exception& e) {
        std::cerr << "Exception in thread: " << e.what() << "n";
    }
}

int main() {
    boost::asio::io_service io_service;
    tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), 1234));
    while (true) {
        tcp::socket socket(io_service);
        acceptor.accept(socket);
        std::thread(start_session, std::move(socket)).detach();
    }
    return 0;
}
