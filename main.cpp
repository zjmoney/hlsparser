#include <urlmon.h>
#include "HLSMasterPlaylist.h"
#include <memory>
#include <iostream>

// For URLOpenBlockingStream
#pragma comment(lib, "urlmon.lib")

void PrintUsage()
{
    cout << "Usage: hlsparser.exe <URL> [<sort_method> -reverseOrder]\n" <<
            "   Sorting Methods:\n" <<
            "   0 - Bandwidth\n" <<
            "   1 - Average Bandwidth\n" <<
            "   2 - Resolution\n" <<
            "   3 - Framerate\n" <<
            "   4 - Codecs\n" <<
            "   5 - Audio Channels\n" <<
            "   6 - Audio Language\n" <<
            "   7 - Video Range\n";
}

int main(int argc, char* argv[])
{
    if (argc < 2 || argc > 4)
    {
        PrintUsage();
        return 1;
    }

    SortParameter sortMethod = SortParameter::DEFAULT;
    if (argc > 2)
    {
        sortMethod = (SortParameter) atoi(argv[2]);
    }

    if (sortMethod > SortParameter::DEFAULT || sortMethod < SortParameter::BANDWIDTH)
    {
        PrintUsage();
        return 1;
    }

    bool sortAscending = true;
    if (argc > 3)
    {
        sortAscending = false;
    }

    // Custom deleter for IStream RAII
    unique_ptr<IStream, function<void(IStream*)>> stream(nullptr, [](IStream* s){ if (s) { s->Release(); }});

    // Use a standard windows API to easily grab the file
    IStream* tempStream = nullptr;
    HRESULT result = URLOpenBlockingStream(0, argv[1], &tempStream, 0, 0);
    if (result != 0)
    {
        PrintUsage();
        return 1;
    }

    stream.reset(tempStream);
    
    // Read the file into a string stream for parsing
    char buffer[1000];
    unsigned long bytesRead;
    stringstream ss;
    stream->Read(buffer, 1000, &bytesRead);
    while (bytesRead > 0U)
    {
        ss.write(buffer, (long long)bytesRead);
        stream->Read(buffer, 1000, &bytesRead);
    }

    unique_ptr<HLSMasterPlaylist> playlist = make_unique<HLSMasterPlaylist>();
    playlist->ParseMasterPlaylist(ss);

    playlist->Sort(sortMethod, sortAscending);
    
    cout << *playlist.get();

    return 0;
}