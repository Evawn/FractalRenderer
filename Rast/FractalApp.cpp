#include "FractalApp.hpp"
#include <nanogui/window.h>
#include <nanogui/glcanvas.h>
#include <nanogui/layout.h>
#include <nanogui/slider.h>
#include <nanogui/label.h>

#include <cpplocate/cpplocate.h>

// Fixed screen size is awfully convenient, but you can also
// call Screen::setSize to set the size after the Screen base
// class is constructed.
const int FractalApp::windowWidth = 800;
const int FractalApp::windowHeight = 600;

bool useDeferred = true;
int bufferNum = 0;

// Constructor runs after nanogui is initialized and the OpenGL context is current.
// Sets up camera, fsq, shading pass (potentially more later)
FractalApp::FractalApp()
    : nanogui::Screen(Eigen::Vector2i(windowWidth, windowHeight), "Fractal Time", false),
      backgroundColor(0.0f, 0.0f, 0.0f, 0.0f)
{

  const std::string resourcePath =
      cpplocate::locatePath("Rast/shaders", "", nullptr) + "Rast/shaders/";
  cout << resourcePath;

  // Set up a simple shader program by passing the shader filenames to the convenience constructor
  fractalShader.reset(new GLWrap::Program("Fractal Shader", {{GL_VERTEX_SHADER, resourcePath + "fsq.vert"},
                                                             {GL_FRAGMENT_SHADER, resourcePath + "march.fs"}}));

  // Create a camera in a default position, respecting the aspect ratio of the window.
  cam = shared_ptr<RTUtil::PerspectiveCamera>(new RTUtil::PerspectiveCamera(
      Eigen::Vector3f(0, 0, -5), // eye
      Eigen::Vector3f(0, 0, 0),
      Eigen::Vector3f(0, 1, 0), // up
      16.0 / 9.0,               // aspect
      1,
      50, // near, far
      1   // fov
      ));

  cc.reset(new RTUtil::DefaultCC(cam));
  Screen::setSize(Eigen::Vector2i(windowWidth, 9.0 / 16 * windowWidth));

  // Upload a two-triangle mesh for drawing a full screen quad
  Eigen::MatrixXf vertices(5, 4);
  vertices.col(0) << -1.0f, -1.0f, 0.0f, 0.0f, 0.0f;
  vertices.col(1) << 1.0f, -1.0f, 0.0f, 1.0f, 0.0f;
  vertices.col(2) << 1.0f, 1.0f, 0.0f, 1.0f, 1.0f;
  vertices.col(3) << -1.0f, 1.0f, 0.0f, 0.0f, 1.0f;

  Eigen::Matrix<float, 3, Eigen::Dynamic> positions = vertices.topRows<3>();
  Eigen::Matrix<float, 2, Eigen::Dynamic> texCoords = vertices.bottomRows<2>();

  fsqMesh.reset(new GLWrap::Mesh());
  fsqMesh->setAttribute(fractalShader->getAttribLocation("vert_position"), positions);
  fsqMesh->setAttribute(fractalShader->getAttribLocation("vert_texCoord"), texCoords);

  // Set viewport
  Eigen::Vector2i framebufferSize;
  glfwGetFramebufferSize(glfwWindow(), &framebufferSize.x(), &framebufferSize.y());
  glViewport(0, 0, framebufferSize.x(), framebufferSize.y());

  // p1angle = 90;
  buildGUI();
  performLayout();
  setVisible(true);
}

bool FractalApp::keyboardEvent(int key, int scancode, int action, int modifiers)
{
  if (Screen::keyboardEvent(key, scancode, action, modifiers))
    return true;

  if (action == GLFW_PRESS)
  {
    Eigen::Vector3f gaze = 0.1 * (cam->getTarget() - cam->getEye()).normalized();
    Eigen::Vector3f right = 0.1 * cam->getRight();
    switch (key)
    {
    case GLFW_KEY_ESCAPE:
      setVisible(false);
      return true;
    case GLFW_KEY_1:
      if (toggle == 0)
      {
        toggle = 1;
      }
      else
      {
        toggle = 0;
      }
      return true;
    case GLFW_KEY_W:
      cam->setEye(cam->getEye() + gaze);
      cam->setTarget(cam->getTarget() + gaze);
      return true;
    case GLFW_KEY_S:
      cam->setEye(cam->getEye() - gaze);
      cam->setTarget(cam->getTarget() - gaze);
      return true;
    case GLFW_KEY_D:
      cam->setEye(cam->getEye() + right);
      cam->setTarget(cam->getTarget() + right);
      return true;
    case GLFW_KEY_A:
      cam->setEye(cam->getEye() - right);
      cam->setTarget(cam->getTarget() - right);
      return true;
    default:
      return true;
    }
  }
  else if (action == GLFW_REPEAT)
  {
    Eigen::Vector3f gaze = 0.1 * (cam->getTarget() - cam->getEye()).normalized();
    Eigen::Vector3f right = 0.1 * cam->getRight();
    switch (key)
    {
    case GLFW_KEY_W:
      cam->setEye(cam->getEye() + gaze);
      cam->setTarget(cam->getTarget() + gaze);
      return true;
    case GLFW_KEY_S:
      cam->setEye(cam->getEye() - gaze);
      cam->setTarget(cam->getTarget() - gaze);
      return true;
    case GLFW_KEY_D:
      cam->setEye(cam->getEye() + right);
      cam->setTarget(cam->getTarget() + right);
      return true;
    case GLFW_KEY_A:
      cam->setEye(cam->getEye() - right);
      cam->setTarget(cam->getTarget() - right);
      return true;
    default:
      return true;
    }
  }
  return cc->keyboardEvent(key, scancode, action, modifiers);
}

bool FractalApp::mouseButtonEvent(const Eigen::Vector2i &p, int button, bool down, int modifiers)
{
  return Screen::mouseButtonEvent(p, button, down, modifiers) ||
         cc->mouseButtonEvent(p, button, down, modifiers);
}

bool FractalApp::mouseMotionEvent(const Eigen::Vector2i &p, const Eigen::Vector2i &rel, int button, int modifiers)
{
  // return Screen::mouseMotionEvent(p, rel, button, modifiers) ||
  //        cc->mouseMotionEvent(p, rel, button, modifiers);

  if (button == GLFW_MOUSE_BUTTON_2)
  {
    Eigen::Affine3f T = Eigen::Affine3f::Identity();
    Eigen::AngleAxisf sideRot(rel.x() * .005, Eigen::Vector3f(0, 1, 0));
    Eigen::AngleAxisf vertRot(rel.y() * .005, cam->getRight());
    cout << sideRot.matrix() << "\n";

    T *= Eigen::Translation3f(cam->getEye());
    T *= vertRot;
    T *= sideRot;
    T *= Eigen::Translation3f(-cam->getEye());

    cam->setTarget(T * cam->getTarget());
  }

  return Screen::mouseMotionEvent(p, rel, button, modifiers);
}

bool FractalApp::scrollEvent(const Eigen::Vector2i &p, const Eigen::Vector2f &rel)
{
  return Screen::scrollEvent(p, rel) ||
         cc->scrollEvent(p, rel);
}

void FractalApp::drawContents()
{
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
  {
    std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
  }

  GLWrap::checkGLError("drawContents start");
  glClearColor(backgroundColor.r(), backgroundColor.g(), backgroundColor.b(), backgroundColor.w());
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);

  currentTime = glfwGetTime();
  t = fmod(currentTime, 1);
  float theta = currentTime / 10;

  fractalShader->uniform("theta", theta);
  Eigen::AngleAxisf turn(theta, Eigen::Vector3f(1, 0, 0));

  // Eigen::Vector3f tar = cam->getTarget();
  // Eigen::Affine3f T = Eigen::Affine3f::Identity();
  // T *= Eigen::Translation3f(tar);
  // T *= turn;
  // T *= Eigen::Translation3f(-tar);
  // cam->setEye(T * cam->getEye());

  fractalShader->use();
  // cout << (Eigen::Affine3f::Identity() * turn).matrix();
  // cout << cam->getViewMatrix().inverse().matrix();
  fractalShader->uniform("turnMat", (Eigen::Affine3f::Identity() * turn).matrix());
  fractalShader->uniform("mPi", cam->getProjectionMatrix().inverse().matrix());
  fractalShader->uniform("mVi", cam->getViewMatrix().inverse().matrix());
  fractalShader->uniform("toggle", toggle);

  for (int i = 0; i < 2; i++)
  {
    fractalShader->uniform("angles[" + to_string(i) + "]", angles[i]);
    fractalShader->uniform("bias[" + to_string(i) + "]", bias[i]);
  }
  // fractalShader->uniform("ang1", this->angles[]);
  // fractalShader->uniform("ang2", this->angles[1]);
  // fractalShader->uniform("bias", this->bias);
  fsqMesh->drawArrays(GL_TRIANGLE_FAN, 0, 4);
  fractalShader->unuse();
}

void FractalApp::buildGUI()
{

  // Creates a window that has this screen as the parent.
  // NB: even though this is a raw pointer, nanogui manages this resource internally.
  // Do not delete it!
  controlPanel = new nanogui::Window(this, "Control Panel");
  controlPanel->setFixedWidth(220);
  controlPanel->setPosition(Eigen::Vector2i(15, 15));
  controlPanel->setLayout(new nanogui::BoxLayout(nanogui::Orientation::Vertical,
                                                 nanogui::Alignment::Middle,
                                                 5, 5));

  // Create a slider widget that adjusts the sun angle parameter
  nanogui::Widget *ang1Widget = new nanogui::Widget(controlPanel);
  ang1Widget->setLayout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal,
                                               nanogui::Alignment::Middle,
                                               0, 5));
  new nanogui::Label(ang1Widget, "Plane 1 Theta:");
  nanogui::Slider *ang1Slider = new nanogui::Slider(ang1Widget);
  ang1Slider->setRange(std::make_pair(0.0f, 360.0f));
  ang1Slider->setValue(this->angles[0]);
  ang1Slider->setCallback([this](float value)
                          { changeAngle(0, value); });

  // Create a slider that adjusts the turbidity
  nanogui::Widget *bias1Widget = new nanogui::Widget(controlPanel);
  bias1Widget->setLayout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal,
                                                nanogui::Alignment::Middle,
                                                0, 5));
  new nanogui::Label(bias1Widget, "Bias 1:");
  nanogui::Slider *bias1Slider = new nanogui::Slider(bias1Widget);
  bias1Slider->setRange(std::make_pair(0.0f, 2.0f));
  bias1Slider->setValue(0.0f);
  bias1Slider->setCallback([this](float value)
                           { changeBias(0, value); });

  // Create a slider widget that adjusts the sun angle parameter
  nanogui::Widget *ang2Widget = new nanogui::Widget(controlPanel);
  ang2Widget->setLayout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal,
                                               nanogui::Alignment::Middle,
                                               0, 5));
  new nanogui::Label(ang2Widget, "Plane 2 Theta:");
  nanogui::Slider *ang2Slider = new nanogui::Slider(ang2Widget);
  ang2Slider->setRange(std::make_pair(0.0f, 360.0f));
  ang2Slider->setValue(this->angles[1]);
  ang2Slider->setCallback([this](float value)
                          { changeAngle(1, value); });

  // Create a slider that adjusts the turbidity
  nanogui::Widget *bias2Widget = new nanogui::Widget(controlPanel);
  bias2Widget->setLayout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal,
                                                nanogui::Alignment::Middle,
                                                0, 5));
  new nanogui::Label(bias2Widget, "Bias 2:");
  nanogui::Slider *bias2Slider = new nanogui::Slider(bias2Widget);
  bias2Slider->setRange(std::make_pair(0.0f, 2.0f));
  bias2Slider->setValue(0.0f);
  bias2Slider->setCallback([this](float value)
                           { changeBias(1, value); });
}

void FractalApp::changeAngle(int i, float ang)
{
  this->angles[i] = ang;
}

void FractalApp::changeBias(int i, float b)
{
  this->bias[i] = b;
}