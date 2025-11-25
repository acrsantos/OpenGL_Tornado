# Tornado | Final Project for Computer Graphics & Visual Computing

## Group Members

- Narag, John Joseph Carlo
- Nuyda, Matthew Andrei
- Santos, Adrian Christian

## Setup Instructions

### 1. Download Dependency

- [FreeGLUT MSVC package (3.0.0)](https://www.transmissionzero.co.uk/software/freeglut-devel/)

### 2. Extract the ZIP files

Extract the downloaded FreeGLUT zip file to your preferred location.

### 3. Clone the Repository

```bash
git clone https://github.com/Mwahahahahahahaha/OpenGL_Tornado.git
```

### 4. Open the Solution File

Open the `.sln` file in Visual Studio.

### 5. Configure Include Directories

- Go to **Properties -> C/C++ -> General -> Additional Include Directories**
- Add the path to the `include` folder from the extracted FreeGLUT ZIP file.

### 6. Configure Library Directories

- Go to **Properties -> Linker -> General -> Additional Library Directories**
- Add the path to the `lib/x64` folder from the extracted FreeGLUT ZIP file.

### 7. Copy Binary File

- In the project’s root directory, create the folder `x64/Debug` (if it doesn’t exist).
- Copy the binary files from `bin/x64` in the extracted FreeGLUT ZIP file into this directory.

### 8. Run the Program

- Press the **Play** button in Visual Studio toolbar, or press `Ctrl + F5`
