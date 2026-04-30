#pragma once

#include <optional>
#include <string>
#include <iomanip>
#include <iostream>

namespace SimpleCommKitTestUtils {

    std::optional<std::size_t> getUserInputInt(const std::string& line, std::size_t max){
        std::size_t ret;

        while (!std::cin.eof()) {
            std::cout << line << " (0-" << max << "): ";
            std::cin >> ret;

            if (!std::cin) {
                return {};
            }

            if (ret <= max) {
                return ret;
            }
        }
        return {};
    }

}