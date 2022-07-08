#include <iostream>
#include <stdio.h>
#include <math.h>
#include <limits>
#include <typeinfo>

#include <Eigen/Core>
//#include "RastApp.hpp"
#include "FractalApp.hpp"

using namespace std;

int main(int argc, char const *argv[])
{
  //string fileName = "box.dae";
  //string fileName = argv[1];
  // string base = fileName.substr(0, fileName.find('.'));

  // shared_ptr<Scene> scene = Generator::generateScene(fileName);

  nanogui::init();

  nanogui::ref<FractalApp> app = new FractalApp();
  nanogui::mainloop(16);

  nanogui::shutdown();
}
