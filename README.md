# FractalRenderer
3D Fractal Renderer using Ray Marching and Distance Equations for Fractal

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
And switch between the Mandelbulb Fractal and Menger Sponge Fractal by pressing '1'.
