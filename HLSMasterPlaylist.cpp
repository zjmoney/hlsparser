#include "HLSMasterPlaylist.h"
#include <iostream>
#include <algorithm>
#include <iterator>

const unordered_map<string, HLSMasterPlaylist::ParseHandler> HLSMasterPlaylist::s_tagProcessor =
{
    {"#EXT-X-MEDIA", &HLSMasterPlaylist::ParseMediaTag },
    {"#EXT-X-STREAM-INF", &HLSMasterPlaylist::ParseStreamInfo },
    {"#EXT-X-I-FRAME-STREAM-INF", &HLSMasterPlaylist::ParseIStream },
};

const unordered_map<string, MediaType> HLSMasterPlaylist::s_mediaTypes =
{
    {"AUDIO", MediaType::AUDIO },
    {"VIDEO", MediaType::VIDEO },
    {"CLOSED_CAPTIONS", MediaType::CLOSED_CAPTIONS },
    {"SUBTITLES", MediaType::SUBTITLES},
};

constexpr const char* MediaTypeToString(MediaType t)
{
    switch (t)
    {
        case MediaType::AUDIO: return "AUDIO";
        case MediaType::VIDEO: return "VIDEO";
        case MediaType::SUBTITLES: return "SUBTITLES";
        case MediaType::CLOSED_CAPTIONS: return "CLOSED-CAPTIONS";
        default: throw invalid_argument("Unimplemented item");
    }
}

constexpr const char* SortTypeToString(SortParameter t)
{
    switch (t)
    {
        case SortParameter::DEFAULT: return "DEFAULT"; 
        case SortParameter::BANDWIDTH: return "BANDWIDTH";
        case SortParameter::VIDEORANGE: return "VIDEORANGE";
        case SortParameter::AVG_BANDWIDTH: return "AVG_BANDWIDTH";
        case SortParameter::RESOLUTION: return "RESOLUTION";
        case SortParameter::CODECS: return "CODECS";
        case SortParameter::FRAMERATE: return "FRAMERATE";
        case SortParameter::CHANNELS: return "CHANNELS";
        case SortParameter::AUDIO_LANGUAGE: return "AUDIO_LANGUAGE";
        default: throw invalid_argument("Unimplemented item");
    }
}

SortParameter HLSMasterPlaylist::s_sortParam = SortParameter::DEFAULT;

void HLSMasterPlaylist::ParseMediaTag(stringstream& playlist, string& tag)
{
    MediaTag mediaTag;

    // Parse through the full media tag
    int curPos = 0;

    while (curPos < tag.length())
    {
        int nextDelim = tag.find('=', curPos);

        string field = tag.substr(curPos, nextDelim-curPos);

        curPos = nextDelim+1;

        // Find out if this is a quoted or unquoted field
        nextDelim = tag.find_first_of("\",", nextDelim);

        if (tag[nextDelim] == '\"')
        {
            // We are handling a string value
            nextDelim++;
            curPos = nextDelim;
            nextDelim = tag.find('\"', nextDelim);
        }

        string val = tag.substr(curPos, nextDelim-curPos);

        nextDelim = tag[nextDelim] == '\"' ? nextDelim+1 : nextDelim;

        // Find out what kind of val this is and populate the
        // corresponding field
        if (field == "TYPE")
        {
            mediaTag.type = s_mediaTypes.at(val);
        }
        else if (field == "GROUP-ID")
        {
            mediaTag.id = val;
        }
        else if (field == "NAME")
        {
            mediaTag.name = val;
        }
        else if (field == "LANGUAGE")
        {
            mediaTag.language = val;
        }
        else if (field == "DEFAULT")
        {
            mediaTag.isDefault = val == "YES" ? true : false;
        }
        else if (field == "AUTOSELECT")
        {
            mediaTag.autoSelect = val == "YES" ? true : false;
        }
        else if (field == "CHANNELS")
        {
            mediaTag.channels = val;
        }
        else if (field == "URI")
        {
            mediaTag.uri = val;
        }
        else
        {
            cout << "WARNING: Encountered unknown media field: " << field << "\n";
        }

        if (nextDelim < 0) break;
        
        // Update the current position
        curPos = nextDelim + 1;
    }

    // Add the media type to our media tags map
    m_mediaTags[mediaTag.id] = mediaTag;
}

void HLSMasterPlaylist::BaseParseStreamInfo(stringstream& playlist, string& tag, StreamType type)
{
    StreamInfo streamInfo;

    streamInfo.type = type;

    // Parse through the full stream tag
    int curPos = 0;

    while (curPos < tag.length())
    {
        int nextDelim = tag.find('=', curPos);

        string field = tag.substr(curPos, nextDelim-curPos);

        curPos = nextDelim+1;

        // Find out if this is a quoted or unquoted field
        nextDelim = tag.find_first_of("\",", nextDelim);

        if (tag[nextDelim] == '\"')
        {
            // We are handling a string value
            nextDelim++;
            curPos = nextDelim;
            nextDelim = tag.find('\"', nextDelim);
        }

        string val = tag.substr(curPos, nextDelim-curPos);

        nextDelim = tag[nextDelim] == '\"' ? nextDelim+1 : nextDelim;

        if (field == "BANDWIDTH")
        {
            streamInfo.bandwidth = atol(val.c_str());
        }
        else if (field == "AVERAGE-BANDWIDTH")
        {
            streamInfo.avgBandwidth = atol(val.c_str());
        }
        else if (field == "CODECS")
        {
            streamInfo.codecs = val;
        }
        else if (field == "RESOLUTION")
        {
            int resPos = val.find('x');
            Resolution res;
            res.width = atoi(val.substr(0, resPos).c_str());
            res.height = atoi(val.substr(resPos+1).c_str());
            streamInfo.resolution = res;
        }
        else if (field == "VIDEO-RANGE")
        {
            streamInfo.videoRange = val;
        }
        else if (field == "FRAME-RATE")
        {
            streamInfo.frameRate = atof(val.c_str());
        }
        else if (field == "AUDIO" && val != "NONE")
        {
            if (m_mediaTags.find(val) != m_mediaTags.end())
            {
                streamInfo.audio = &m_mediaTags[val];
            }
            else
            {
                // Create the tag and it will be populated later
                m_mediaTags[val] = MediaTag();
            }
        }
        else if (field == "CLOSED-CAPTIONS" && val != "NONE")
        {
            if (m_mediaTags.find(val) != m_mediaTags.end())
            {
                streamInfo.closedCaptions = &m_mediaTags[val];
            }
            else
            {
                // Create the tag and it will be populated later
                m_mediaTags[val] = MediaTag();
            }
        }
        else if (field == "VIDEO" && val != "NONE")
        {
            if (m_mediaTags.find(val) != m_mediaTags.end())
            {
                streamInfo.audio = &m_mediaTags[val];
            }
            else
            {
                // Create the tag and it will be populated later
                m_mediaTags[val] = MediaTag();
            }
        }
        else if (field == "URI")
        {
            streamInfo.uri = val;
        }
        else if (val != "NONE")
        {
            cout << "WARNING: Encountered unknown stream field: " << field << "\n";
        }

        if (nextDelim < 0) break;

        curPos = nextDelim + 1;
    }

    if (type == StreamType::MEDIA)
    {
        // The next line will contain the URI
        getline(playlist, streamInfo.uri);
    }

    // Add this stream to our streams list
    switch (type)
    {
        case StreamType::MEDIA:
            m_streams.push_back(streamInfo);
            break;
        case StreamType::IFRAME:
            m_iStreams.push_back(streamInfo);
            break;
        default:
            throw invalid_argument("Unknown stream type encountered");
            break;
    }
    
}

void HLSMasterPlaylist::ParseStreamInfo(stringstream& playlist, string& tag)
{
    BaseParseStreamInfo(playlist, tag, StreamType::MEDIA);
}


void HLSMasterPlaylist::ParseIStream(stringstream& playlist, string& tag)
{
    BaseParseStreamInfo(playlist, tag, StreamType::IFRAME);
}


void HLSMasterPlaylist::ParseMasterPlaylist(stringstream& playlist)
{
    string curLine;
    bool isValid = false;

    // Clear the current data if it exists
    m_iStreams.clear();
    m_mediaTags.clear();
    m_streams.clear();

    while (getline(playlist, curLine))
    {
        // The first line of the file should read #EXTM3U. If it does not, it is 
        // a malformed file
        if (curLine == "#EXTM3U")
        {
            isValid = true;
            continue;
        }

        // All valid lines will start with #EXT. If we have not read the #EXTM3U tag first,
        // then this file is incalid
        if (curLine.length() >= 4 && curLine.substr(0, 4) == "#EXT")
        {
            if (!isValid)
            {
                throw logic_error("Malformed HLS Playlist");
            }

            // Find the index of the end of the tag. The first 
            // part of the tag will map to our tag processor
            int curIdx = curLine.find(':');

            if (curIdx == string::npos)
            {
                // This is referencing a global tag. Apply it to our
                // playlist file
                // TODO: Add support for other global tags. For now,
                // only support the independent segments tag
                if (curLine == "#EXT-X-INDEPENDENT-SEGMENTS")
                {
                    m_independentSegments = true;
                }
                else
                {
                    cout << "WARNING: Encountered unknown global tag: " << curLine << "\n";
                }

                continue;
            }

            string tag = curLine.substr(0, curIdx);
            string attributeList = curLine.substr(curIdx+1);

            auto const tagProcessor = s_tagProcessor.find(tag);

            if (tagProcessor == s_tagProcessor.end())
            {
                cout << "WARNING: Encountered unknown tag: " << tag << "\n";
                continue;
            }

            (this->*(tagProcessor->second))(playlist, attributeList);
        }
    }

    Sort(s_sortParam, isAscendingSort);
}

void HLSMasterPlaylist::Sort(SortParameter sortParam, bool isAscending)
{
    s_sortParam = sortParam;
    m_sortedMediaTypes.clear();
    transform(m_mediaTags.begin(), m_mediaTags.end(), back_inserter(m_sortedMediaTypes),
            [](auto& kv){ return kv.second; });

    // Only perform sorting if a sort method has been selected
    if (s_sortParam != SortParameter::DEFAULT)
    {
        if (isAscending)
        {
            sort(m_streams.begin(), m_streams.end());
            sort(m_iStreams.begin(), m_iStreams.end());
            sort(m_sortedMediaTypes.begin(), m_sortedMediaTypes.end());
        }
        else
        {
            sort(m_streams.rbegin(), m_streams.rend());
            sort(m_iStreams.rbegin(), m_iStreams.rend());
            sort(m_sortedMediaTypes.begin(), m_sortedMediaTypes.end());
        }
    }

}

ostream& operator << (ostream& os, const HLSMasterPlaylist::MediaTag& mediaTag)
{
    os << "#EXT-X-MEDIA:" << "TYPE=" << MediaTypeToString(mediaTag.type) << 
        ",GROUP-ID=\"" << mediaTag.id << "\",NAME=\"" << mediaTag.name << "\",DEFAULT=" <<
        (mediaTag.isDefault ? "YES" : "NO") << ",AUTOSELECT=" << 
        (mediaTag.autoSelect ? "YES" : "NO");

    if (mediaTag.language != "") os << ",LANGUAGE=\"" << mediaTag.language << "\"";

    if (mediaTag.type == MediaType::AUDIO) os << ",CHANNELS=\"" << mediaTag.channels << "\"";

    if (mediaTag.uri != "") os << ",URI=\"" << mediaTag.uri << "\"";

    return os;
}

ostream& operator << (ostream& os, const HLSMasterPlaylist::Resolution& resolution)
{
    os << resolution.width << "x" << resolution.height;
    return os;
}

ostream& operator << (ostream& os, const HLSMasterPlaylist::StreamInfo& streamInfo)
{
    os << (streamInfo.type == StreamType::IFRAME ? "#EXT-X-I-FRAME-STREAM-INF:" : "#EXT-X-STREAM-INF:") <<
        "BANDWIDTH=" << streamInfo.bandwidth;

    if (streamInfo.avgBandwidth > 0) os << ",AVG-BANDWIDTH=" << streamInfo.avgBandwidth;

    os << ",CODECS=\"" << streamInfo.codecs << "\"";

    if (streamInfo.resolution.width > 0) os << ",RESOLUTION=" << streamInfo.resolution;
    if (streamInfo.frameRate > 0) os << ",FRAME-RATE=" << streamInfo.frameRate;
    if (streamInfo.videoRange != "") os << ",VIDEO-RANGE=" << streamInfo.videoRange;
    if (streamInfo.audio) os << ",AUDIO=\"" << streamInfo.audio->id << "\"";
    if (streamInfo.video) os << ",VIDEO=\"" << streamInfo.video->id << "\"";
    if (streamInfo.closedCaptions) os << ",CLOSED-CAPTIONS=\"" << streamInfo.closedCaptions->id << "\"";

    // Put the URI on a new line if the stream type is media to match formatting of a regular
    // HLS playlist
    streamInfo.type == StreamType::IFRAME ? (os << ",URI=\"" << streamInfo.uri << "\"") : (os << "\n" << streamInfo.uri);

    return os;
}

ostream& operator << (ostream& os, const HLSMasterPlaylist& playlist)
{
    // Output the info in the following way:
    //  1: output any global tags present
    //  2: output all media types in sorted order
    //  3. output all i-streams in sorted order
    //  4. Output all regular streams in sorted order

    os << "Sorting order: " << SortTypeToString(HLSMasterPlaylist::s_sortParam) << "\n";

    // Currently the only supported global tag is INDEPENDENT_SEGMENTS
    os << (playlist.m_independentSegments ? "#EXT-X-INDEPENDENT-SEGMENTS\n\n" : "\n");

    os << "Media Types:\n";
    for (const auto& mediaTag : playlist.m_sortedMediaTypes)
    {
        os << mediaTag << "\n";
    }

    os << "\nI-Frame Streams:\n";
    for (const auto& stream : playlist.m_iStreams)
    {
        os << stream << "\n";
    }

    os << "\nMedia Streams:\n";
    for (const auto& stream : playlist.m_streams)
    {
        os << stream << "\n";
    }

    return os;
}
