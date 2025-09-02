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

namespace py = pybind11;

FMOD::System* System = nullptr; // central controller
FMOD::Sound* Sound = nullptr;   // audio file
FMOD::Channel* Channel = nullptr; // audio controller

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
        return "Playing: " + name;
    }
    return "Unknown error.";
}

void PauseSound() {
    if (Channel) Channel->setPaused(true);
}

void ResumeSound() {
    if (Channel) Channel->setPaused(false);
}

void EndSound() {
    if (Sound) Sound->release();
}


int GetCurrentPosition()
{
        unsigned int  positionInMs;
        Channel->getPosition(&positionInMs, FMOD_TIMEUNIT_MS);
		return round(positionInMs / 1000);
}
int GetLength()
{
	unsigned int lengthInMs;
	Sound->getLength(&lengthInMs, FMOD_TIMEUNIT_MS);
	return round(lengthInMs / 1000);

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
}
