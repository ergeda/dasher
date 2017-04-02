// ColourIO.cpp
//
// Copyright (c) 2002 Iain Murray

#include "../Common/Common.h"

#include "ColourIO.h"
#include <cstring>

using namespace Dasher;
using namespace std;
//using namespace expat;

// Track memory leaks on Windows to the line that new'd the memory
#ifdef _WIN32
#ifdef _DEBUG
#define DEBUG_NEW new( _NORMAL_BLOCK, THIS_FILE, __LINE__ )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

// TODO: Share information with AlphIO class?

CColourIO::CColourIO(CMessageDisplay *pMsgs) : AbstractXMLParser(pMsgs), BlankInfo() {
  CreateDefault();
}

void CColourIO::GetColours(std::vector <std::string >*ColourList) const {
  ColourList->clear();

  typedef std::map < std::string, ColourInfo >::const_iterator CI;
  CI End = Colours.end();

  for(CI Cur = Colours.begin(); Cur != End; Cur++)
    ColourList->push_back((*Cur).second.ColourID);
}

const CColourIO::ColourInfo & CColourIO::GetInfo(const std::string &ColourID) {
  if(ColourID == "")            // return Default if no colour scheme is specified
    return Colours["Default"];
  else {
    if(Colours.count(ColourID) != 0) {
      Colours[ColourID].ColourID = ColourID;    // Ensure consistency
      return Colours[ColourID];
    }
    else {
      // if we don't have the colour scheme they asked for, return default
      return Colours["Default"];
    }
  }
}

void CColourIO::CreateDefault() {
  // TODO: Urgh - replace with a table

  ColourInfo & Default = Colours["Default"];
  Default.ColourID = "Default";
  Default.Mutable = false;

  // white
  Default.Reds.push_back(255);
  Default.Greens.push_back(255);
  Default.Blues.push_back(255);

  // gray
  Default.Reds.push_back(150);
  Default.Greens.push_back(150);
  Default.Blues.push_back(150);

  // dark
  Default.Reds.push_back(70);
  Default.Greens.push_back(70);
  Default.Blues.push_back(70);

  // dark (alpha = 0.8)
  double alpha = 0.8;
  Default.Reds.push_back(95);
  Default.Greens.push_back(95);
  Default.Blues.push_back(95);

  // dark (alpha = 0.4)
  alpha = 0.4;
  Default.Reds.push_back(120);
  Default.Greens.push_back(120);
  Default.Blues.push_back(120);

  // color interpolation from red to yellow then to green
  const int channel_max = 240;
  int total = 28;
  for (int i = 0; i < total; ++i) {
      if (i <= total / 2) {
          Default.Reds.push_back(channel_max);
          Default.Greens.push_back((channel_max * i * 2) / total);
          Default.Blues.push_back(0);
      }
      else {
          Default.Reds.push_back((channel_max * (total - i) * 2) / total);
          Default.Greens.push_back(channel_max);
          Default.Blues.push_back(0);
      }
  }

  // mimic alpha(=0.8) blending (over white background)
  alpha = 0.8;
  for (int i = 0; i < total; ++i) {
      if (i <= total / 2) {
          Default.Reds.push_back(channel_max * alpha + (1- alpha) * 255);
          Default.Greens.push_back(((channel_max * i * 2) / total) * alpha + (1 - alpha) * 255);
          Default.Blues.push_back(0 * alpha + (1 - alpha) * 255);
      }
      else {
          Default.Reds.push_back(((channel_max * (total - i) * 2) / total) * alpha + (1 - alpha) * 255);
          Default.Greens.push_back(channel_max * alpha + (1 - alpha) * 255);
          Default.Blues.push_back(0 * alpha + (1 - alpha) * 255);
      }
  }

  // mimic alpha(=0.4) blending (over white background)
  alpha = 0.4;
  for (int i = 0; i < total; ++i) {
      if (i <= total / 2) {
          Default.Reds.push_back(channel_max * alpha + (1 - alpha) * 255);
          Default.Greens.push_back(((channel_max * i * 2) / total) * alpha + (1 - alpha) * 255);
          Default.Blues.push_back(0 * alpha + (1 - alpha) * 255);
      }
      else {
          Default.Reds.push_back(((channel_max * (total - i) * 2) / total) * alpha + (1 - alpha) * 255);
          Default.Greens.push_back(channel_max * alpha + (1 - alpha) * 255);
          Default.Blues.push_back(0 * alpha + (1 - alpha) * 255);
      }
  }
}

// Below here handlers for the Expat XML input library
////////////////////////////////////////////////////////////////////////////////////

void CColourIO::XmlStartHandler(const XML_Char *name, const XML_Char **atts) {

  CData = "";

  if(strcmp(name, "palette") == 0) {
    ColourInfo NewInfo;
    InputInfo = NewInfo;
    InputInfo.Mutable = isUser();
    while(*atts != 0) {
      if(strcmp(*atts, "name") == 0) {
        InputInfo.ColourID = *(atts+1);
      }
      atts += 2;
    }
    return;
  }
  if(strcmp(name, "colour") == 0) {
    while(*atts != 0) {
      if(strcmp(*atts, "r") == 0) {
        InputInfo.Reds.push_back(atoi(*(atts+1)));
      }
      if(strcmp(*atts, "g") == 0) {
        InputInfo.Greens.push_back(atoi(*(atts+1)));
      }
      if(strcmp(*atts, "b") == 0) {
        InputInfo.Blues.push_back(atoi(*(atts+1)));
      }
      atts += 2;
    }
    return;
  }
}
void CColourIO::XmlEndHandler(const XML_Char *name) {
  
  if(strcmp(name, "palette") == 0) {
    Colours[InputInfo.ColourID] = InputInfo;
    return;
  }
}

void CColourIO::XmlCData(const XML_Char *s, int len) {
  // CAREFUL: s points to a string which is NOT null-terminated.
  CData.append(s, len);
}
