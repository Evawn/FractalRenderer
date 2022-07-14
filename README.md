# FractalRenderer
### 3D Fractal Renderer using Ray Marching and Distance Estimators for the Mandlebulb and Menger Sponge Fractals.
Made by Evan Azari and Chuckwuemeka Ubakanma

## INSTRUCTIONS TO RUN:
First, unzip ```ext.zip```
Then, in the main folder, run the following commands in the terminal:

```
mkdir build
cd build
cmake ..
make -j 8
```

This creates the executable Rast. You can render a scene by typing:

```
./Rast
```
- Switch between the Mandelbulb Fractal and Menger Sponge Fractal by pressing '1'
- Pan the camera using WASD
- Rotate the camera by clicking and dragging the mouse
