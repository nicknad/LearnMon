#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <print>
#include <random>
#include <ranges>
#include <string>
#include <vector>

enum class LessonType {
    Random = 0,
    Spelling = 1,
    MultipleChoice = 2,
    Hangman = 3
};

struct LessonEntry {
    uint8_t lesson_number{};
    std::string word;
    std::string description;
    std::string origin_word;

    LessonEntry(uint8_t num, std::string w, std::string d, std::string o)
        : lesson_number(num), word(std::move(w)), description(std::move(d)), origin_word(std::move(o)) {}
};

void recap_lesson(const std::vector<LessonEntry> &lessons);
bool serve_spelling_lesson(const LessonEntry &lesson);
bool serve_multiple_choice_lesson(const LessonEntry &lesson, std::default_random_engine &rng);
bool serve_hangman_lesson(const LessonEntry &lesson);
std::vector<LessonEntry> read_lesson_from_file(const std::filesystem::path &path, std::optional<uint8_t> lesson_no);

std::vector<std::string> split(std::string_view s, char delimiter);
std::vector<std::string> split_word_to_chars(const std::string& s);

inline void clear_screen();

int main(int argc, char *argv[]) {
    clear_screen();
    if (argc < 2) {
        std::println(std::cerr, "Usage: {} \"filepath\" [lesson number] [lesson type]", argv[0]);
        return 1;
    }

    const std::filesystem::path p = argv[1];
    if (!std::filesystem::exists(p)) {
        std::println(std::cerr, "File does not exist: {}", p.string());
        return 1;
    }

    std::random_device r;
    std::default_random_engine rng(r());
    std::optional<uint8_t> lesson_no{};
    LessonType lesson_type = LessonType::Random;

    try {
        if (argc >= 3) {
            const int temp = std::stoi(argv[2]);
            if (temp < 0 || temp > 255) {
                std::println(std::cerr, "Error: Lesson number must be between 0 and 255.");
                return 1;
            }
            lesson_no = static_cast<uint8_t>(temp);
            std::println("Preparing Lesson No {} ...", lesson_no.value());
        }

        if (argc >= 4) {
            const int temp = std::stoi(argv[3]);
            if (temp >= 0 && temp <= 3) {
                lesson_type = static_cast<LessonType>(temp);
            }
        }

        if (lesson_type == LessonType::Random) {
            std::uniform_int_distribution<> range(1, 3);
            switch (range(rng)) {
                case 1: lesson_type = LessonType::Spelling; break;
                case 2: lesson_type = LessonType::MultipleChoice; break;
                case 3: lesson_type = LessonType::Hangman; break;
                default: std::unreachable();
            }
        }
    } catch (const std::exception &e) {
        std::println(std::cerr, "Error: Invalid number format. {}", e.what());
        return 1;
    }

    if (argc > 4) {
        std::println(std::cerr, "Too many parameters.\nUsage: {} \"filepath\" [lesson number] [lesson type]", argv[0]);
        return 1;
    }

    auto lessons = read_lesson_from_file(p, lesson_no);

    if (lessons.empty()) {
        std::println(std::cerr, "No lessons found or file is empty.");
        return 1;
    }

    std::println("\nRecap\n");
    recap_lesson(lessons);
    std::println("\nPress Enter to start the lesson...\n");
    std::cin.get();
    clear_screen();

    std::println("\nStarting lesson...\n");
    std::ranges::shuffle(lessons, rng);

    switch (lesson_type) {
        case LessonType::Spelling: {
            for (const auto &lesson : lessons) {
                serve_spelling_lesson(lesson);
                std::println("\nPress Enter to continue...\n");
                std::cin.get();
                clear_screen();
            }
            break;
        }
        case LessonType::MultipleChoice:
            serve_multiple_choice_lesson(lessons.front(), rng);
            break;
        case LessonType::Hangman:
            serve_hangman_lesson(lessons.front());
            break;

            case LessonType::Random: {
                std::unreachable();
                break;
            }
    }

    std::cin.get();

    return 0;
}

inline void clear_screen() {
#if defined(_WIN32) || defined(_WIN64)
    system("cls");
#else
    system("clear");
#endif
}

std::vector<std::string> split(std::string_view s, const char delimiter) {
    std::vector<std::string> tokens;
    size_t start = 0, end = 0;
    while ((end = s.find(delimiter, start)) != std::string_view::npos) {
        tokens.emplace_back(s.substr(start, end - start));
        start = end + 1;
    }
    tokens.emplace_back(s.substr(start));
    return tokens;
}

std::vector<std::string> split_word_to_chars(const std::string& s) {
    std::vector<std::string> chars;
    for (size_t i = 0; i < s.length(); ) {
        if ((s[i] & 0x80) == 0x00) {
            chars.push_back(s.substr(i, 1));
            i += 1;
        } else if ((s[i] & 0xE0) == 0xC0) {
            chars.push_back(s.substr(i, 2));
            i += 2;
        } else if ((s[i] & 0xF0) == 0xE0) {
            chars.push_back(s.substr(i, 3));
            i += 3;
        } else if ((s[i] & 0xF8) == 0xF0) {
            chars.push_back(s.substr(i, 4));
            i += 4;
        } else {
            // Malformed UTF-8, fall back to single byte
            chars.push_back(s.substr(i, 1));
            i += 1;
        }
    }
    return chars;
}

std::vector<LessonEntry> read_lesson_from_file(const std::filesystem::path &path,
                                                const std::optional<uint8_t> lesson_no) {
    std::vector<LessonEntry> result;
    result.reserve(100);

    if (std::ifstream file{path}; file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            const auto words = split(line, ';');

            if (words.size() < 4) {
                std::println(std::cerr, "Warning: Skipping line with invalid format: {}", line);
                continue;
            }

            try {
                const int temp_lesson_no = std::stoi(words[0]);

                if (temp_lesson_no < 0 || temp_lesson_no > 255) {
                    std::println(std::cerr, "Invalid lesson number: {}!", words[0]);
                    continue;
                }
                const auto current_lesson_no = static_cast<uint8_t>(temp_lesson_no);

                if (lesson_no.has_value() && current_lesson_no != lesson_no.value()) {
                    continue;
                }

                result.emplace_back(current_lesson_no, words[1], words[2], words[3]);
            } catch (const std::exception &e) {
                std::println(std::cerr, "Error parsing lesson number on line: {}. {}", line, e.what());
            }
        }
    }
    return result;
}

void recap_lesson(const std::vector<LessonEntry> &lessons) {
    for (const auto &lesson : lessons) {
        std::println("{} ({})- {}", lesson.word, lesson.description, lesson.origin_word);
    }
}

bool serve_hangman_lesson(const LessonEntry &lesson) {
    std::string target;
    std::ranges::transform(lesson.word, std::back_inserter(target),
                           [](unsigned char c) -> unsigned char { return std::tolower(c); });

    std::vector<std::string> target_chars = split_word_to_chars(target);

    std::vector<std::string> guess_chars;
    guess_chars.resize(target_chars.size());
    for (size_t i = 0; i < target_chars.size(); ++i) {
        if (target_chars.at(i) == " ") {
            guess_chars.at(i) = " ";
        } else {
            guess_chars.at(i) = "_";
        }
    }

    auto get_guess_string = [&]() {
        std::string s;
        for (const auto& c : guess_chars) {
            s += c;
        }
        return s;
    };

    while (get_guess_string().contains('_')) {
        clear_screen();
        std::println("\nGuess the word!\n Current: {}", get_guess_string());

        std::string input;
        std::println("Enter a letter or a full word:");
        std::getline(std::cin, input);

        if (input.empty()) {
            continue;
        }

        std::ranges::transform(input, input.begin(),
                               [](unsigned char c) { return std::tolower(c); });

        if (input == "quit") {
            std::println("The word was: {} ", lesson.word);
            return false;
        }

        if (input == target) {
            std::println("\nYou found the word! \n{}\n{}\n{}", lesson.word, lesson.description, lesson.origin_word);
            return true;
        }

        if (input.empty() > 0) {
            bool found_char = false;
            for (size_t i = 0; i < target_chars.size(); ++i) {
                if (target_chars.at(i) == input) {
                    guess_chars.at(i) = input;
                    found_char = true;
                }
            }

            if (!found_char) {
                std::println("Wrong!");
            }
        }

        if (!get_guess_string().contains("_")) {
            std::println("\nYou found the word! \n{}\n{}\n{}", lesson.word, lesson.description, lesson.origin_word);
            return true;
        }
    }

    return true;
}

bool serve_multiple_choice_lesson(const LessonEntry &lesson, std::default_random_engine &rng) {
    std::vector<std::string> mongolian_letters = {"а", "б", "в", "г", "д", "е", "ё", "ж", "з", "и", "й", "к",
                                                    "л", "м", "н", "о", "ө", "п", "р", "с", "т", "у", "ү", "ф", "х", "ц", "ч",
                                                    "ш", "щ", "ъ", "ы", "ь", "э", "ю", "я" };

    std::vector<std::string> choices;
    choices.reserve(4);

    auto target_chars = split_word_to_chars(lesson.word);
    choices.push_back(lesson.word);
    for (int i = 0; i < 3; ++i) {
        std::string incorrect_word = lesson.word;
        std::uniform_int_distribution<> how_many_changes(2, std::min(static_cast<int>(target_chars.size()), 4));
        int amount_changes = how_many_changes(rng);
        auto shuffled_target_chars = target_chars;
        std::ranges::shuffle(shuffled_target_chars, rng);

        while (amount_changes > 0) {
            const auto& char_to_change_str = shuffled_target_chars.back();
            shuffled_target_chars.pop_back();
            size_t idx = incorrect_word.find(char_to_change_str);
            if (idx == std::string::npos || char_to_change_str == " ") {
                continue;
            }

            std::ranges::shuffle(mongolian_letters, rng);
            const auto& new_letter = mongolian_letters.front();

            if (new_letter == char_to_change_str) {
                continue;
            }

            incorrect_word.replace(idx, char_to_change_str.size(), new_letter);
            amount_changes--;
        }

        choices.push_back(incorrect_word);
    }

    std::ranges::shuffle(choices, rng);

    int correct_choice_idx = -1;
    for (size_t i = 0; i < choices.size(); ++i) {
        std::println("{}. {}", i + 1, choices[i]);
        if (choices[i] == lesson.word) {
            correct_choice_idx = static_cast<int>(i) + 1;
        }
    }

    while (true) {
        std::string input;
        std::println("\nHow do you spell {}?", lesson.origin_word);
        std::println("Enter your choice (1-4):");
        std::getline(std::cin, input);

        if (input.empty()) {
            continue;
        }

        if (input == "quit") {
            std::println("The word was: {} ", lesson.word);
            return false;
        }

        try {
            int choice = std::stoi(input);
            if (choice < 1 || choice > 4) {
                std::println("Invalid input! Please enter a number between 1 and 4.");
                continue;
            }

            if (choice == correct_choice_idx) {
                std::println("Correct! You found the word! \n{}\n{}\n{}", lesson.word, lesson.description, lesson.origin_word);

                return true;
            } else {
                std::println("Wrong! The correct choice was {}!", correct_choice_idx);
                std::println("The word was: {}\n{}\n{}", lesson.word, lesson.description, lesson.origin_word);

                return false;
            }
        } catch (const std::exception& e) {
            std::println("Invalid input! Please enter a number.");
        }
    }

    return false;
}

bool serve_spelling_lesson(const LessonEntry &lesson) {
    std::string target;
    std::ranges::transform(lesson.word, std::back_inserter(target),
                           [](unsigned char c) -> unsigned char { return std::tolower(c); });

    std::println("How do you spell {}?", lesson.origin_word);

    while (true) {
        std::string input;
        std::println("Your answer:");
        std::getline(std::cin, input);

        if (input.empty()) {
            continue;
        }

        std::ranges::transform(input, input.begin(),
                               [](unsigned char c) { return std::tolower(c); });

        if (input == "hint") {
            std::println("{}", lesson.description);
            continue;
        }

        if (input == "quit") {
            std::println("The correct spelling is: {} ", lesson.word);
            return false;
        }

        if (input == target) {
            std::println("Correct! The word is: {}", lesson.word);
            return true;
        } else {
            std::println("Incorrect. Try again.");
        }
    }
}
