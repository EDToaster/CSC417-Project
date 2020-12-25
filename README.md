CSC417 Project: Exploring the Tech of Noita
===========================================

## References
All references are from [this](https://www.youtube.com/watch?v=prXuyMCgbTc) gcd talk.

[Youtube Explanation](https://youtu.be/ixqOd76E_bU)

[![Recreating Noita](https://img.youtube.com/vi/ixqOd76E_bU/0.jpg)](https://www.youtube.com/watch?v=ixqOd76E_bU)

## User Guide
This project has been test-built on Windows using Visual Studio 2019. I'm not sure if they work on other platforms.

### Setup (Windows)
1. Clone this repository using `git clone --recursive` to get all of the submodules as well.
1. Create and `cd` into the `build` directory at the base of the repository.
1. Run `cmake ..`
1. Open the VS `RecreatingNoita.sln` file.
1. Under solution explorer, right click `main` > select Properties > Change `Output Directory` to the `build/` directory you created in step 2.
1. Build and run the executable using `RELEASE` mode!

### Controls
* Use the number keys to select a material (the name of the material is outputted into the console).
* Left click to place material, right click to erase material
* Scroll wheel to change the radius of the brush
* Space bar to start/stop the simulation. By default, the simulation starts paused.
* Tab to spawn rigid body bouncy balls (only if `SIMULATE_RIGID_BODIES` is set)

### Additional Configurations
Additional configuration is under the `USER SETTINGS` section of `main.cpp`.

| Variable | Use | Default |
| -------- | --- | ------- |
| `SIMULATE_RIGID_BODIES`   | Set this compile flag if you want to simulate rigid bodies. | SET |
| `SPAWN_BODY`              | Set this value to one of `NONE`, `PENDULUM`, `BOUNCE`, or `CAR` to spawn initial rigid bodies. | NONE |
| `DOUGLAS_PEUCKER`         | Set this compile flag if you want to approximate particle contours with the Douglas-Peucker algorithm. Greatly increases performance | SET |
| `DEBUG_DRAW`              | Set this compile flag if you want to show calculated contours. | UNSET |
| `LOAD_FROM_FILE`          | Set this compile flag if you want to load a file from disk as initial falling sand state | UNSET |
| `TEXTURE_FILE`            | Set this value to a `.b` file name under the `/assets/` folder. This is the initial state of the falling sand simulation | |
| `SIM_WIDTH`, `SIM_HEIGHT` | The dimensions of the simulation. Lower this if the simulation runs too slow. | 400, 300 |
| `RENDER_WIDTH`, `RENDER_HEIGHT` | The dimensions of the render window. Leave this as a whole number multiple of `SIM_WIDTH`, `SIM_HEIGHT` | 1200, 900 |
