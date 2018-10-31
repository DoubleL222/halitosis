#include "log.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include <chrono>

static std::ofstream log_file;
static std::vector<std::string> log_buffer;
static bool has_opened = false;
static bool has_atexit = false;

void dump_buffer_at_exit() {
    if (has_opened) {
        return;
    }

    auto now_in_nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    std::string filename = "bot-unknown-" + std::to_string(now_in_nanos) + ".log";
    std::ofstream file(filename, std::ios::trunc | std::ios::out);
    for (const std::string& message : log_buffer) {
        file << message << std::endl;
    }
}

// NEW VERSION - Always checks if the log file already exists, and if it does, it tries to create a unique name for it.
void hlt::log::open(int bot_id) {
    if (has_opened) {
        hlt::log::log("Error: log: tried to create new log file, but we have already created one.");
        exit(1);
    }

    has_opened = true;

	int fileno = 0;
	bool success = false;
	std::string filename;

	do {
		filename = "bot-" + std::to_string(bot_id) + "-(" + std::to_string(fileno) + ").log";
		fileno++; //increase by one to get a new file name
	} while (std::experimental::filesystem::exists(filename) && fileno < INT_MAX);
	
	if (!std::experimental::filesystem::exists(filename)) {
		log_file.open(filename, std::ios::trunc | std::ios::out);

		for (const std::string& message : log_buffer) {
			log_file << message << std::endl;
		}
		log_buffer.clear();
	}
	else {
		hlt::log::log("Log file could not be created!");
		exit(1);
	}
}

// OLD VERSION
//void hlt::log::open(int bot_id) {
//	if (has_opened) {
//		hlt::log::log("Error: log: tried to open(" + std::to_string(bot_id) + ") but we have already opened before.");
//		exit(1);
//	}
//
//	has_opened = true;
//	std::string filename = "bot-" + std::to_string(bot_id) + ".log";
//	log_file.open(filename, std::ios::trunc | std::ios::out);
//
//	for (const std::string& message : log_buffer) {
//		log_file << message << std::endl;
//	}
//	log_buffer.clear();
//}

void hlt::log::log(const std::string& message) {
    if (has_opened) {
        log_file << message << std::endl;
    } else {
        if (!has_atexit) {
            has_atexit = true;
            atexit(dump_buffer_at_exit);
        }
        log_buffer.push_back(message);
    }
}
