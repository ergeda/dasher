// DasherViewDial.cpp
//
// Copyright (c) 2008 The Dasher Team
//
// This file is part of Dasher.
//
// Dasher is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// Dasher is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Dasher; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

#include "../Common/Common.h"

#ifdef _WIN32
#include "..\Win32\Common\WinCommon.h"
#endif

#include "DasherViewDial.h"
#include "DasherView.h"
#include "DasherTypes.h"
#include "Event.h"
#include "Observable.h"

#include <algorithm>
#include <iostream>
#include <limits>
#include <stdlib.h>

using namespace Dasher;
using namespace Opts;

// Track memory leaks on Windows to the line that new'd the memory
#ifdef _WIN32
#ifdef _DEBUG_MEMLEAKS
#define DEBUG_NEW new( _NORMAL_BLOCK, THIS_FILE, __LINE__ )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

// FIXME - quite a lot of the code here probably should be moved to
// the parent class (DasherView). I think we really should make the
// parent class less general - we're probably not going to implement
// anything which uses radically different co-ordinate transforms, and
// we can always override if necessary.

// FIXME - duplicated 'mode' code throught - needs to be fixed (actually, mode related stuff, Input2Dasher etc should probably be at least partially in some other class)

CDasherViewDial::CDasherViewDial(CSettingsUser *pCreateFrom, CDasherScreen *DasherScreen, Opts::ScreenOrientations orient)
: CDasherView(DasherScreen,orient), CSettingsUserObserver(pCreateFrom), m_Y1(4), m_Y2(0.95 * CDasherModel::MAX_Y), m_Y3(0.05 * CDasherModel::MAX_Y),
  m_bVisibleRegionValid(false), m_pSelected(NULL), m_offset(0) {

  //Note, nonlinearity parameters set in SetScaleFactor
  ScreenResized(DasherScreen);
}

CDasherViewDial::~CDasherViewDial() {}

void CDasherViewDial::SetOrientation(Opts::ScreenOrientations newOrient) {
  if (newOrient == GetOrientation()) return;
  CDasherView::SetOrientation(newOrient);
  m_bVisibleRegionValid=false;
  SetScaleFactor();
}

void CDasherViewDial::HandleEvent(int iParameter) {
  switch (iParameter) {
    case LP_MARGIN_WIDTH:
    case BP_NONLINEAR_Y:
    case LP_NONLINEAR_X:
    case LP_GEOMETRY:
      m_bVisibleRegionValid = false;
      SetScaleFactor();
  }
}

void CDasherViewDial::KeyDown(int iId) {
    if (iId == 100) { // mouse left-click
        // handle output to edit control
        if (m_pSelected) {
            m_pSelected->Output();
            m_pSelected->SetFlag(NF_SEEN, true);
            // set parent
            Model()->Make_root(m_pSelected);
            // update offset
            m_offset = m_cachedOffset;
        } 
    }
    else if (iId == 101) { // mouse right-click
        // handle delete/undo
        if (m_pSelected != NULL) {
            m_pSelected = m_pSelected->Parent();
            if (m_pSelected != NULL) m_pSelected->Undo();
            Model()->Reparent_root();
            if (m_pSelected->Parent() != NULL && m_pSelected->Parent()->Parent() == NULL) m_offset = 0;
        }
    }
}

CDasherNode *CDasherViewDial::Render(CDasherNode *pRoot, myint iRootMin, myint iRootMax, CExpansionPolicy &policy) {
    DASHER_ASSERT(pRoot != 0);

    m_iRenderCount = 0;
    screenint width = Screen()->GetWidth();
    screenint height = Screen()->GetHeight();
    // white background
    Screen()->DrawRectangle(0, 0, width, height, 0, -1, 0);

    // render dial
    screenint origin_x = width / 2;
    screenint origin_y = height / 2;
    const screenint dial_radius = 160;
    const screenint thickness_level0 = 50;
    const screenint thickness_level1 = 35;
    const screenint thickness_level2 = 25;
    const double PI = 3.141592654;
    const screenint line_thickness = 1;
    const screenint selected_line_thickness = 4;
    const int line_color = 1; // gray
    const int selected_line_color = 2; // dark

    // start angle offset
    screenint mouse_x, mouse_y;
    Input()->GetScreenCoords(mouse_x, mouse_y, this);
    double startAngle_offset = atan((mouse_y - origin_y) * -1.0 / (mouse_x - origin_x)) * 180.0 / PI;
    if (mouse_x - origin_x < 0) startAngle_offset += 180;
    else if (mouse_y - origin_y > 0) startAngle_offset += 360;

    // Level-0
    if (pRoot->ChildCount() == 0) policy.ExpandNode(pRoot); // expand the node if not yet
    CDasherNode *pDefault = pRoot->GetChildren()[0];
    for (CDasherNode::ChildMap::const_iterator I = pRoot->GetChildren().begin(), E = pRoot->GetChildren().end(); I != E; ++I) {
        if ((*I)->Range() > pDefault->Range()) pDefault = *I;
    }
    // decide pointer_index
    CDasherNode *pLevel1 = pDefault;
    int offset = CDasherModel::NORMALIZATION - (pLevel1->Lbnd() + pLevel1->Hbnd()) / 2;
    for (CDasherNode::ChildMap::const_iterator I = pRoot->GetChildren().begin(), E = pRoot->GetChildren().end(); I != E; ++I) {
        double lbnd = ((*I)->Lbnd() + offset) * 360.0 / CDasherModel::NORMALIZATION - startAngle_offset + m_offset;
        double hbnd = ((*I)->Hbnd() + offset) * 360.0 / CDasherModel::NORMALIZATION - startAngle_offset + m_offset;
        while (hbnd < 0) { lbnd += 360; hbnd += 360; }
        while (lbnd > 0) { lbnd -= 360; hbnd -= 360; }
        if (lbnd <= 0 && hbnd >= 0) { pLevel1 = *I; break; }
    }
    m_pSelected = pLevel1;
    // Level-1
    if (pLevel1->ChildCount() == 0) policy.ExpandNode(pLevel1);
    CDasherNode *pLevel2 = pLevel1->GetChildren()[0];
    for (CDasherNode::ChildMap::const_iterator I = pLevel1->GetChildren().begin(), E = pLevel1->GetChildren().end(); I != E; ++I) {
        if ((*I)->Range() > pLevel2->Range()) pLevel2 = *I;
    }
    // Level-2 (expand level-1's most probable child)
    if (pLevel2->ChildCount() == 0) policy.ExpandNode(pLevel2);
    CDasherNode *pLevel3 = pLevel2->GetChildren()[0];
    for (CDasherNode::ChildMap::const_iterator I = pLevel2->GetChildren().begin(), E = pLevel2->GetChildren().end(); I != E; ++I) {
        if ((*I)->Range() > pLevel3->Range()) pLevel3 = *I;
    }

    // Level-2 ring
    int count = 5 + 28 + 28;
    int radius = dial_radius + thickness_level0 + thickness_level1 + thickness_level2;
    // angle offset
    offset = CDasherModel::NORMALIZATION - (pLevel3->Lbnd() + pLevel3->Hbnd()) / 2;
    for (CDasherNode::ChildMap::const_iterator I = pLevel2->GetChildren().begin(), E = pLevel2->GetChildren().end(); I != E; ++I) {
        CDasherNode *pChild = *I;
        double startAngle = (pChild->Lbnd() + offset) * 360.0 / CDasherModel::NORMALIZATION + startAngle_offset;
        double sweepAngle = (pChild->Hbnd() - pChild->Lbnd()) * 360.0 / CDasherModel::NORMALIZATION;
        Screen()->DrawSolidArc(origin_x, origin_y, radius, startAngle, sweepAngle, count++, line_color, line_thickness);

        // render text label
        screenint label_x = origin_x + (radius - thickness_level2 / 2) * cos((startAngle + sweepAngle / 2) * PI / 180);
        screenint label_y = origin_y - (radius - thickness_level2 / 2) * sin((startAngle + sweepAngle / 2) * PI / 180);
        Screen()->DrawString(pChild->getLabel(), label_x, label_y, 20, 4);
    }

    // Level-1 ring
    count = 5 + 28;
    radius = dial_radius + thickness_level0 + thickness_level1;
    // angle offset
    offset = CDasherModel::NORMALIZATION - (pLevel2->Lbnd() + pLevel2->Hbnd()) / 2;
    for (CDasherNode::ChildMap::const_iterator I = pLevel1->GetChildren().begin(), E = pLevel1->GetChildren().end(); I != E; ++I) {
        CDasherNode *pChild = *I;
        double startAngle = (pChild->Lbnd() + offset) * 360.0 / CDasherModel::NORMALIZATION + startAngle_offset;
        double sweepAngle = (pChild->Hbnd() - pChild->Lbnd()) * 360.0 / CDasherModel::NORMALIZATION;
        Screen()->DrawSolidArc(origin_x, origin_y, radius, startAngle, sweepAngle, count++, line_color, line_thickness);

        // render text label
        screenint label_x = origin_x + (radius - thickness_level1 / 2) * cos((startAngle + sweepAngle / 2) * PI / 180);
        screenint label_y = origin_y - (radius - thickness_level1 / 2) * sin((startAngle + sweepAngle / 2) * PI / 180);
        Screen()->DrawString(pChild->getLabel(), label_x, label_y, 22, 3);
    }

    // Inner-most (level-0) ring
    count = 5;
    double selected_startAngle, selected_sweepAngle;
    int selected_color_index;
    screenint selected_label_x, selected_label_y;
    radius = dial_radius + thickness_level0;
    // angle offset
    offset = CDasherModel::NORMALIZATION - (pDefault->Lbnd() + pDefault->Hbnd()) / 2;
    for (CDasherNode::ChildMap::const_iterator I = pRoot->GetChildren().begin(), E = pRoot->GetChildren().end(); I != E; ++I) {
        CDasherNode *pChild = *I;
        double startAngle = (pChild->Lbnd() + offset) * 360.0 / CDasherModel::NORMALIZATION + m_offset;
        double sweepAngle = (pChild->Hbnd() - pChild->Lbnd()) * 360.0 / CDasherModel::NORMALIZATION;
        if (pChild == pLevel1) { selected_startAngle = startAngle; selected_sweepAngle = sweepAngle; selected_color_index = count; }
        else Screen()->DrawSolidArc(origin_x, origin_y, radius, startAngle, sweepAngle, count, line_color, line_thickness);
        count++;
        // render text label
        screenint label_x = origin_x + (radius - thickness_level0 / 2) * cos((startAngle + sweepAngle / 2) * PI / 180);
        screenint label_y = origin_y - (radius - thickness_level0 / 2) * sin((startAngle + sweepAngle / 2) * PI / 180);
        if (pChild == pLevel1) { selected_label_x = label_x; selected_label_y = label_y; }
        else Screen()->DrawString(pChild->getLabel(), label_x, label_y, 24, 2);
    }
    // render the selected one at last
    Screen()->DrawSolidArc(origin_x, origin_y, radius + 5, selected_startAngle, selected_sweepAngle, selected_color_index, selected_line_color, selected_line_thickness);
    Screen()->DrawString(pLevel1->getLabel(), selected_label_x, selected_label_y, 26, 2);
    // inner most white cover
    Screen()->DrawCircle(origin_x, origin_y, dial_radius, 0, selected_line_color, selected_line_thickness);

    // Pointer
    CDasherScreen::point p[4];
    p[0].x = origin_x + dial_radius*cos(startAngle_offset * PI / 180); p[0].y = origin_y - dial_radius*sin(startAngle_offset * PI / 180);
    p[1].x = origin_x + (dial_radius - 25)*cos(startAngle_offset * PI / 180) - 12*sin(startAngle_offset * PI / 180); p[1].y = origin_y - 12*cos(startAngle_offset * PI / 180) - (dial_radius - 25)*sin(startAngle_offset * PI / 180);
    p[2].x = origin_x + (dial_radius - 25)*cos(startAngle_offset * PI / 180) + 12*sin(startAngle_offset * PI / 180); p[2].y = origin_y + 12*cos(startAngle_offset * PI / 180) - (dial_radius - 25)*sin(startAngle_offset * PI / 180);
    p[3].x = p[0].x; p[3].y = p[0].y;
    Screen()->Polygon(p, 4, selected_line_color, selected_line_color, selected_line_thickness);

    // update offset
    m_cachedOffset = startAngle_offset;

    return NULL;
}

#if 0 // rotate mode
CDasherNode *CDasherViewDial::Render(CDasherNode *pRoot, myint iRootMin, myint iRootMax, CExpansionPolicy &policy) {
    DASHER_ASSERT(pRoot != 0);
    
    m_iRenderCount = 0;
    screenint width = Screen()->GetWidth();
    screenint height = Screen()->GetHeight();
    // white background
    Screen()->DrawRectangle(0, 0, width, height, 0, -1, 0);

    // render dial
    screenint origin_x = width / 2;
    screenint origin_y = height / 2;
    const screenint dial_radius = 160;
    const screenint thickness_level0 = 50;
    const screenint thickness_level1 = 35;
    const screenint thickness_level2 = 25;
    const double PI = 3.141592654;
    
    // start angle offset
    screenint mouse_x, mouse_y;
    Input()->GetScreenCoords(mouse_x, mouse_y, this);
    double startAngle_offset = atan((mouse_y - origin_y) * -1.0 / (mouse_x - origin_x)) * 180.0 / PI;
    if (mouse_x - origin_x < 0) startAngle_offset += 180;

    // Level-0
    CDasherNode *pOutput = pRoot->Parent();
    if (pRoot->ChildCount() == 0) policy.ExpandNode(pRoot); // expand the node if not yet
    CDasherNode *pDefault = pRoot->GetChildren()[0];
    for (CDasherNode::ChildMap::const_iterator I = pRoot->GetChildren().begin(), E = pRoot->GetChildren().end(); I != E; ++I) {
        if ((*I)->Range() > pDefault->Range()) pDefault = *I;
    }
    // decide pointer_index
    CDasherNode *pLevel1 = pDefault;
    int offset = CDasherModel::NORMALIZATION - (pLevel1->Lbnd() + pLevel1->Hbnd()) / 2;
    for (CDasherNode::ChildMap::const_iterator I = pRoot->GetChildren().begin(), E = pRoot->GetChildren().end(); I != E; ++I) {
        double lbnd = ((*I)->Lbnd() + offset) * 360.0 / CDasherModel::NORMALIZATION + startAngle_offset - 360;
        double hbnd = ((*I)->Hbnd() + offset) * 360.0 / CDasherModel::NORMALIZATION + startAngle_offset - 360;
        if (hbnd < 0) { lbnd += 360; hbnd += 360; }
        if (lbnd > 0) { lbnd -= 360; hbnd -= 360; }
        if (lbnd <= 0 && hbnd >= 0) { pLevel1 = *I; break; }
    }
    pOutput = pLevel1;

    // Level-1
    if (pLevel1->ChildCount() == 0) policy.ExpandNode(pLevel1);
    CDasherNode *pLevel2 = pLevel1->GetChildren()[0];
    for (CDasherNode::ChildMap::const_iterator I = pLevel1->GetChildren().begin(), E = pLevel1->GetChildren().end(); I != E; ++I) {
        if ((*I)->Range() > pLevel2->Range()) pLevel2 = *I;
    }
    // Level-2 (expand level-1's most probable child)
    if (pLevel2->ChildCount() == 0) policy.ExpandNode(pLevel2);
    CDasherNode *pLevel3 = pLevel2->GetChildren()[0];
    for (CDasherNode::ChildMap::const_iterator I = pLevel2->GetChildren().begin(), E = pLevel2->GetChildren().end(); I != E; ++I) {
        if ((*I)->Range() > pLevel3->Range()) pLevel3 = *I;
    }
    
    // Level-2 ring
    int count = 2;
    int radius = dial_radius + thickness_level0 + thickness_level1 + thickness_level2;
    // angle offset
    offset = CDasherModel::NORMALIZATION - (pLevel3->Lbnd() + pLevel3->Hbnd()) / 2;
    for (CDasherNode::ChildMap::const_iterator I = pLevel2->GetChildren().begin(), E = pLevel2->GetChildren().end(); I != E; ++I) {
        CDasherNode *pChild = *I;
        double startAngle = (pChild->Lbnd() + offset) * 360.0 / CDasherModel::NORMALIZATION;
        double sweepAngle = (pChild->Hbnd() - pChild->Lbnd()) * 360.0 / CDasherModel::NORMALIZATION;
        Screen()->DrawArc(origin_x, origin_y, radius, startAngle, sweepAngle, count++, 1, 2);

        // render text label
        screenint label_x = origin_x + (radius - thickness_level2 / 2) * cos((startAngle + sweepAngle / 2) * PI / 180);
        screenint label_y = origin_y - (radius - thickness_level2 / 2) * sin((startAngle + sweepAngle / 2) * PI / 180);
        Screen()->DrawString(pChild->getLabel(), label_x, label_y, 20, 1);
    }

    // Level-1 ring
    count = 2;
    radius = dial_radius + thickness_level0 + thickness_level1;
    // angle offset
    offset = CDasherModel::NORMALIZATION -(pLevel2->Lbnd() + pLevel2->Hbnd()) / 2;
    for (CDasherNode::ChildMap::const_iterator I = pLevel1->GetChildren().begin(), E = pLevel1->GetChildren().end(); I != E; ++I) {
        CDasherNode *pChild = *I;
        double startAngle = (pChild->Lbnd() + offset) * 360.0 / CDasherModel::NORMALIZATION;
        double sweepAngle = (pChild->Hbnd() - pChild->Lbnd()) * 360.0 / CDasherModel::NORMALIZATION;
        Screen()->DrawArc(origin_x, origin_y, radius, startAngle, sweepAngle, count++, 1, 2);

        // render text label
        screenint label_x = origin_x + (radius - thickness_level1 / 2) * cos((startAngle + sweepAngle / 2) * PI / 180);
        screenint label_y = origin_y - (radius - thickness_level1 / 2) * sin((startAngle + sweepAngle / 2) * PI / 180);
        Screen()->DrawString(pChild->getLabel(), label_x, label_y, 22, 1);
    }

    // Inner-most (level-0) ring
    count = 2;
    radius = dial_radius + thickness_level0;
    // angle offset
    offset = CDasherModel::NORMALIZATION -(pDefault->Lbnd() + pDefault->Hbnd()) / 2;
    for (CDasherNode::ChildMap::const_iterator I = pRoot->GetChildren().begin(), E = pRoot->GetChildren().end(); I != E; ++I) {
        CDasherNode *pChild = *I;
        double startAngle = (pChild->Lbnd() + offset) * 360.0 / CDasherModel::NORMALIZATION + startAngle_offset;
        double sweepAngle = (pChild->Hbnd() - pChild->Lbnd()) * 360.0 / CDasherModel::NORMALIZATION;
        if (pChild == pLevel1) Screen()->DrawArc(origin_x, origin_y, radius+5, startAngle, sweepAngle, 1, 1, 2);
        else Screen()->DrawArc(origin_x, origin_y, radius, startAngle, sweepAngle, count, 1, 2);
        count++;
        // render text label
        screenint label_x = origin_x + (radius - thickness_level0/2) * cos((startAngle + sweepAngle / 2) * PI / 180);
        screenint label_y = origin_y - (radius - thickness_level0/2) * sin((startAngle + sweepAngle / 2) * PI / 180);
        if (pChild == pLevel1) Screen()->DrawString(pChild->getLabel(), label_x, label_y, 26, 0);
        else Screen()->DrawString(pChild->getLabel(), label_x, label_y, 24, 1);
    }
    Screen()->DrawCircle(origin_x, origin_y, dial_radius, 0, 1, 2);
    
    // Pointer
    CDasherScreen::point p[4];
    p[0].x = origin_x + 200 - 40; p[0].y = origin_y;
    p[1].x = origin_x + 200 - 40 - 25; p[1].y = origin_y - 12;
    p[2].x = origin_x + 200 - 40 - 25; p[2].y = origin_y + 12;
    p[3].x = origin_x + 200 - 40; p[3].y = origin_y;
    Screen()->Polygon(p, 4, 1, 1, 2);
    // Pointed Label
    static int counter = 0;
    if (counter < 20) {
        Screen()->DrawString(pLevel1->getLabel(), p[0].x - 35, p[0].y, 24, 1);
    }
    ++counter; counter %= 30;

    return pOutput;
}
#endif

/// Draw text specified in Dasher co-ordinates. The position is
/// specified as two co-ordinates, intended to the be the corners of
/// the leading edge of the containing box.
CDasherViewDial::CTextString *CDasherViewDial::DasherDrawText(myint iDasherMaxX, myint iDasherMidY, CDasherScreen::Label *pLabel, CTextString *pParent, int iColor) {

  screenint x,y;
  Dasher2Screen(iDasherMaxX, iDasherMidY, x, y);

  //compute font size...
  int iSize = GetLongParameter(LP_DASHER_FONTSIZE);
  {
    const myint iMaxY(CDasherModel::MAX_Y);
    if (Screen()->MultiSizeFonts() && iSize>4) {
      //font size maxes out at ((iMaxY*3)/2)+iMaxY)/iMaxY = 3/2*smallest
      // which is reached when iDasherMaxX == iMaxY/2, i.e. the crosshair
      iSize = ((min(iDasherMaxX*3,(iMaxY*3)/2) + iMaxY) * iSize) / iMaxY;
    } else {
      //old style fonts; ignore iSize passed-in.
      myint iLeftTimesFontSize = (iMaxY - iDasherMaxX )*iSize;
      if(iLeftTimesFontSize < iMaxY * 19/ 20)
        iSize *= 20;
      else if(iLeftTimesFontSize < iMaxY * 159 / 160)
        iSize *= 14;
      else
        iSize *= 11;
    }
  }

  CTextString *pRet = new CTextString(pLabel, x, y, iSize, iColor);
  vector<CTextString *> &dest(pParent ? pParent->m_children : m_DelayedTexts);
  dest.push_back(pRet);
  return pRet;
}

void CDasherViewDial::DoDelayedText(CTextString *pText) {

  //note that it'd be better to compute old-style font sizes here, or even after shunting
  // across according to the aiMax array, but this needs Dasher co-ordinates, which were
  // more easily available at CTextString creation time. If it really doesn't look as good,
  // can put in extra calls to Screen2Dasher....
  screenint x(pText->m_ix), y(pText->m_iy);
  pair<screenint,screenint> textDims=Screen()->TextSize(pText->m_pLabel, pText->m_iSize);
  switch (GetOrientation()) {
    case Dasher::Opts::LeftToRight: {
      screenint iRight = x + textDims.first;
      if (iRight < Screen()->GetWidth()) {
        Screen()->DrawString(pText->m_pLabel, x, y-textDims.second/2, pText->m_iSize, pText->m_iColor);
        for (vector<CTextString *>::iterator it = pText->m_children.begin(); it!=pText->m_children.end(); it++) {
          CTextString *pChild=*it;
          pChild->m_ix = max(pChild->m_ix, iRight);
          DoDelayedText(pChild);
        }
        pText->m_children.clear();
      }
      break;
    }
    case Dasher::Opts::RightToLeft: {
      screenint iLeft = x-textDims.first;
      if (iLeft>=0) {
        Screen()->DrawString(pText->m_pLabel, iLeft, y-textDims.second/2, pText->m_iSize, pText->m_iColor);
        for (vector<CTextString *>::iterator it = pText->m_children.begin(); it!=pText->m_children.end(); it++) {
          CTextString *pChild=*it;
          pChild->m_ix = min(pChild->m_ix, iLeft);
          DoDelayedText(*it);
        }
        pText->m_children.clear();
      }
      break;
    }
    case Dasher::Opts::TopToBottom: {
      screenint iBottom = y + textDims.second;
      if (iBottom < Screen()->GetHeight()) {
        Screen()->DrawString(pText->m_pLabel, x-textDims.first/2, y, pText->m_iSize, pText->m_iColor);
        for (vector<CTextString *>::iterator it = pText->m_children.begin(); it!=pText->m_children.end(); it++) {
          CTextString *pChild=*it;
          pChild->m_iy = max(pChild->m_iy, iBottom);
          DoDelayedText(*it);
        }
        pText->m_children.clear();
      }
      break;
    }
    case Dasher::Opts::BottomToTop: {
      screenint iTop = y - textDims.second;
      if (y>=0) {
        Screen()->DrawString(pText->m_pLabel, x-textDims.first/2, iTop, pText->m_iSize, pText->m_iColor);
        for (vector<CTextString *>::iterator it = pText->m_children.begin(); it!=pText->m_children.end(); it++) {
          CTextString *pChild=*it;
          pChild->m_iy = min(pChild->m_iy, iTop);
          DoDelayedText(*it);
        }
        pText->m_children.clear();
      }
      break;
    }
    default:
      break;
  }
  delete pText;
}

CDasherViewDial::CTextString::~CTextString() {
  for (vector<CTextString *>::iterator it = m_children.begin(); it!=m_children.end(); it++)
    delete *it;
}

bool CDasherViewDial::IsSpaceAroundNode(myint y1, myint y2) {
  myint iVisibleMinX;
  myint iVisibleMinY;
  myint iVisibleMaxX;
  myint iVisibleMaxY;

  VisibleRegion(iVisibleMinX, iVisibleMinY, iVisibleMaxX, iVisibleMaxY);
  const myint maxX(y2-y1);
  if ((maxX < iVisibleMaxX) || (y1 > iVisibleMinY) || (y2 < iVisibleMaxY))
    return true; //space around sq => space around anything smaller!

  //in theory, even if the crosshair is off-screen (!), anything spanning y1-y2 should cover it...
  DASHER_ASSERT (CoversCrosshair(y2-y1, y1, y2));

  return false;
}

bool CDasherViewDial::CoversCrosshair(myint Range, myint y1, myint y2) {
  if (Range > CDasherModel::ORIGIN_X && y1 < CDasherModel::ORIGIN_Y && y2 > CDasherModel::ORIGIN_Y) {
      return true;
  }
  return false;
}

void CDasherViewDial::NewRender(CDasherNode *pRender, myint y1, myint y2,
                                  CTextString *pPrevText, CExpansionPolicy &policy, double dMaxCost,
                                  CDasherNode *&pOutput)
{
	//when we have only one child node to render, which'll be the last thing we
	// do before returning, we make a tail call by jumping here, rather than
	// pushing another stack frame:
beginning:
  DASHER_ASSERT_VALIDPTR_RW(pRender);

  ++m_iRenderCount;

  // Set the NF_SUPER flag if this node entirely frames the visual
  // area.

  myint iDasherMinX;
  myint iDasherMinY;
  myint iDasherMaxX;
  myint iDasherMaxY;
  VisibleRegion(iDasherMinX, iDasherMinY, iDasherMaxX, iDasherMaxY);
  pRender->SetFlag(NF_SUPER, !IsSpaceAroundNode(y1, y2));

  const int myColor = pRender->getColour();

  if( pRender->getLabel() )
  {
    const int textColor = GetLongParameter(LP_OUTLINE_WIDTH)<0 ? myColor : 4;
    myint ny1 = std::min(iDasherMaxY, std::max(iDasherMinY, y1)),
    ny2 = std::min(iDasherMaxY, std::max(iDasherMinY, y2));
    CTextString *pText = DasherDrawText(y2-y1, (ny1+ny2)/2, pRender->getLabel(), pPrevText, textColor);
    if (pRender->bShove()) pPrevText = pText;
  }

  const myint Range(y2-y1);
  // Draw node...we can both fill & outline in one go, since
  // we're drawing the whole node at once (unlike disjoint-rects),
  // as any part of the outline which is obscured by a child node,
  // will have the outline of the child node painted over it,
  // and all outlines are the same colour.

  //"invisible" nodes are given same colour as parent, so we neither fill
  // nor outline them. TODO this isn't quite right, as nodes that are
  // _supposed_ to be the same colour as their parent, will have no outlines...
  // (thankfully having 2 "phases" means this doesn't happen in standard
  // colour schemes)
  if (pRender->GetFlag(NF_VISIBLE)) {
	//outline width 0 = fill only; >0 = fill + outline; <0 = outline only
	int fillColour = GetLongParameter(LP_OUTLINE_WIDTH)>=0 ? myColor : -1;
	int lineWidth = abs(GetLongParameter(LP_OUTLINE_WIDTH));
    DasherDrawRectangle(std::min(Range, iDasherMaxX), std::max(y1, iDasherMinY), 0, std::min(y2, iDasherMaxY), fillColour, -1, lineWidth);
  }

  //Does node cover crosshair?
  if (pOutput == pRender->Parent() && CoversCrosshair(Range, y1, y2))
    pOutput = pRender;

  if (pRender->ChildCount() == 0) {
    if (pOutput==pRender) {
      //covers crosshair! forcibly populate, now!
      policy.ExpandNode(pRender);
    } else {
      //allow empty node to be expanded, it's big enough.
      policy.pushNode(pRender, y1, y2, true, dMaxCost);
      return; //no children atm => nothing more to do
    }
  } else {
    //Node has children. It can therefore be collapsed...however,
    // we don't allow a node covering the crosshair to be collapsed
    // (at best this'll mean there's nowhere useful to go forwards;
    // at worst, all kinds of crashes trying to do text output!)

    //No reason why we can't collapse a game mode node that's too small/offscreen
    // - we've got its coordinates, and can recreate its children and set their
    // NF_GAME flags appropriately when it becomes renderable again...
    if (pRender != pOutput)
      dMaxCost = policy.pushNode(pRender, y1, y2, false, dMaxCost);
  }
  //Node has children - either it already did, or else it covers the crosshair,
  // and we've just made them...so render them.

  //first check if there's only one child we need to render
  if (CDasherNode *pChild = pRender->onlyChildRendered) {
    //if child still covers screen, render _just_ it and return
    myint newy1 = y1 + (Range * pChild->Lbnd()) / CDasherModel::NORMALIZATION;
    myint newy2 = y1 + (Range * pChild->Hbnd()) / CDasherModel::NORMALIZATION;
    if (newy1 < iDasherMinY && newy2 > iDasherMaxY) { //covers entire y-axis!
         //render just that child; nothing more to do for this node => tail call to beginning
         pRender = pChild; y1=newy1; y2=newy2;
         goto beginning;
    }
    pRender->onlyChildRendered = NULL;
  }

  //ok, need to render all children...
  myint newy1=y1, newy2;
  CDasherNode::ChildMap::const_iterator I = pRender->GetChildren().begin(), E = pRender->GetChildren().end();
  while (I!=E) {
    CDasherNode *pChild(*I);

    newy2 = y1 + (Range * pChild->Hbnd()) / CDasherModel::NORMALIZATION;
    if (newy1<=iDasherMaxY && newy2 >= iDasherMinY) { //onscreen
      if (newy2-newy1 > GetLongParameter(LP_MIN_NODE_SIZE)) {
        //definitely big enough to render.
        NewRender(pChild, newy1, newy2, pPrevText, policy, dMaxCost, pOutput);
      } else if (!pChild->GetFlag(NF_SEEN)) pChild->Delete_children();
      if (newy2 > iDasherMaxY) {
        //remaining children offscreen and no game-mode child we might skip
        // (among the remainder, or any previous off the top of the screen)
        if (newy1 < iDasherMinY) pRender->onlyChildRendered = pChild; //previous children also offscreen!
        break; //skip remaining children
      }
    }
    I++;
    newy1=newy2;
  }
  if (I!=E) {
    //broke out of loop. Possibly more to delete...
    while (++I!=E) if (!(*I)->GetFlag(NF_SEEN)) (*I)->Delete_children();
  }
  //all children rendered.
}

/// Convert screen co-ordinates to dasher co-ordinates. This doesn't
/// include the nonlinear mapping for eyetracking mode etc - it is
/// just the inverse of the mapping used to calculate the screen
/// positions of boxes etc.
void CDasherViewDial::Screen2Dasher(screenint iInputX, screenint iInputY, myint &iDasherX, myint &iDasherY) {
  // Things we're likely to need:
  screenint iScreenWidth = Screen()->GetWidth();
  screenint iScreenHeight = Screen()->GetHeight();

  switch(GetOrientation()) {
  case Dasher::Opts::LeftToRight:
    iDasherX = ( iScreenWidth - iInputX ) * SCALE_FACTOR / iScaleFactorX;
    iDasherY = CDasherModel::MAX_Y / 2 + ( iInputY - iScreenHeight / 2 ) * SCALE_FACTOR / iScaleFactorY;
    break;
  case Dasher::Opts::RightToLeft:
    iDasherX = myint( ( iInputX ) * SCALE_FACTOR / iScaleFactorX);
    iDasherY = myint(CDasherModel::MAX_Y / 2 + ( iInputY - iScreenHeight / 2 ) * SCALE_FACTOR / iScaleFactorY);
    break;
  case Dasher::Opts::TopToBottom:
    iDasherX = myint( ( iScreenHeight - iInputY ) * SCALE_FACTOR / iScaleFactorX);
    iDasherY = myint(CDasherModel::MAX_Y / 2 + ( iInputX - iScreenWidth / 2 ) * SCALE_FACTOR / iScaleFactorY);
    break;
  case Dasher::Opts::BottomToTop:
    iDasherX = myint( ( iInputY  ) * SCALE_FACTOR / iScaleFactorX);
    iDasherY = myint(CDasherModel::MAX_Y / 2 + ( iInputX - iScreenWidth / 2 ) * SCALE_FACTOR / iScaleFactorY);
    break;
    default:
      break;
  }

  iDasherX = ixmap(iDasherX);
  iDasherY = iymap(iDasherY);
}

void CDasherViewDial::SetScaleFactor( void )
{
  //Parameters for X non-linearity.
  // Set some defaults here, in case we change(d) them later...
  m_iXlogThres = CDasherModel::MAX_Y/2; //threshold: DasherX's less than this are linear; those greater are logarithmic

  //set log scaling coefficient (unused if LP_NONLINEAR_X==0)
  // note previous value = 1/0.2, i.e. a value of LP_NONLINEAR_X =~= 4.8
  m_dXlogCoeff = exp(GetLongParameter(LP_NONLINEAR_X)/3.0);

  const bool bHoriz(GetOrientation() == Dasher::Opts::LeftToRight || GetOrientation() == Dasher::Opts::RightToLeft);
  const screenint iScreenWidth(Screen()->GetWidth()), iScreenHeight(Screen()->GetHeight());
  const double dPixelsX(bHoriz ? iScreenWidth : iScreenHeight), dPixelsY(bHoriz ? iScreenHeight : iScreenWidth);

  //Defaults/starting values, will be modified later according to scheme in use...
  iMarginWidth = GetLongParameter(LP_MARGIN_WIDTH);
  double dScaleFactorY(dPixelsY / CDasherModel::MAX_Y );
  double dScaleFactorX(dPixelsX / static_cast<double>(CDasherModel::MAX_Y + iMarginWidth) );

  if (dScaleFactorX < dScaleFactorY) {
      //fewer (pixels per dasher coord) in X direction - i.e., X is more compressed.
      //So, use X scale for Y too...except first, we'll _try_ to reduce the difference
      // by changing the relative scaling of X and Y (by at most 20%):
      double dMul = max(0.8, dScaleFactorX / dScaleFactorY);
      dScaleFactorY = std::max(dScaleFactorX / dMul, dScaleFactorY / 4.0);
      dScaleFactorX *= 0.9;
      iMarginWidth = (CDasherModel::MAX_Y / 20.0 + iMarginWidth*0.95) / 0.9;
  }
  else {
      //X has more room; use Y scale for both -> will get lots history
      // however, "compensate" by relaxing the default "relative scaling" of X
      // (normally only 90% of Y) towards 1...
      double dXmpc = std::min(1.0, 0.9 * dScaleFactorX / dScaleFactorY);
      dScaleFactorX = max(dScaleFactorY, dScaleFactorX / 4.0)*dXmpc;
      iMarginWidth = (iMarginWidth + dPixelsX / dScaleFactorX - CDasherModel::MAX_Y) / 2;
  }

  iScaleFactorX = myint(dScaleFactorX * SCALE_FACTOR);
  iScaleFactorY = myint(dScaleFactorY * SCALE_FACTOR);

#ifdef DEBUG
  //test...
  for (screenint x=0; x<iScreenWidth; x++) {
    dasherint dx, dy;
    Screen2Dasher(x, 0, dx, dy);
    screenint fx, fy;
    Dasher2Screen(dx, dy, fx, fy);
    if (fx!=x)
      std::cout << "ERROR ScreenX " << x << " becomes " << dx << " back to " << fx << std::endl;;
  }
  for (screenint y=0; y<iScreenHeight; y++) {
    dasherint dx,dy;
    Screen2Dasher(0, y, dx, dy);
    screenint fx,fy;
    Dasher2Screen(dx, dy, fx, fy);
    if (fy!=y)
      std::cout << "ERROR ScreenY " << y << " becomes " << dy << " back to " << fy << std::endl;
  }
#endif

  //notify listeners that coordinates have changed...
  Observable<CDasherView*>::DispatchEvent(this);
}


inline myint CDasherViewDial::CustomIDivScaleFactor(myint iNumerator) {
  // Integer division rounding away from zero

  long long int num, denom, quot, rem;
  myint res;

  num   = iNumerator;
  denom = SCALE_FACTOR;

  DASHER_ASSERT(denom != 0);

#ifdef HAVE_LLDIV
  lldiv_t ans = ::lldiv(num, denom);

  quot = ans.quot;
  rem  = ans.rem;
#else
  quot = num / denom;
  rem  = num % denom;
#endif

  if (rem < 0)
    res = quot - 1;
  else if (rem > 0)
    res = quot + 1;
  else
    res = quot;

  return res;
}

void CDasherViewDial::Dasher2Screen(myint iDasherX, myint iDasherY, screenint &iScreenX, screenint &iScreenY) {
  // Apply the nonlinearities
  iDasherX = xmap(iDasherX);
  iDasherY = ymap(iDasherY);

  // Things we're likely to need:
  screenint iScreenWidth = Screen()->GetWidth();
  screenint iScreenHeight = Screen()->GetHeight();

  // Note that integer division is rounded *away* from zero here to
  // ensure that this really is the inverse of the map the other way
  // around.
  switch( GetOrientation() ) {
  case Dasher::Opts::LeftToRight:
    iScreenX = screenint(iScreenWidth -
			 CustomIDivScaleFactor(iDasherX  * iScaleFactorX));
    iScreenY = screenint(iScreenHeight / 2 +
			 CustomIDivScaleFactor(( iDasherY - CDasherModel::MAX_Y / 2 ) * iScaleFactorY));
    break;
  case Dasher::Opts::RightToLeft:
    iScreenX = screenint(CustomIDivScaleFactor(iDasherX * iScaleFactorX));
    iScreenY = screenint(iScreenHeight / 2 +
			 CustomIDivScaleFactor( (iDasherY - CDasherModel::MAX_Y/2) * iScaleFactorY));
    break;
  case Dasher::Opts::TopToBottom:
    iScreenX = screenint(iScreenWidth / 2 +
			 CustomIDivScaleFactor( (iDasherY - CDasherModel::MAX_Y/2) * iScaleFactorY));
    iScreenY = screenint(iScreenHeight -
			 CustomIDivScaleFactor( iDasherX * iScaleFactorX ));
    break;
  case Dasher::Opts::BottomToTop:
    iScreenX = screenint(iScreenWidth / 2 +
			 CustomIDivScaleFactor(( iDasherY - CDasherModel::MAX_Y/2 ) * iScaleFactorY));
    iScreenY = screenint(CustomIDivScaleFactor( iDasherX  * iScaleFactorX ));
    break;
    default:
      break;
  }
}

void CDasherViewDial::Dasher2Polar(myint iDasherX, myint iDasherY, double &r, double &theta) {
	iDasherX = xmap(iDasherX);
    iDasherY = ymap(iDasherY);

  myint iDasherOX = xmap(CDasherModel::ORIGIN_X);
    myint iDasherOY = ymap(CDasherModel::ORIGIN_Y);

    double x = -(iDasherX - iDasherOX) / double(iDasherOX); //Use normalised coords so min r works
    double y = -(iDasherY - iDasherOY) / double(iDasherOY);
    theta = atan2(y, x);
    r = sqrt(x * x + y * y);
}

void CDasherViewDial::DasherLine2Screen(myint x1, myint y1, myint x2, myint y2, vector<CDasherScreen::point> &vPoints) {
  if (x1!=x2 && y1!=y2) { //only diagonal lines ever get changed...
    if (GetBoolParameter(BP_NONLINEAR_Y)) {
      if ((y1 < m_Y3 && y2 > m_Y3) ||(y2 < m_Y3 && y1 > m_Y3)) {
        //crosses bottom non-linearity border
        myint x_mid = x1+(x2-x1) * (m_Y3-y1)/(y2-y1);
        DasherLine2Screen(x1, y1, x_mid, m_Y3, vPoints);
        x1=x_mid; y1=m_Y3;
      }//else //no, a single line might cross _both_ borders!
      if ((y1 > m_Y2 && y2 < m_Y2) || (y2 > m_Y2 && y1 < m_Y2)) {
        //crosses top non-linearity border
        myint x_mid = x1 + (x2-x1) * (m_Y2-y1)/(y2-y1);
        DasherLine2Screen(x1, y1, x_mid, m_Y2, vPoints);
        x1=x_mid; y1=m_Y2;
      }
    }
    if (GetLongParameter(LP_NONLINEAR_X) && (x1 > m_iXlogThres || x2 > m_iXlogThres)) {
      //into logarithmic section
      CDasherScreen::point pStart, pScreenMid, pEnd;
      Dasher2Screen(x2, y2, pEnd.x, pEnd.y);
      for(;;) {
        Dasher2Screen(x1, y1, pStart.x, pStart.y);
        //a straight line on the screen between pStart and pEnd passes through pScreenMid:
        pScreenMid.x = (pStart.x + pEnd.x)/2;
        pScreenMid.y = (pStart.y + pEnd.y)/2;
        //whereas a straight line _in_Dasher_space_ passes through pDasherMid:
        myint xMid=(x1+x2)/2, yMid=(y1+y2)/2;
        CDasherScreen::point pDasherMid;
        Dasher2Screen(xMid, yMid, pDasherMid.x, pDasherMid.y);

        //since we know both endpoints are in the same section of the screen wrt. Y nonlinearity,
        //the midpoint along the DasherY axis of both lines should be the same.
        if (GetOrientation()==Dasher::Opts::LeftToRight || GetOrientation()==Dasher::Opts::RightToLeft) {
          DASHER_ASSERT(abs(pDasherMid.y - pScreenMid.y)<=1);//allow for rounding error
          if (abs(pDasherMid.x - pScreenMid.x)<=1) break; //call a straight line accurate enough
        } else {
          DASHER_ASSERT(abs(pDasherMid.x - pScreenMid.x)<=1);
          if (abs(pDasherMid.y - pScreenMid.y)<=1) break;
        }
        //line should appear bent. Subdivide!
        DasherLine2Screen(x1,y1,xMid,yMid,vPoints); //recurse for first half (to Dasher-space midpoint)
        if (x1==xMid || y1 == yMid) break; // as test on entry, only diagonal lines need to be bent...
        x1=xMid; y1=yMid; //& loop round for second half
      }
      //broke out of loop. a straight line (x1,y1)-(x2,y2) on the screen is an accurate portrayal of a straight line in Dasher-space.
      vPoints.push_back(pEnd);
      return;
    }
    //ok, not in x nonlinear section; fall through.
  }
#ifdef DEBUG
  CDasherScreen::point pTest;
  Dasher2Screen(x1, y1, pTest.x, pTest.y);
  DASHER_ASSERT(vPoints.back().x == pTest.x && vPoints.back().y == pTest.y);
#endif
  CDasherScreen::point p;
  Dasher2Screen(x2, y2, p.x, p.y);
  vPoints.push_back(p);
}

void CDasherViewDial::VisibleRegion( myint &iDasherMinX, myint &iDasherMinY, myint &iDasherMaxX, myint &iDasherMaxY ) {
  // TODO: Change output parameters to pointers and allow NULL to mean
  // 'I don't care'. Need to be slightly careful about this as it will
  // require a slightly more sophisticated caching mechanism

  if(!m_bVisibleRegionValid) {

    switch( GetOrientation() ) {
    case Dasher::Opts::LeftToRight:
      Screen2Dasher(Screen()->GetWidth(),0,m_iDasherMinX,m_iDasherMinY);
      Screen2Dasher(0,Screen()->GetHeight(),m_iDasherMaxX,m_iDasherMaxY);
      break;
    case Dasher::Opts::RightToLeft:
      Screen2Dasher(0,0,m_iDasherMinX,m_iDasherMinY);
      Screen2Dasher(Screen()->GetWidth(),Screen()->GetHeight(),m_iDasherMaxX,m_iDasherMaxY);
      break;
    case Dasher::Opts::TopToBottom:
      Screen2Dasher(0,Screen()->GetHeight(),m_iDasherMinX,m_iDasherMinY);
      Screen2Dasher(Screen()->GetWidth(),0,m_iDasherMaxX,m_iDasherMaxY);
      break;
    case Dasher::Opts::BottomToTop:
      Screen2Dasher(0,0,m_iDasherMinX,m_iDasherMinY);
      Screen2Dasher(Screen()->GetWidth(),Screen()->GetHeight(),m_iDasherMaxX,m_iDasherMaxY);
      break;
    default:
      break;
    }

    m_bVisibleRegionValid = true;
  }

  iDasherMinX = m_iDasherMinX;
  iDasherMaxX = m_iDasherMaxX;
  iDasherMinY = m_iDasherMinY;
  iDasherMaxY = m_iDasherMaxY;
}

void CDasherViewDial::ScreenResized(CDasherScreen *NewScreen) {
  m_bVisibleRegionValid = false;
  SetScaleFactor();
}
