#pragma once
#include <unordered_map>
#include <string>
#include <sstream>
#include <functional>

using namespace std;

enum class MediaType
{
    AUDIO = 0,
    VIDEO,
    SUBTITLES,
    CLOSED_CAPTIONS
};

enum class StreamType
{
    MEDIA = 0,
    IFRAME
};

enum class SortParameter
{
    BANDWIDTH = 0,
    AVG_BANDWIDTH,
    RESOLUTION,
    FRAMERATE,
    CODECS,
    CHANNELS,
    AUDIO_LANGUAGE,
    VIDEORANGE,
    DEFAULT
};

class HLSMasterPlaylist
{
public:
    struct MediaTag
    {
        string id;
        MediaType type;
        string uri;
        string name;
        string language;
        bool isDefault = false;
        bool autoSelect = false;

        // Some compressed audio formats use string specifiers
        // for special channel information
        string channels;

        // Not including CHARACTERISTICS or 
        // INSTREAM-ID parts due to scope

        bool operator < (const MediaTag& other) const
        {
            // Only support sorting on audio media types for now
            if (type != MediaType::AUDIO || other.type != MediaType::AUDIO)
            {
                return true;
            }

            switch (HLSMasterPlaylist::s_sortParam)
            {
                case SortParameter::DEFAULT:
                    // If a sorting parameter hasn't been defined, we cannot sort
                    throw logic_error("Cannot sort with no sorting method");

                case SortParameter::CHANNELS:
                    // For audio channel formats, we assume compressed formats will
                    // have longer lengths due to specifiers, and PCM formats will
                    // have just a channel count
                    if (channels.length() == other.channels.length())
                    {
                        return channels < other.channels;
                    }
                    return channels.length() < other.channels.length();

                case SortParameter::AUDIO_LANGUAGE:
                    return language < other.language;
                default:
                    // use default sorting by ID when other sorting methods are used
                    return id < other.id;
            }
        }

        friend ostream& operator << (ostream& os, const HLSMasterPlaylist::MediaTag& mediaTag);
    };

    struct Resolution
    {
        int width = 0;
        int height = 0;

        bool operator < (const Resolution& other) const
        {
            // Sorting is done by total height and width
            return (width + height) < (other.width + other.height);
        }

        friend ostream& operator << (ostream& os, const HLSMasterPlaylist::Resolution& resolution);
    };

    struct StreamInfo
    {
        StreamType type;
        long bandwidth = 0;
        long avgBandwidth = 0;

        // TODO: quantify codecs into a struct/class or enum
        string codecs;

        MediaTag* audio = nullptr;
        MediaTag* video = nullptr;
        MediaTag* closedCaptions = nullptr;
        Resolution resolution;
        string uri;
        float frameRate = 0;

        // TODO: Create better mechanism for handling video ranges
        string videoRange;

        // Override the < operator so that we can use sort functions
        // based on our sort parameter
        bool operator < (const StreamInfo& other) const
        {
            switch (HLSMasterPlaylist::s_sortParam)
            {
                case SortParameter::DEFAULT:
                    // If a sorting parameter hasn't been defined, we cannot sort
                    throw logic_error("Cannot sort with no sorting method");

                case SortParameter::AUDIO_LANGUAGE:
                case SortParameter::CHANNELS:
                    // Just return which language is alphabetically.
                    // If one stream does not have a language, the stream with
                    // a language will come first
                    if (!other.audio || (!audio && !other.audio))
                    {
                        return true;
                    }
                    else if (!audio)
                    {
                        return false;
                    }

                    return *audio < *(other.audio);

                case SortParameter::BANDWIDTH:
                    return bandwidth < other.bandwidth;

                case SortParameter::AVG_BANDWIDTH:
                    return avgBandwidth < other.avgBandwidth;

                case SortParameter::RESOLUTION:
                    return resolution < other.resolution;

                case SortParameter::CODECS:
                    // TODO: Sort codecs based on their profiles and complexity.
                    // For simplicity, we will sort them alphabetically for now
                    return codecs < other.codecs;

                case SortParameter::FRAMERATE:
                    return frameRate < other.frameRate;

                case SortParameter::VIDEORANGE:
                    // For now, just return which video range comes first alphabetically
                    return videoRange < other.videoRange;
                default:
                    throw logic_error("Unknown sort parameter");
            }
        }

        friend ostream& operator << (ostream& os, const HLSMasterPlaylist::StreamInfo& streamInfo);
    };

private:
    // Function map to process tags
    typedef void(HLSMasterPlaylist::*ParseHandler)(stringstream& playlist, string& tag);
    
    // Function and type maps for call direction
    static const unordered_map<string, ParseHandler> s_tagProcessor;
    static const unordered_map<string, MediaType> s_mediaTypes;
    
    // Media tags vector must be declared before streams. The default destructor will destroy
    // all streams first, which have a pointer reference into the corresponding media tags
    unordered_map<string, MediaTag> m_mediaTags;

    // Sortable lists of streams and media types
    vector<StreamInfo> m_streams;
    vector<StreamInfo> m_iStreams;
    vector<MediaTag> m_sortedMediaTypes;

    bool m_independentSegments = false;

    // Sort parameter must be static so that comparators can compare the correct
    // field of a stream or media tag
    static SortParameter s_sortParam;
    bool isAscendingSort = true;

public:
    // Parses an HLS playlist stored in a string stream into its media types,
    // streams, and i-streams. Once the playlist is parsed, it is auto sorted
    // by the currently selected sorting parameter. Calling this function multiple
    // times will clear the current built playlist
    void ParseMasterPlaylist(stringstream& playlist);

    // Updates the sorting parameter used and re-sorts the playlist parameters
    void Sort(SortParameter param, bool isAscending);
    
    friend ostream& operator << (ostream& os, const HLSMasterPlaylist& playlist);

private:
    void ParseMediaTag(stringstream& playlist, string& tag);
    void ParseStreamInfo(stringstream& playlist, string& tag);
    void ParseIStream(stringstream& playlist, string& tag);
    void BaseParseStreamInfo(stringstream& playlist, string& tag, StreamType type);
};