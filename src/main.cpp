#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <limits>
#include <filesystem>
#include <system_error>

struct LogEntry
{
    int lineNumber{};
    std::string fullText;
    std::string level;
};

struct LogSummary
{
    int totalEntries = 0;
    int infoCount = 0;
    int warningCount = 0;
    int errorCount = 0;
};

std::string toLowerCase(const std::string& text)
{
    std::string result = text;

    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char ch)
        {
            return static_cast<char>(std::tolower(ch));
        });

    return result;
}

std::string toUpperCase(const std::string& text)
{
    std::string result = text;

    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char ch)
        {
            return static_cast<char>(std::toupper(ch));
        });

    return result;
}

std::filesystem::path getDataFolder()
{
#ifdef LOG_DATA_DIR
    return std::filesystem::path(LOG_DATA_DIR);
#else
    std::filesystem::path current = std::filesystem::current_path();

    while (true)
    {
        std::filesystem::path possibleDataFolder = current / "data";

        if (std::filesystem::exists(possibleDataFolder) &&
            std::filesystem::is_directory(possibleDataFolder))
        {
            return possibleDataFolder;
        }

        if (!current.has_parent_path() || current == current.parent_path())
        {
            break;
        }

        current = current.parent_path();
    }

    return std::filesystem::current_path() / "data";
#endif
}

bool prepareDataFolder()
{
    std::error_code error;
    std::filesystem::create_directories(getDataFolder(), error);

    if (error)
    {
        std::cout << "Error: Could not create data folder." << std::endl;
        std::cout << "Folder: " << getDataFolder().string() << std::endl;
        return false;
    }

    return true;
}

bool createDefaultFilesIfMissing()
{
    if (!prepareDataFolder())
    {
        return false;
    }

    std::filesystem::path sampleLogFile = getDataFolder() / "sample_log.txt";
    std::filesystem::path summaryFile = getDataFolder() / "log_summary.txt";

    if (!std::filesystem::exists(sampleLogFile))
    {
        std::ofstream file(sampleLogFile);

        if (!file)
        {
            std::cout << "Error: Could not create sample_log.txt." << std::endl;
            return false;
        }

        file << "[INFO] Application started" << std::endl;
        file << "[INFO] User login successful" << std::endl;
        file << "[WARNING] Disk space is getting low" << std::endl;
        file << "[ERROR] Failed to connect to database" << std::endl;
        file << "[INFO] Retrying connection" << std::endl;
        file << "[ERROR] Database connection failed" << std::endl;
        file << "[WARNING] High memory usage detected" << std::endl;
        file << "[INFO] Application ended" << std::endl;
    }

    if (!std::filesystem::exists(summaryFile))
    {
        std::ofstream file(summaryFile);

        if (!file)
        {
            std::cout << "Error: Could not create log_summary.txt." << std::endl;
            return false;
        }
    }

    return true;
}

std::filesystem::path removeLeadingDataFolder(const std::filesystem::path& path)
{
    std::filesystem::path result;
    bool firstPart = true;
    bool removedData = false;

    for (const auto& part : path)
    {
        std::string partText = part.string();

        if (firstPart && toLowerCase(partText) == "data")
        {
            removedData = true;
            firstPart = false;
            continue;
        }

        result /= part;
        firstPart = false;
    }

    if (removedData)
    {
        return result;
    }

    return path;
}

std::filesystem::path resolveLogFilePath(const std::string& filename)
{
    std::filesystem::path userPath(filename);
    userPath = userPath.lexically_normal();

    if (userPath.is_absolute())
    {
        return userPath;
    }

    if (std::filesystem::exists(userPath))
    {
        return userPath;
    }

    std::filesystem::path dataFolder = getDataFolder();

    if (userPath.has_parent_path())
    {
        std::filesystem::path withoutData = removeLeadingDataFolder(userPath);
        return dataFolder / withoutData;
    }

    return dataFolder / userPath;
}

std::filesystem::path getReportFilePath()
{
    return getDataFolder() / "log_summary.txt";
}

void displayMenu()
{
    std::cout << std::endl;
    std::cout << "====================================" << std::endl;
    std::cout << "          Log File Analyzer" << std::endl;
    std::cout << "====================================" << std::endl;
    std::cout << "1. Load and Analyze Log File" << std::endl;
    std::cout << "2. Display Summary" << std::endl;
    std::cout << "3. Display Error Messages" << std::endl;
    std::cout << "4. Search Logs by Keyword" << std::endl;
    std::cout << "5. Save Summary Report" << std::endl;
    std::cout << "6. Exit" << std::endl;
    std::cout << "Please choose an option: ";
}

bool readMenuChoice(int& choice)
{
    std::cin >> choice;

    if (std::cin.fail())
    {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Invalid input. Please enter a number." << std::endl;
        return false;
    }

    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return true;
}

std::string extractLogLevel(const std::string& line)
{
    std::size_t start = line.find('[');
    std::size_t end = line.find(']');

    if (start == std::string::npos || end == std::string::npos || end <= start)
    {
        return "UNKNOWN";
    }

    return toUpperCase(line.substr(start + 1, end - start - 1));
}

bool analyzeLogFile(
    const std::filesystem::path& filename,
    std::vector<LogEntry>& entries,
    LogSummary& summary)
{
    std::ifstream file(filename);

    if (!file)
    {
        std::cout << "Error: Could not open log file." << std::endl;
        std::cout << "Tried to open: " << filename.string() << std::endl;
        std::cout << "Current running folder: "
            << std::filesystem::current_path().string()
            << std::endl;
        std::cout << "Data folder: "
            << getDataFolder().string()
            << std::endl;
        return false;
    }

    entries.clear();
    summary = LogSummary();

    std::string line;
    int lineNumber = 0;

    while (std::getline(file, line))
    {
        lineNumber++;

        if (line.empty())
        {
            continue;
        }

        std::string level = extractLogLevel(line);

        LogEntry entry;
        entry.lineNumber = lineNumber;
        entry.fullText = line;
        entry.level = level;

        entries.push_back(entry);

        summary.totalEntries++;

        if (level == "INFO")
        {
            summary.infoCount++;
        }
        else if (level == "WARNING")
        {
            summary.warningCount++;
        }
        else if (level == "ERROR")
        {
            summary.errorCount++;
        }
    }

    file.close();

    std::cout << "Log file analyzed successfully." << std::endl;
    std::cout << "Loaded file: " << filename.string() << std::endl;

    return true;
}

void displaySummary(const LogSummary& summary)
{
    std::cout << std::endl;
    std::cout << "========== Log Summary ==========" << std::endl;
    std::cout << "Total log entries : " << summary.totalEntries << std::endl;
    std::cout << "INFO messages     : " << summary.infoCount << std::endl;
    std::cout << "WARNING messages  : " << summary.warningCount << std::endl;
    std::cout << "ERROR messages    : " << summary.errorCount << std::endl;
}

bool hasLoadedLogs(const std::vector<LogEntry>& entries)
{
    if (entries.empty())
    {
        std::cout << "No log data loaded. Please analyze a log file first." << std::endl;
        return false;
    }

    return true;
}

void displayErrorMessages(const std::vector<LogEntry>& entries)
{
    if (!hasLoadedLogs(entries))
    {
        return;
    }

    bool found = false;

    std::cout << std::endl;
    std::cout << "========== Error Messages ==========" << std::endl;

    for (const LogEntry& entry : entries)
    {
        if (entry.level == "ERROR")
        {
            std::cout << "Line " << entry.lineNumber << ": "
                << entry.fullText << std::endl;
            found = true;
        }
    }

    if (!found)
    {
        std::cout << "No error messages found." << std::endl;
    }
}

void searchLogsByKeyword(const std::vector<LogEntry>& entries)
{
    if (!hasLoadedLogs(entries))
    {
        return;
    }

    std::string keyword;

    std::cout << "Enter keyword to search: ";
    std::getline(std::cin, keyword);

    if (keyword.empty())
    {
        std::cout << "Keyword cannot be empty." << std::endl;
        return;
    }

    std::string lowerKeyword = toLowerCase(keyword);
    bool found = false;

    std::cout << std::endl;
    std::cout << "========== Search Results ==========" << std::endl;

    for (const LogEntry& entry : entries)
    {
        std::string lowerText = toLowerCase(entry.fullText);

        if (lowerText.find(lowerKeyword) != std::string::npos)
        {
            std::cout << "Line " << entry.lineNumber << ": "
                << entry.fullText << std::endl;
            found = true;
        }
    }

    if (!found)
    {
        std::cout << "No matching log entries found." << std::endl;
    }
}

void saveSummaryReport(
    const LogSummary& summary,
    const std::vector<LogEntry>& entries,
    const std::filesystem::path& reportFilename)
{
    if (!hasLoadedLogs(entries))
    {
        return;
    }

    if (!prepareDataFolder())
    {
        return;
    }

    std::ofstream file(reportFilename);

    if (!file)
    {
        std::cout << "Error: Could not save summary report." << std::endl;
        std::cout << "Tried to save to: " << reportFilename.string() << std::endl;
        return;
    }

    file << "========== Log Summary Report ==========" << std::endl;
    file << "Total log entries : " << summary.totalEntries << std::endl;
    file << "INFO messages     : " << summary.infoCount << std::endl;
    file << "WARNING messages  : " << summary.warningCount << std::endl;
    file << "ERROR messages    : " << summary.errorCount << std::endl;

    file << std::endl;
    file << "========== Error Messages ==========" << std::endl;

    bool foundError = false;

    for (const LogEntry& entry : entries)
    {
        if (entry.level == "ERROR")
        {
            file << "Line " << entry.lineNumber << ": "
                << entry.fullText << std::endl;
            foundError = true;
        }
    }

    if (!foundError)
    {
        file << "No error messages found." << std::endl;
    }

    file.close();

    std::cout << "Summary report saved successfully." << std::endl;
    std::cout << "Saved to: " << reportFilename.string() << std::endl;
}

int main()
{
    std::vector<LogEntry> entries;
    LogSummary summary;
    std::filesystem::path reportFilename = getReportFilePath();
    int choice;

    createDefaultFilesIfMissing();

    while (true)
    {
        displayMenu();

        if (!readMenuChoice(choice))
        {
            continue;
        }

        switch (choice)
        {
        case 1:
        {
            std::string filename;

            std::cout << "Enter log file name: ";
            std::getline(std::cin, filename);

            if (filename.empty())
            {
                std::cout << "File name cannot be empty." << std::endl;
                break;
            }

            std::filesystem::path logFilePath = resolveLogFilePath(filename);
            analyzeLogFile(logFilePath, entries, summary);

            break;
        }

        case 2:
            if (hasLoadedLogs(entries))
            {
                displaySummary(summary);
            }
            break;

        case 3:
            displayErrorMessages(entries);
            break;

        case 4:
            searchLogsByKeyword(entries);
            break;

        case 5:
            saveSummaryReport(summary, entries, reportFilename);
            break;

        case 6:
            std::cout << "Thank you for using the Log File Analyzer." << std::endl;
            return 0;

        default:
            std::cout << "Invalid option. Please choose again." << std::endl;
        }
    }
}