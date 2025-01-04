# JikkyoSubtitle
JikkyoSubtitle: Real-time transcription and translation of system audio into Chinese.

## Usage

1. **Download:** Obtain the pre-built binary from the latest release in the [Releases](https://github.com/Sg4Dylan/JikkyoSubtitle/releases) section.
2. **Configure:** Modify the `config.json` file. Detailed instructions for each setting are provided in `configs/README.md`.
3. **Launch:** Run `realtime-bilingual-asr.exe`.
4. **Settings:** After launching, you can adjust settings by right-clicking the system tray icon.

**Note:**

* Ensure that the directory containing `realtime-bilingual-asr.exe` has write permissions for the application to function correctly.
* Please refer to the [configs/README.md](https://github.com/Sg4Dylan/JikkyoSubtitle/blob/master/configs/README.md) for the first-time setup.

## Build

**Prerequisites:**

1. **Visual Studio with the "Desktop development with C++" workload:**  Ensure it includes the MSVC compiler and CMake.
2. **vcpkg:** Installed and integrated with your system (you've likely already done this if you have a vcpkg.json). Make sure `VCPKG_ROOT` environment variable is set and `vcpkg integrate install` has been run.
3. **Git:** For fetching the project (if applicable).
4. **(Optional) VC-LTL:** Just follow the steps for "Using VC-LTL in CMake." No changes to the CMake and vcpkg commands are needed.

**Steps:**

1. **Clone (if needed):**
    ```bash
    git clone https://github.com/Sg4Dylan/JikkyoSubtitle.git
    cd JikkyoSubtitle
    ```

2. **Install Dependencies (using vcpkg manifest mode):**
    ```bash
    vcpkg install
    ```

3. **Configure:**
    ```bash
    cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake
    ```
    *   `-B build`: Creates a `build` directory for out-of-source build.
    *   `-S .`: Specifies the source directory (current directory).
    *   `-DCMAKE_TOOLCHAIN_FILE`: Points CMake to the vcpkg toolchain file.

4. **Build:**
    ```bash
    cmake --build build --config Release
    ```
    *   `--config Release`: Builds the Release configuration. Change to `Debug` for debugging.

5. **Run:** Your executable should be in the `build/Release` or `build/Debug` folder.

## License

This project is licensed under the AGPL. It incorporates the following open-source projects:

* **Windows-universal-samples** (by microsoft): [MIT license](https://github.com/microsoft/Windows-universal-samples#MIT-1-ov-file).  
  Used to capture audio.
* **sherpa-onnx** (by k2-fsa): [Apache-2.0 license](https://github.com/k2-fsa/sherpa-onnx/blob/master/LICENSE).  
  Used for voice activity detection & speech recognition.
* **libwtfdanmaku** (by copyliu): [LGPL 2.1 License](https://github.com/copyliu/libwtfdanmaku/blob/master/LICENSE.txt).  
  Used for rendering on-screen subtitle.
* **IXWebSocket** (by machinezone): [BSD-3 Clause License](https://github.com/machinezone/IXWebSocket/blob/master/LICENSE.txt).   
  Used for HTTP/WebSocket implementation.
* **JSON for Modern C++** (by nlohmann): [MIT License](https://github.com/nlohmann/json/blob/develop/LICENSE.MIT).  
  Used for JSON serialization/deserialization. 
* **VC-LTL5** (by Chuyu-Team): [EPL-2.0 license](https://github.com/Chuyu-Team/VC-LTL5?tab=EPL-2.0-1-ov-file#readme).  
  Used to enhance program compatibility.

Please refer to the respective project repositories for full license details.