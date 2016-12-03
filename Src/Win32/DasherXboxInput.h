#pragma once
#include "../DasherCore/DasherInput.h"

namespace Dasher {
  class CDasherXboxInput;
} 

class Dasher::CDasherXboxInput : public CScreenCoordInput {
public:
  CDasherXboxInput(HWND _hwnd);
  ~CDasherXboxInput(void);

  virtual bool GetScreenCoords(screenint &iX, screenint &iY, CDasherView *pView);

private:
  HWND m_hwnd;
};
