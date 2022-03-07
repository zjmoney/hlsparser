# hlsparser

## Usage
This repro contains an HLS Parser which can parse and sort an HLS master playlist file from a URL.

```
Usage: hlsparser.exe <URL> [<sort_method> -reverseOrder]
   Sorting Methods:
   0 - Bandwidth
   1 - Average Bandwidth
   2 - Resolution
   3 - Framerate
   4 - Codecs
   5 - Audio Channels
   6 - Audio Language
   7 - Video Range
```
  
  Example:
  `hlsparser.exe https://lw.bamgrid.com/2.0/hls/vod/bam/ms02/hls/dplus/bao/master_unenc_hdr10_all.m3u8 2`
  
  would sort the playlist ascending by resolution.
  
  ## Building
  The project can be built with visual studio code using the VS build toolchain. Because the project uses a windows-only libarary for getting a URL, it cannot be built on linux or with the g++/gcc/clang compilers on windows:
  
  1. Ensure you have visual studio code and some version of the visual studio build tools installed
  2. Also ensure you have the C++ extensions installed for visual studio code
  3. Launch the visual studio command line interface "Developer Command Prompt"
  4. Navigate to the repro folder and open it with `code .`
  5. In the auto-generated .vscode generated directory, add the following build task:
```
{
  "type": "shell",
  "label": "cl.exe build active file",
  "command": "cl.exe",
  "args": [
    "/Zi",
    "/EHsc",
    "/Fe:",
    "${fileDirname}\\hlsparser.exe",
    "${workspaceFolder}\\*.cpp"
  ],
  "problemMatcher": [
    "$msCompile"
  ],
  "group": {
    "kind": "build",
    "isDefault": true
  }
}
 ```
 
 6. Launch the build task to build the project
