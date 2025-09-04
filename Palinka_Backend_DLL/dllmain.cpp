#include <pybind11/pybind11.h>
#include <fmod.hpp>
#include <string>
#include <vector>
#include <filesystem>  
#include <iostream>
#include <Shlobj.h>
#include <conio.h>
#include <Mmsystem.h>
#include <mciapi.h>
#include <pybind11/stl.h>
#include <stdexcept>
#include <iostream>
#include "sqlite3.h"
namespace py = pybind11;

FMOD::System* System = nullptr; // central controller
FMOD::Sound* Sound = nullptr;   // audio file
FMOD::Channel* Channel = nullptr; // audio controller

sqlite3* db; // database pointer
char* dbErrMsg = 0;

std::string DbOpen(std::string name)
{
    if(sqlite3_open(name.c_str(), &db))
    {
		return "Error opening database (" + name + "): " + sqlite3_errmsg(db);
    }
    else
    {
        return "";
	}
}





// Function to insert a row into the "songs" table
xxx this needs to be a retval string function and return an string of errors or "" upon succsess
bool InsertSong( std::string name, std::string artists, int year) {
    sqlite3_stmt* stmt;
    const char* sql = "INSERT INTO songs (name,artists,year) VALUES (?,?,?)";

    // Prepare the statement
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    // Bind the parameters
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC); // bind name to first '?'
    sqlite3_bind_text(stmt, 2, artists.c_str(), -1, SQLITE_STATIC);         // bind score to second '?'
	sqlite3_bind_int(stmt, 3, year);          // bind year to third '?'
    // Execute the statement
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "Failed to execute statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }

    // Clean up
    sqlite3_finalize(stmt);
    return true;
}

//To be fair i have "partly" no idea what chatgpt cooked here but im just gonna assume this works
// Function to query the database and return all results as a vector of rows
// Each row is a vector of strings (one string per column)
//IMPORTANT! be carful with this as there may be vecot out of range erros as the function ONLY returns as much colums and rows are needed, e.g SELECT * FROM songs will return the whole table but SELECT name FROM songs will only return one collum, of course if you set extra modifierts like WHERE year = xxxx; then there will be less rows too. 
std::vector<std::vector<std::string>> QueryDatabase (const std::string& sql) {
    sqlite3_stmt* stmt;  // Prepared statement pointer
    std::vector<std::vector<std::string>> results;

    // 1. Prepare the SQL statement
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return results;  // return empty vector if preparation failed
    }

    // 2. Execute the statement and fetch all rows
    while (sqlite3_step(stmt) == SQLITE_ROW) {//brother wth
        int colCount = sqlite3_column_count(stmt);  // Number of columns in this row
        std::vector<std::string> row;

        // 3. Read each column in the current row
        for (int i = 0; i < colCount; i++) {
            const char* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
            // If column is NULL, store "NULL" string
            row.push_back(text ? text : "NULL");
        }

        // 4. Add the row to the results
        results.push_back(row);
    }

    // 5. Clean up: finalize the statement to avoid memory leaks
    sqlite3_finalize(stmt);

    return results;
}

//chatgpt cooked again 
int InstallChoclatey()
{
    // The exact command to install Chocolatey
    std::wstring command =
        L"Set-ExecutionPolicy Bypass -Scope Process -Force; "
        L"[System.Net.ServicePointManager]::SecurityProtocol = "
        L"[System.Net.ServicePointManager]::SecurityProtocol -bor 3072; "
        L"iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))";

    // Full PowerShell execution line
    std::wstring psCommand = L"powershell -NoProfile -ExecutionPolicy Bypass -Command \"" + command + L"\"";

    // Use ShellExecute to run as Administrator
    HINSTANCE result = ShellExecuteW(
        NULL,
        L"runas",         // "runas" triggers UAC prompt for admin
        L"powershell.exe",
        psCommand.c_str(),
        NULL,
        SW_SHOWNORMAL
    );

    if ((INT_PTR)result <= 32) {
        return -1; //error
    }
    else {
        return 0;
    }




}

//chatgpt cooked 
int FileNameToDBEntry(const std::string& filenameInput)
{
    std::vector<std::vector<std::string>> rows =
        QueryDatabase("SELECT id, name, artists FROM songs");

    // Lowercase filename for case-insensitive search
    std::string filename = filenameInput;
    std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);

    for (const auto& row : rows) {
        if (row.size() < 3) continue; // safety check

        std::string id = row[0];
        std::string name = row[1];
        std::string artist = row[2];

        // Lowercase DB strings
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        std::transform(artist.begin(), artist.end(), artist.begin(), ::tolower);

        // Check if both name and artist appear in filename
        if (filename.find(name) != std::string::npos &&
            filename.find(artist) != std::string::npos)
        {
            return std::stoi(id); // return actual DB id
        }
    }

    return -1; // not found
}

void InitFmod() {
    FMOD::System_Create(&System);
    System->init(512, FMOD_INIT_NORMAL, 0);
}

std::string FmodErrorToString(FMOD_RESULT result) {
    switch (result) {
    case FMOD_OK: return "No errors.";
    case FMOD_ERR_FILE_NOTFOUND: return "File not found.";
    case FMOD_ERR_INVALID_HANDLE: return "Invalid handle.";
    case FMOD_ERR_INVALID_PARAM: return "Invalid parameter.";
    case FMOD_ERR_MEMORY: return "Not enough memory.";
    case FMOD_ERR_SUBSOUND_ALLOCATED: return "Subsound already allocated.";
    case FMOD_ERR_INTERNAL: return "Internal error.";
    case FMOD_ERR_OUTPUT_INIT: return "Error initializing output device.";
    case FMOD_ERR_FORMAT: return "Unsupported file format.";
    default: return "Unknown FMOD error (" + std::to_string(result) + ")";
    }
}

std::string PlaySound_(const std::string& name) {
    if (!System) {
        return "FMOD system is not initialized. Call InitFmod() first.";
    }

    std::string path = "Media/" + name;
    FMOD_RESULT result;

    // Create the sound
    result = System->createSound(path.c_str(), FMOD_CREATESTREAM, nullptr, &Sound);
    if (result != FMOD_OK) {
        return "FMOD createSound failed: " + FmodErrorToString(result);
    }

    // Play the sound
    result = System->playSound(Sound, nullptr, false, &Channel);
    if (result != FMOD_OK) {
        return "FMOD playSound failed: " + FmodErrorToString(result);
    }

    if (result == FMOD_OK) {
		return ""; // Success
    }
    return "Unknown error.";
}
bool isPaused() {
    bool paused;
    if (Channel) {
        Channel->getPaused(&paused);
        return paused;
    }
	return false; // No channel means nothing is playing, hence not paused

}
void PauseSound() {
    if (Channel&&!isPaused()) Channel->setPaused(true);
}

void ResumeSound() {

    if (Channel&&isPaused()) Channel->setPaused(false);
}

void EndSound() {
    if (Sound) Sound->release();
    
}


int GetCurrentPosition()
{
    if (Channel)
    {
        unsigned int  positionInMs;
        Channel->getPosition(&positionInMs, FMOD_TIMEUNIT_MS);
        return round(positionInMs / 1000);
    }
    return -1;
    }
int GetLength()
{
    if (Channel)
    {
        unsigned int lengthInMs;
        Sound->getLength(&lengthInMs, FMOD_TIMEUNIT_MS);
        return round(lengthInMs / 1000);
    }
    return -1;
}
void MakeMediaFolder() {
    system("mkdir Media");
    system("cls");
}

std::vector<std::string> GetMediaFileNames() {
    std::string path = "Media";
    std::vector<std::string> buffer;

    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if (std::filesystem::is_regular_file(entry)) {
            buffer.push_back(entry.path().filename().string());
        }
    }
    return buffer;
}



void Cleanup() {
    EndSound();
    if (System) {
        System->close();
        System->release();
    }
    sqlite3_close(db);
}

// test function
int Add(int a, int b) {
    return a + b;
}

void ExamplePrint() {
    std::cout << "Hello from cpp";
}

PYBIND11_MODULE(Palinka, m) {
    m.def("InitFmod", &InitFmod, "Initialises FMOD [MUST BE DONE BEFORE CALLING A FMOD FUNCTION]");
	m.def("PlaySound", &PlaySound_, "Plays a sound. Takes a string as file name parameter, file must be in Media folder");//need an underscore because it appears this is also and function in fmod
    m.def("PauseSound", &PauseSound, "Pauses the currently playing sound");
    m.def("ResumeSound", &ResumeSound, "Resumes the currently paused sound");
    m.def("EndSound", &EndSound, "Ends the currently playing sound");
    m.def("MakeMediaFolder", &MakeMediaFolder, "Creates a Media folder in the current directory");
    m.def("GetMediaFileNames", &GetMediaFileNames, "Returns a list of file names in the Media folder");
    m.def("Cleanup", &Cleanup, "Cleans up FMOD resources, should be called before exiting the program");
    m.def("Add", &Add, "A function that adds two numbers");
    m.def("ExamplePrint", &ExamplePrint, "A function that prints a message to the console");
	m.def("GetCurrentPosition", &GetCurrentPosition, "Gets the current position of the playing sound in seconds");
	m.def("GetLength", &GetLength, "Gets the length of the currently loaded sound in seconds");
    m.def("DbOpen", &DbOpen, "Opens (or creates) a database with the given name");
    m.def("InsertSong", &InsertSong, "Inserts a song into the songs table");
	m.def("QueryDatabase", &QueryDatabase, "Executes a SQL query and returns the results as a vector of rows&collums (2D vector);IMPORTANT! be carful with this as there may be vecot out of range erros as the function ONLY returns as much colums and rows are needed, e.g SELECT * FROM songs will return the whole table but SELECT name FROM songs will only return one collum, of course if you set extra modifierts like WHERE year = xxxx; then there will be less rows too. ");
	m.def("FileNameToDBEntry", &FileNameToDBEntry, "Takes a filename as input and returns the corresponding database ID if a match is found, or -1 if no match exists. The function performs a case-insensitive search for both the song name and artist within the filename.");
	m.def("IsPaused", &isPaused, "Returns true if the currently playing sound is paused, false otherwise");
}
