#include <filesystem>
#include <iostream>
#include <print>
#include <vector>
#include <fstream>
#include <random>

struct lesson_entry {
    uint8_t lesson_number;
    std::string word;
    std::string description;
    std::string origin_word;
};

enum LESSON_TYPE { RANDOM = 0, SPELLING = 1, MULTIPLE_CHOICE = 2, HANGMAN = 3 };

void recap_lesson(const std::vector<lesson_entry> &lessons);

// returns success or failure of lesson
bool serve_spelling_lesson(const lesson_entry &lesson);

// returns success or failure of lesson
bool serve_multiple_choice_lesson(const lesson_entry &lesson);

// returns success or failure of lesson
bool serve_hangman_lesson(const lesson_entry &lesson);

std::vector<lesson_entry> read_lesson_from_file(const std::filesystem::path &path, std::optional<uint8_t> lesson_no);

std::vector<std::string> split(std::string &s, const std::string &delimiter);

inline void clear_screen();

int main(const int argc, char *argv[]) {
    clear_screen();
    if (argc == 1) {
        std::println(std::cerr, "Usage {} \"filepath\" \"lesson number(optional)\" \"lesson type(optional)\"", argv[0]);
        return 1;
    }

    if (!std::filesystem::exists(argv[1])) {
        std::println(std::cerr, "File does not exist");
        return 1;
    }

    std::random_device r;
    std::default_random_engine rng(r());
    std::optional<uint8_t> lesson_no{};
    LESSON_TYPE lt = LESSON_TYPE::RANDOM;
    if (argc > 1) {
        const int temp = std::stoi(argv[2]);
        if (temp < -1 || temp > 255) {
            std::println("Lesson number must be between 0 and 255");
            return 1;
        }

        lesson_no = std::optional{static_cast<uint8_t>(temp)};
        std::println("Preparing Lesson No {} ...", lesson_no.value());
    }

    if (argc > 2) {
        const int temp = std::stoi(argv[3]);
        switch (temp) {
            case 1:
                lt = LESSON_TYPE::SPELLING;
                break;
            case 2:
                lt = LESSON_TYPE::MULTIPLE_CHOICE;
                break;

            case 3:
                lt = LESSON_TYPE::HANGMAN;
                break;
            default:
                lt = LESSON_TYPE::RANDOM;
                break;
        }

        if (lt == LESSON_TYPE::RANDOM) {
            std::uniform_int_distribution<> range(1, 3);
            switch (range(rng)) {
                case 1:
                    lt = LESSON_TYPE::SPELLING;
                    break;
                case 2:
                    lt = LESSON_TYPE::MULTIPLE_CHOICE;
                    break;
                case 3:
                    lt = LESSON_TYPE::HANGMAN;
                    break;
                default:
                    std::unreachable();
            }
        }
    }

    if (argc > 3) {
        std::println(
            std::cerr,
            "Too many parameter.\nUsage {} \"filepath\" \"lesson number(optional)\" \"lesson type(optional)\"",
            argv[0]);

        return 1;
    }

    const std::filesystem::path p = argv[1];

    auto lessons = read_lesson_from_file(p, lesson_no);
    std::println("\nRecap\n");
    recap_lesson(lessons);
    std::cin.get();
    clear_screen();

    std::println("\nStart lesson\n");
    // Seed with a real random value, if available

    std::ranges::shuffle(lessons, rng);

    switch (lt) {
        case LESSON_TYPE::SPELLING:
            for (const auto &lesson_entry: lessons) {
                serve_spelling_lesson(lesson_entry);
                std::cin.get();
                clear_screen();
            }
            break;
        case LESSON_TYPE::MULTIPLE_CHOICE:
            serve_multiple_choice_lesson(lessons.front());
            break;
        case LESSON_TYPE::HANGMAN:
            serve_hangman_lesson(lessons.front());
            break;
        default:
            std::unreachable();
    }

    return 0;
}

// A simple function to clear the console screen for different operating systems.
inline void clear_screen() {
#if defined(_WIN32) || defined(_WIN64)
    system("cls");
#else
    system("clear");
#endif
}

std::vector<std::string> split(std::string &s, const std::string &delimiter) {
    std::vector<std::string> tokens;
    tokens.reserve(4);
    size_t pos = 0;
    while ((pos = s.find(delimiter)) != std::string::npos) {
        std::string token = s.substr(0, pos);
        tokens.push_back(token);
        s.erase(0, pos + delimiter.length());
    }
    tokens.push_back(s);

    return tokens;
}

std::vector<lesson_entry> read_lesson_from_file(const std::filesystem::path &path,
                                                const std::optional<uint8_t> lesson_no) {
    std::vector<lesson_entry> result;

    if (std::ifstream file{path}; file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            auto words = split(line, ";");

            const bool has_valid_lesson_no = std::ranges::all_of(words[0], [](const unsigned char c) {
                return std::isdigit(c);
            });

            if (!has_valid_lesson_no) {
                std::println(std::cerr, "Invalid lesson number: {}!", words[0]);
                return {};
            }

            if (lesson_no.has_value()) {
                if (static_cast<uint8_t>(std::stoi(words[0])) != lesson_no.value()) {
                    continue;
                }
            }

            result.emplace_back(lesson_entry{
                static_cast<uint8_t>(std::stoi(words[0])),
                words[1],
                words[2],
                words[3]
            });
        }
        file.close();
    }

    result.shrink_to_fit();

    return result;
}

void recap_lesson(const std::vector<lesson_entry> &lessons) {
    for (auto lesson_entry: lessons) {
        std::println("{} ({})- {}", lesson_entry.word, lesson_entry.description, lesson_entry.origin_word);
    }
}

bool serve_hangman_lesson(const lesson_entry &lesson) {
    std::string target;
    target.resize(lesson.word.size());
    std::ranges::transform(lesson.word.begin(), lesson.word.end(), target.begin(),
                           [](unsigned char c) -> unsigned char { return std::tolower(c); });

    const auto len = target.size();
    std::string guess;
    guess.resize(len);
    for (auto i = 0; i < len; ++i) {
        if (lesson.word.at(i) == ' ') {
            guess.at(i) = ' ';
            continue;
        }
        guess.at(i) = '_';
    }

    while (guess.contains('_')) {
        clear_screen();
        std::println("Guess the word!\n Current: {}", guess);

        std::string input;
        std::getline(std::cin, input);

        if (input.empty()) {
            continue;
        }
        std::ranges::transform(input.begin(), input.end(), input.begin(),
                                 [](unsigned char c) -> unsigned char { return std::tolower(c); });
        if (input == "quit") {
            std::println("The word was: {} ", lesson.word);

            return false;
        }

        if (input == guess) {
            std::println("You found the word! \n{}\n{}\n{}", lesson.word, lesson.description, lesson.origin_word);
            return true;
        }

        if (input.length() == 1) {
            if (!target.contains(input)) {
                std::println("Wrong!");
                continue;
            }

            for (auto i = 0; i < len; ++i) {
                if (target.at(i) == input.at(0)) {
                    guess.at(i) = input.at(0);
                }
            }

            if (!guess.contains("_")) {
                std::println("You found the word! \n{}\n{}\n{}", lesson.word, lesson.description, lesson.origin_word);
                return true;
            }
        }
    }

    return false;
}

// returns success or failure of lesson
bool serve_multiple_choice_lesson(const lesson_entry &lesson) {
    // TODO
    return false;
}

bool serve_spelling_lesson(const lesson_entry &lesson) {
    std::string target;
    target.resize(lesson.word.size());
    std::ranges::transform(lesson.word.begin(), lesson.word.end(), target.begin(),
                           [](unsigned char c) -> unsigned char { return std::tolower(c); });
    std::println("How do you spell {} in Mongolian?", lesson.origin_word);
    while (true) {
        std::string input;
        std::getline(std::cin, input);

        if (input.empty()) {
            continue;
        }

        std::ranges::transform(input.begin(), input.end(), input.begin(),
                               [](unsigned char c) -> unsigned char { return std::tolower(c); });
        if (input == "hint") {
            std::println("{}", lesson.description);
            continue;
        }

        if (input == "quit") {
            std::println("You spell it: {} ", lesson.word);

            return false;
        }

        if (input == target) {
            return true;
        }
    }
}
