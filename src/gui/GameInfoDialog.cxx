//============================================================================
//
//   SSSS    tt          lll  lll
//  SS  SS   tt           ll   ll
//  SS     tttttt  eeee   ll   ll   aaaa
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "Bankswitch.hxx"
#include "Console.hxx"
#include "Cart.hxx"
#include "MouseControl.hxx"
#include "SaveKey.hxx"
#include "Dialog.hxx"
#include "EditTextWidget.hxx"
#include "RadioButtonWidget.hxx"
#include "Launcher.hxx"
#include "OSystem.hxx"
#include "CartDetector.hxx"
#include "ControllerDetector.hxx"
#include "PopUpWidget.hxx"
#include "Props.hxx"
#include "PropsSet.hxx"
#include "TabWidget.hxx"
#include "TIAConstants.hxx"
#include "Widget.hxx"
#include "Font.hxx"

#include "FrameBuffer.hxx"
#include "TIASurface.hxx"
#include "TIA.hxx"
#include "Switches.hxx"
#include "AudioSettings.hxx"

#include "GameInfoDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GameInfoDialog::GameInfoDialog(
      OSystem& osystem, DialogContainer& parent, const GUI::Font& font,
      GuiObject* boss, int max_w, int max_h)
  : Dialog(osystem, parent, font, "Game properties"),
    CommandSender(boss)
{
  const GUI::Font& ifont = instance().frameBuffer().infoFont();
  const int lineHeight   = font.getLineHeight(),
            fontWidth    = font.getMaxCharWidth(),
            fontHeight   = font.getFontHeight(),
            buttonHeight = font.getLineHeight() + 4,
            infoLineHeight = ifont.getLineHeight();
  const int VBORDER = 8;
  const int HBORDER = 10;
  const int VGAP = 4;

  int xpos, ypos, lwidth, fwidth, pwidth, tabID;
  WidgetArray wid;
  VariantList items, ports, ctrls;
  StaticTextWidget* t;

  // Set real dimensions
  setSize(HBORDER * 2 + 53 * fontWidth,
          8 * (lineHeight + VGAP) + 1 * (infoLineHeight + VGAP) + VBORDER * 2 + _th +
          buttonHeight + fontHeight + ifont.getLineHeight() + 20,
          max_w, max_h);

  // The tab widget
  myTab = new TabWidget(this, font, 2, 4 + _th, _w - 2 * 2,
                        _h - (_th + buttonHeight + 20));
  addTabWidget(myTab);

  //////////////////////////////////////////////////////////////////////////////
  // 1) Emulation properties
  tabID = myTab->addTab("Emulation", TabWidget::AUTO_WIDTH);

  ypos = VBORDER;

  t = new StaticTextWidget(myTab, font, HBORDER, ypos + 1, "Type (*)      ");
  pwidth = font.getStringWidth("CM (SpectraVideo CompuMate)");
  items.clear();
  for(uInt32 i = 0; i < uInt32(Bankswitch::Type::NumSchemes); ++i)
    VarList::push_back(items, Bankswitch::BSList[i].desc, Bankswitch::BSList[i].name);
  myBSType = new PopUpWidget(myTab, font, t->getRight() + 8, ypos,
                           pwidth, lineHeight, items, "");
  wid.push_back(myBSType);
  ypos += lineHeight + VGAP;

  myTypeDetected = new StaticTextWidget(myTab, ifont, t->getRight() + 8, ypos,
                                        "CM (SpectraVideo CompuMate) detected");
  ypos += ifont.getLineHeight() + VGAP;

  // Start bank
  myStartBankLabel = new StaticTextWidget(myTab, font, HBORDER, ypos + 1, "Start bank (*) ");
  items.clear();
  myStartBank = new PopUpWidget(myTab, font, myStartBankLabel->getRight(), ypos,
                                font.getStringWidth("AUTO"), lineHeight, items, "", 0, 0);
  wid.push_back(myStartBank);
  ypos += lineHeight + VGAP * 4;

  pwidth = font.getStringWidth("Auto-detect");
  t = new StaticTextWidget(myTab, font, HBORDER, ypos + 1, "TV format      ");
  items.clear();
  VarList::push_back(items, "Auto-detect", "AUTO");
  VarList::push_back(items, "NTSC", "NTSC");
  VarList::push_back(items, "PAL", "PAL");
  VarList::push_back(items, "SECAM", "SECAM");
  VarList::push_back(items, "NTSC50", "NTSC50");
  VarList::push_back(items, "PAL60", "PAL60");
  VarList::push_back(items, "SECAM60", "SECAM60");
  myFormat = new PopUpWidget(myTab, font, t->getRight(), ypos,
                             pwidth, lineHeight, items, "", 0, 0);
  wid.push_back(myFormat);

  myFormatDetected = new StaticTextWidget(myTab, ifont, myFormat->getRight() + 8, ypos + 4, "SECAM60 detected");


  // Phosphor
  ypos += lineHeight + VGAP;
  myPhosphor = new CheckboxWidget(myTab, font, HBORDER, ypos + 1, "Phosphor (enabled for all ROMs)", kPhosphorChanged);
  wid.push_back(myPhosphor);

  ypos += lineHeight + VGAP * 0;
  myPPBlend = new SliderWidget(myTab, font,
                               HBORDER + 20, ypos,
                               "Blend  ", 0, kPPBlendChanged, 4 * fontWidth, "%");
  myPPBlend->setMinValue(0); myPPBlend->setMaxValue(100);
  myPPBlend->setTickmarkIntervals(2);
  wid.push_back(myPPBlend);

  ypos += lineHeight + VGAP;
  t = new StaticTextWidget(myTab, font, HBORDER, ypos + 1, "V-Center ");
  myVCenter = new SliderWidget(myTab, font, t->getRight() + 2, ypos, "",
                               0, kVCenterChanged, 7 * fontWidth, "px", 0, true);

  myVCenter->setMinValue(TIAConstants::minVcenter);
  myVCenter->setMaxValue(TIAConstants::maxVcenter);
  myVCenter->setTickmarkIntervals(4);
  wid.push_back(myVCenter);

  ypos += lineHeight + VGAP * 3;
  mySound = new CheckboxWidget(myTab, font, HBORDER, ypos + 1, "Stereo sound");
  wid.push_back(mySound);

  // Add message concerning usage
  ypos = myTab->getHeight() - 5 - fontHeight - ifont.getFontHeight() - 10;
  new StaticTextWidget(myTab, ifont, HBORDER, ypos,
                       "(*) Changes require a ROM reload");

  // Add items for tab 0
  addToFocusList(wid, myTab, tabID);

  //////////////////////////////////////////////////////////////////////////////
  // 2) Console properties
  wid.clear();
  tabID = myTab->addTab(" Console ", TabWidget::AUTO_WIDTH);

  xpos = HBORDER; ypos = VBORDER;
  lwidth = font.getStringWidth(GUI::RIGHT_DIFFICULTY + " ");

  new StaticTextWidget(myTab, font, xpos, ypos + 1, "TV type");
  myTVTypeGroup = new RadioButtonGroup();
  RadioButtonWidget* r = new RadioButtonWidget(myTab, font, xpos + lwidth, ypos + 1,
                            "Color", myTVTypeGroup);
  wid.push_back(r);
  ypos += lineHeight;
  r = new RadioButtonWidget(myTab, font, xpos + lwidth, ypos + 1,
                            "B/W", myTVTypeGroup);
  wid.push_back(r);
  ypos += lineHeight + VGAP * 2;

  new StaticTextWidget(myTab, font, xpos, ypos+1, GUI::LEFT_DIFFICULTY);
  myLeftDiffGroup = new RadioButtonGroup();
  r = new RadioButtonWidget(myTab, font, xpos + lwidth, ypos + 1,
                            "A (Expert)", myLeftDiffGroup);
  wid.push_back(r);
  ypos += lineHeight;
  r = new RadioButtonWidget(myTab, font, xpos + lwidth, ypos + 1,
                            "B (Novice)", myLeftDiffGroup);
  wid.push_back(r);
  ypos += lineHeight + VGAP * 2;

  new StaticTextWidget(myTab, font, xpos, ypos+1, GUI::RIGHT_DIFFICULTY);
  myRightDiffGroup = new RadioButtonGroup();
  r = new RadioButtonWidget(myTab, font, xpos + lwidth, ypos + 1,
                            "A (Expert)", myRightDiffGroup);
  wid.push_back(r);
  ypos += lineHeight;
  r = new RadioButtonWidget(myTab, font, xpos + lwidth, ypos + 1,
                            "B (Novice)", myRightDiffGroup);
  wid.push_back(r);

  // Add items for tab 1
  addToFocusList(wid, myTab, tabID);

  //////////////////////////////////////////////////////////////////////////////
  // 3) Controller properties
  wid.clear();
  tabID = myTab->addTab("Controllers", TabWidget::AUTO_WIDTH);

  ctrls.clear();
  VarList::push_back(ctrls, "Auto-detect", "AUTO");
  VarList::push_back(ctrls, "Joystick", "JOYSTICK");
  VarList::push_back(ctrls, "Paddles", "PADDLES");
  VarList::push_back(ctrls, "Paddles_IAxis", "PADDLES_IAXIS");
  VarList::push_back(ctrls, "Paddles_IAxDr", "PADDLES_IAXDR");
  VarList::push_back(ctrls, "BoosterGrip", "BOOSTERGRIP");
  VarList::push_back(ctrls, "Driving", "DRIVING");
  VarList::push_back(ctrls, "Keyboard", "KEYBOARD");
  VarList::push_back(ctrls, "AmigaMouse", "AMIGAMOUSE");
  VarList::push_back(ctrls, "AtariMouse", "ATARIMOUSE");
  VarList::push_back(ctrls, "Trakball", "TRAKBALL");
  VarList::push_back(ctrls, "AtariVox", "ATARIVOX");
  VarList::push_back(ctrls, "SaveKey", "SAVEKEY");
  VarList::push_back(ctrls, "Sega Genesis", "GENESIS");
  VarList::push_back(ctrls, "KidVid", "KIDVID");
  VarList::push_back(ctrls, "Lightgun", "LIGHTGUN");
  VarList::push_back(ctrls, "MindLink", "MINDLINK");

  ypos = VBORDER;
  pwidth = font.getStringWidth("Paddles_IAxis");
  myLeftPortLabel = new StaticTextWidget(myTab, font, HBORDER, ypos+1, "Left port        ");
  myLeftPort = new PopUpWidget(myTab, font, myLeftPortLabel->getRight(),
                               myLeftPortLabel->getTop()-1,
                               pwidth, lineHeight, ctrls, "", 0, kLeftCChanged);
  wid.push_back(myLeftPort);
  ypos += lineHeight + VGAP;

  myLeftPortDetected = new StaticTextWidget(myTab, ifont, myLeftPort->getLeft(), ypos,
                                            "Sega Genesis detected");
  ypos += ifont.getLineHeight() + VGAP;

  myRightPortLabel = new StaticTextWidget(myTab, font, HBORDER, ypos+1, "Right port       ");
  myRightPort = new PopUpWidget(myTab, font, myRightPortLabel->getRight(),
                                myRightPortLabel->getTop()-1,
                                pwidth, lineHeight, ctrls, "", 0, kRightCChanged);
  wid.push_back(myRightPort);
  ypos += lineHeight + VGAP;
  myRightPortDetected = new StaticTextWidget(myTab, ifont, myRightPort->getLeft(), ypos,
                                             "Sega Genesis detected");
  ypos += ifont.getLineHeight() + VGAP + 4;

  mySwapPorts = new CheckboxWidget(myTab, font, myLeftPort->getRight() + fontWidth*4,
                                   myLeftPort->getTop()+1, "Swap ports");
  wid.push_back(mySwapPorts);
  mySwapPaddles = new CheckboxWidget(myTab, font, myRightPort->getRight() + fontWidth*4,
                                     myRightPort->getTop()+1, "Swap paddles");
  wid.push_back(mySwapPaddles);

  // EEPROM erase button for left/right controller
  //ypos += lineHeight + VGAP + 4;
  pwidth = myRightPort->getWidth();   //font.getStringWidth("Erase EEPROM ") + 23;
  myEraseEEPROMLabel = new StaticTextWidget(myTab, font, HBORDER, ypos, "AtariVox/SaveKey ");
  myEraseEEPROMButton = new ButtonWidget(myTab, font, myEraseEEPROMLabel->getRight(), ypos - 4,
                                         pwidth, buttonHeight, "Erase EEPROM", kEEButtonPressed);
  wid.push_back(myEraseEEPROMButton);
  myEraseEEPROMInfo = new StaticTextWidget(myTab, ifont, myEraseEEPROMButton->getRight() + 4,
                                           myEraseEEPROMLabel->getTop() + 3, "(for this game only)");

  ypos += lineHeight + VGAP * 4;
  myMouseControl = new CheckboxWidget(myTab, font, xpos, ypos + 1, "Specific mouse axes", kMCtrlChanged);
  wid.push_back(myMouseControl);

  // Mouse controller specific axis
  pwidth = font.getStringWidth("MindLink 0");
  items.clear();
  VarList::push_back(items, "None",       static_cast<uInt32>(MouseControl::Type::NoControl));
  VarList::push_back(items, "Paddle 0",   static_cast<uInt32>(MouseControl::Type::Paddle0));
  VarList::push_back(items, "Paddle 1",   static_cast<uInt32>(MouseControl::Type::Paddle1));
  VarList::push_back(items, "Paddle 2",   static_cast<uInt32>(MouseControl::Type::Paddle2));
  VarList::push_back(items, "Paddle 3",   static_cast<uInt32>(MouseControl::Type::Paddle3));
  VarList::push_back(items, "Driving 0",  static_cast<uInt32>(MouseControl::Type::Driving0));
  VarList::push_back(items, "Driving 1",  static_cast<uInt32>(MouseControl::Type::Driving1));
  VarList::push_back(items, "MindLink 0", static_cast<uInt32>(MouseControl::Type::MindLink0));
  VarList::push_back(items, "MindLink 1", static_cast<uInt32>(MouseControl::Type::MindLink1));

  xpos += 20;
  ypos += lineHeight + VGAP;
  myMouseX = new PopUpWidget(myTab, font, xpos, ypos, pwidth, lineHeight, items,
               "X-Axis is ");
  wid.push_back(myMouseX);

  ypos += lineHeight + VGAP;
  myMouseY = new PopUpWidget(myTab, font, myMouseX->getLeft(), ypos, pwidth, lineHeight, items,
               "Y-Axis is ");
  wid.push_back(myMouseY);

  xpos = HBORDER; ypos += lineHeight + VGAP;
  myMouseRange = new SliderWidget(myTab, font, HBORDER, ypos,
                                  "Mouse axes range ", 0, 0, fontWidth * 4, "%");
  myMouseRange->setMinValue(1); myMouseRange->setMaxValue(100);
  myMouseRange->setTickmarkIntervals(4);
  wid.push_back(myMouseRange);

  // Add items for tab 2
  addToFocusList(wid, myTab, tabID);

  //////////////////////////////////////////////////////////////////////////////
  // 4) Cartridge properties
  wid.clear();
  tabID = myTab->addTab("Cartridge", TabWidget::AUTO_WIDTH);

  xpos = HBORDER; ypos = VBORDER;
  lwidth = font.getStringWidth("Manufacturer ");
  fwidth = _w - lwidth - HBORDER * 2 - 2;
  new StaticTextWidget(myTab, font, xpos, ypos + 1, lwidth, fontHeight, "Name");
  myName = new EditTextWidget(myTab, font, xpos + lwidth, ypos - 1,
                              fwidth, lineHeight, "");
  wid.push_back(myName);

  ypos += lineHeight + VGAP;
  new StaticTextWidget(myTab, font, xpos, ypos + 1, lwidth, fontHeight, "MD5");
  myMD5 = new EditTextWidget(myTab, font, xpos + lwidth, ypos - 1,
                             fwidth, lineHeight, "");
  myMD5->setEditable(false);

  ypos += lineHeight + VGAP;
  new StaticTextWidget(myTab, font, xpos, ypos + 1, lwidth, fontHeight, "Manufacturer");
  myManufacturer = new EditTextWidget(myTab, font, xpos + lwidth, ypos - 1,
                                      fwidth, lineHeight, "");
  wid.push_back(myManufacturer);

  ypos += lineHeight + VGAP;
  new StaticTextWidget(myTab, font, xpos, ypos + 1, lwidth, fontHeight,
                       "Model", TextAlign::Left);
  myModelNo = new EditTextWidget(myTab, font, xpos + lwidth, ypos - 1,
                                 fwidth, lineHeight, "");
  wid.push_back(myModelNo);

  ypos += lineHeight + VGAP;
  new StaticTextWidget(myTab, font, xpos, ypos + 1, lwidth, fontHeight, "Rarity");
  myRarity = new EditTextWidget(myTab, font, xpos + lwidth, ypos - 1,
                                fwidth, lineHeight, "");
  wid.push_back(myRarity);

  ypos += lineHeight + VGAP;
  new StaticTextWidget(myTab, font, xpos, ypos + 1, lwidth, fontHeight, "Note");
  myNote = new EditTextWidget(myTab, font, xpos + lwidth, ypos - 1,
                              fwidth, lineHeight, "");
  wid.push_back(myNote);

  // Add items for tab 3
  addToFocusList(wid, myTab, tabID);

  //////////////////////////////////////////////////////////////////////////////
  // 4) High Scores properties
  wid.clear();
  tabID = myTab->addTab("High Scores", TabWidget::AUTO_WIDTH);

  xpos = HBORDER; ypos = VBORDER;
  lwidth = font.getStringWidth("Variations ");

  myHighScores = new CheckboxWidget(myTab, font, xpos, ypos + 1, "Enable High Scores", kHiScoresChanged);

  xpos += 20; ypos += lineHeight + VGAP;

  items.clear();
  VarList::push_back(items, "1", "1");
  VarList::push_back(items, "2", "2");
  VarList::push_back(items, "3", "3");
  VarList::push_back(items, "4", "4");
  pwidth = font.getStringWidth("4");

  myPlayersLabel = new StaticTextWidget(myTab, font, xpos, ypos + 1, lwidth, fontHeight, "Players");
  myPlayers = new PopUpWidget(myTab, font, xpos + lwidth, ypos, pwidth, lineHeight, items, "", 0, kPlayersChanged);
  wid.push_back(myPlayers);

  int awidth = font.getStringWidth("FFFF") + 4;
  EditableWidget::TextFilter fAddr = [](char c) {
    return (c >= 'a' && c <= 'f') || (c >= '0' && c <= '9');
  };
  int rwidth = font.getStringWidth("FF") + 4;

  myPlayersAddressLabel = new StaticTextWidget(myTab, font, myPlayers->getRight() + 16, ypos + 1, "Address ");
  myPlayersAddress = new EditTextWidget(myTab, font, myPlayersAddressLabel->getRight(), ypos - 1, awidth, lineHeight);
  myPlayersAddress->setTextFilter(fAddr);
  wid.push_back(myPlayersAddress);
  myPlayersAddressVal = new EditTextWidget(myTab, font, myPlayersAddress->getRight() + 2, ypos - 1, rwidth, lineHeight, "56");
  myPlayersAddressVal->setEditable(false);

  ypos += lineHeight + VGAP;

  fwidth = font.getStringWidth("255") + 5;
  myVariationsLabel = new StaticTextWidget(myTab, font, xpos, ypos + 1, lwidth, fontHeight, "Variations");
  myVariations = new EditTextWidget(myTab, font, xpos + lwidth, ypos - 1, fwidth, lineHeight);
  wid.push_back(myVariations);

  myVarAddressLabel = new StaticTextWidget(myTab, font, myPlayersAddressLabel->getLeft(), ypos + 1, "Address ");
  myVarAddress = new EditTextWidget(myTab, font, myVarAddressLabel->getRight(), ypos - 1, awidth, lineHeight);
  myVarAddress->setTextFilter(fAddr);
  wid.push_back(myVarAddress);
  myVarAddressVal = new EditTextWidget(myTab, font, myVarAddress->getRight() + 2, ypos - 1, rwidth, lineHeight, "56");
  myVarAddressVal->setEditable(false);

  myVarFormat = new CheckboxWidget(myTab, font, myVarAddressVal->getRight() + 16, ypos + 1, "BCD", kVarFormatChanged);
  wid.push_back(myVarFormat);

  myVarZeroBased = new CheckboxWidget(myTab, font, myVarFormat->getRight() + 16, ypos + 1, "0-based", kVar0BasedChanged);
  wid.push_back(myVarZeroBased);

  ypos += lineHeight + VGAP;

  items.clear();
  VarList::push_back(items, "1", "1");
  VarList::push_back(items, "2", "2");
  VarList::push_back(items, "3", "3");
  VarList::push_back(items, "4", "4");
  VarList::push_back(items, "5", "5");
  VarList::push_back(items, "6", "6");

  myScoresLabel = new StaticTextWidget(myTab, font, xpos, ypos + 1, lwidth, fontHeight, "Scores");

  xpos += 20; ypos += lineHeight + VGAP;

  myScoreDigitsLabel = new StaticTextWidget(myTab, font, xpos, ypos + 1, "Digits ");
  myScoreDigits = new PopUpWidget(myTab, font, myScoreDigitsLabel->getRight(), ypos, pwidth, lineHeight,
                                  items, "", 0, kScoreDigitsChanged);
  wid.push_back(myScoreDigits);

  items.clear();
  VarList::push_back(items, "1", "1");
  VarList::push_back(items, "10", "10");
  VarList::push_back(items, "100", "100");
  pwidth = font.getStringWidth("100");

  myScoreMultLabel = new StaticTextWidget(myTab, font, myScoreDigits->getRight() + 20, ypos + 1, "Multiplier ");
  myScoreMult = new PopUpWidget(myTab, font, myScoreMultLabel->getRight(), ypos, pwidth, lineHeight,
                                items, "", 0, kScoreMultChanged);
  wid.push_back(myScoreMult);

  myScoreFormat = new CheckboxWidget(myTab, font, myVarFormat->getLeft(), ypos + 1, "BCD", kScoreFormatChanged);
  wid.push_back(myScoreFormat);

  ypos += lineHeight + VGAP;

  myP1AddressLabel = new StaticTextWidget(myTab, font, xpos, ypos + 1, "P1 Addresses ");
  myP1Address0 = new EditTextWidget(myTab, font, myP1AddressLabel->getRight(), ypos - 1, awidth, lineHeight);
  myP1Address0->setTextFilter(fAddr);
  wid.push_back(myP1Address0);
  myP1Address0Val = new EditTextWidget(myTab, font, myP1Address0->getRight() + 2, ypos - 1, rwidth, lineHeight, "12");
  myP1Address0Val->setEditable(false);

  myP1Address1 = new EditTextWidget(myTab, font, myP1Address0Val->getRight() + 16, ypos - 1, awidth, lineHeight);
  myP1Address1->setTextFilter(fAddr);
  wid.push_back(myP1Address1);
  myP1Address1Val = new EditTextWidget(myTab, font, myP1Address1->getRight() + 2, ypos - 1, rwidth, lineHeight, "34");
  myP1Address1Val->setEditable(false);

  myP1Address2 = new EditTextWidget(myTab, font, myP1Address1Val->getRight() + 16, ypos - 1, awidth, lineHeight);
  myP1Address2->setTextFilter(fAddr);
  wid.push_back(myP1Address2);
  myP1Address2Val = new EditTextWidget(myTab, font, myP1Address2->getRight() + 2, ypos - 1, rwidth, lineHeight, "56");
  myP1Address2Val->setEditable(false);

  ypos += lineHeight + VGAP;

  myP2AddressLabel = new StaticTextWidget(myTab, font, xpos, ypos + 1, "P2 Addresses ");
  myP2Address0 = new EditTextWidget(myTab, font, myP2AddressLabel->getRight(), ypos - 1, awidth, lineHeight);
  myP2Address0->setTextFilter(fAddr);
  wid.push_back(myP2Address0);
  myP2Address0Val = new EditTextWidget(myTab, font, myP2Address0->getRight() + 2, ypos - 1, rwidth, lineHeight, "12");
  myP2Address0Val->setEditable(false);

  myP2Address1 = new EditTextWidget(myTab, font, myP2Address0Val->getRight() + 16, ypos - 1, awidth, lineHeight);
  myP2Address1->setTextFilter(fAddr);
  wid.push_back(myP2Address1);
  myP2Address1Val = new EditTextWidget(myTab, font, myP2Address1->getRight() + 2, ypos - 1, rwidth, lineHeight, "34");
  myP2Address1Val->setEditable(false);

  myP2Address2 = new EditTextWidget(myTab, font, myP2Address1Val->getRight() + 16, ypos - 1, awidth, lineHeight);
  myP2Address2->setTextFilter(fAddr);
  wid.push_back(myP2Address2);
  myP2Address2Val = new EditTextWidget(myTab, font, myP2Address2->getRight() + 2, ypos - 1, rwidth, lineHeight, "56");
  myP2Address2Val->setEditable(false);

  ypos += lineHeight + VGAP;

  myP3AddressLabel = new StaticTextWidget(myTab, font, xpos, ypos + 1, "P3 Addresses ");
  myP3Address0 = new EditTextWidget(myTab, font, myP3AddressLabel->getRight(), ypos - 1, awidth, lineHeight);
  myP3Address0->setTextFilter(fAddr);
  wid.push_back(myP3Address0);
  myP3Address0Val = new EditTextWidget(myTab, font, myP3Address0->getRight() + 2, ypos - 1, rwidth, lineHeight, "");
  myP3Address0Val->setEditable(false);

  myP3Address1 = new EditTextWidget(myTab, font, myP3Address0Val->getRight() + 16, ypos - 1, awidth, lineHeight);
  myP3Address1->setTextFilter(fAddr);
  wid.push_back(myP3Address1);
  myP3Address1Val = new EditTextWidget(myTab, font, myP3Address1->getRight() + 2, ypos - 1, rwidth, lineHeight, "");
  myP3Address1Val->setEditable(false);

  myP3Address2 = new EditTextWidget(myTab, font, myP3Address1Val->getRight() + 16, ypos - 1, awidth, lineHeight);
  myP3Address2->setTextFilter(fAddr);
  wid.push_back(myP3Address2);
  myP3Address2Val = new EditTextWidget(myTab, font, myP3Address2->getRight() + 2, ypos - 1, rwidth, lineHeight, "");
  myP3Address2Val->setEditable(false);

  ypos += lineHeight + VGAP;

  myP4AddressLabel = new StaticTextWidget(myTab, font, xpos, ypos + 1, "P4 Addresses ");
  myP4Address0 = new EditTextWidget(myTab, font, myP4AddressLabel->getRight(), ypos - 1, awidth, lineHeight);
  myP4Address0->setTextFilter(fAddr);
  wid.push_back(myP4Address0);
  myP4Address0Val = new EditTextWidget(myTab, font, myP4Address0->getRight() + 2, ypos - 1, rwidth, lineHeight, "");
  myP4Address0Val->setEditable(false);

  myP4Address1 = new EditTextWidget(myTab, font, myP4Address0Val->getRight() + 16, ypos - 1, awidth, lineHeight);
  myP4Address1->setTextFilter(fAddr);
  wid.push_back(myP4Address1);
  myP4Address1Val = new EditTextWidget(myTab, font, myP4Address1->getRight() + 2, ypos - 1, rwidth, lineHeight, "");
  myP4Address1Val->setEditable(false);

  myP4Address2 = new EditTextWidget(myTab, font, myP4Address1Val->getRight() + 16, ypos - 1, awidth, lineHeight);
  myP4Address2->setTextFilter(fAddr);
  wid.push_back(myP4Address2);
  myP4Address2Val = new EditTextWidget(myTab, font, myP4Address2->getRight() + 2, ypos - 1, rwidth, lineHeight, "");
  myP4Address2Val->setEditable(false);


  /*ypos += lineHeight + VGAP;
  fwidth = _w - lwidth - HBORDER * 2 - 22;
  new StaticTextWidget(myTab, font, xpos, ypos + 1, lwidth, fontHeight, "Formats");
  myFormats = new EditTextWidget(myTab, font, xpos + lwidth, ypos - 1,
                                    fwidth, lineHeight);
  wid.push_back(myVariations);

  ypos += lineHeight + VGAP;
  new StaticTextWidget(myTab, font, xpos, ypos + 1, lwidth, fontHeight, "Addresses");
  myAddresses = new EditTextWidget(myTab, font, xpos + lwidth, ypos - 1,
                                    fwidth, lineHeight);
  wid.push_back(myVariations);*/


  // Add items for tab 3
  addToFocusList(wid, myTab, tabID);

  //////////////////////////////////////////////////////////////////////////////
  // Activate the first tab
  myTab->setActiveTab(0);

  // Add Defaults, OK and Cancel buttons
  wid.clear();
  addDefaultsOKCancelBGroup(wid, font);
  addBGroupToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::loadConfig()
{
  if(instance().hasConsole())
  {
    myGameProperties = instance().console().properties();
  }
  else
  {
    const string& md5 = instance().launcher().selectedRomMD5();
    instance().propSet().getMD5(md5, myGameProperties);
  }

  loadEmulationProperties(myGameProperties);
  loadConsoleProperties(myGameProperties);
  loadControllerProperties(myGameProperties);
  loadCartridgeProperties(myGameProperties);
  loadHighScoresProperties(myGameProperties);

  myTab->loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::loadEmulationProperties(const Properties& props)
{
  string bsDetected = "";

  myBSType->setSelected(props.get(PropType::Cart_Type), "AUTO");
  if(myBSType->getSelectedTag().toString() == "AUTO")
  {
    if(instance().hasConsole())
    {
      string bs = instance().console().about().BankSwitch;
      size_t pos = bs.find_first_of('*');
      // remove '*':
      if(pos != string::npos)
        bs = bs.substr(0, pos) + bs.substr(pos + 1);
      bsDetected = bs + "detected";
    }
    else
    {
      const FilesystemNode& node = FilesystemNode(instance().launcher().selectedRom());
      ByteBuffer image;
      string md5 = props.get(PropType::Cart_MD5);
      size_t size = 0;

      // try to load the image for auto detection
      if(!instance().hasConsole() &&
        node.exists() && !node.isDirectory() && (image = instance().openROM(node, md5, size)) != nullptr)
      {
        bsDetected = Bankswitch::typeToDesc(CartDetector::autodetectType(image, size)) + " detected";
      }
    }
  }
  myTypeDetected->setLabel(bsDetected);

  // Start bank
  VariantList items;

  VarList::push_back(items, "Auto", "AUTO");
  if(instance().hasConsole())
  {
    uInt16 numBanks = instance().console().cartridge().bankCount();

    for(uInt16 i = 0; i < numBanks; ++i)
      VarList::push_back(items, i, i);
    myStartBank->setEnabled(true);
  }
  else
  {
    const string& startBank = props.get(PropType::Cart_StartBank);

    VarList::push_back(items, startBank, startBank);
    myStartBank->setEnabled(false);
  }
  myStartBank->addItems(items);
  myStartBank->setSelected(props.get(PropType::Cart_StartBank), "AUTO");

  myFormat->setSelected(props.get(PropType::Display_Format), "AUTO");
  if(instance().hasConsole() && myFormat->getSelectedTag().toString() == "AUTO")
  {
    const string& format = instance().console().about().DisplayFormat;
    string label;
    if (format.at(format.length() - 1) == '*')
      label = format.substr(0, format.length() - 1);
    else
      label = format;
    myFormatDetected->setLabel(label + " detected");
  }
  else
    myFormatDetected->setLabel("");

  // if phosphor is always enabled, disable game specific phosphor settings
  bool alwaysPhosphor = instance().settings().getString("tv.phosphor") == "always";
  bool usePhosphor = props.get(PropType::Display_Phosphor) == "YES";
  myPhosphor->setState(usePhosphor);
  myPhosphor->setEnabled(!alwaysPhosphor);
  if (alwaysPhosphor)
    myPhosphor->setLabel("Phosphor (enabled for all ROMs)");
  else
    myPhosphor->setLabel("Phosphor");
  myPPBlend->setEnabled(!alwaysPhosphor && usePhosphor);

  const string& blend = props.get(PropType::Display_PPBlend);
  myPPBlend->setValue(BSPF::stringToInt(blend));

  // set vertical center
  Int32 vcenter = BSPF::stringToInt(props.get(PropType::Display_VCenter));
  myVCenter->setValueLabel(vcenter);
  myVCenter->setValue(vcenter);
  myVCenter->setValueUnit(vcenter ? "px" : "");

  mySound->setState(props.get(PropType::Cart_Sound) == "STEREO");
  // if stereo is always enabled, disable game specific stereo setting
  mySound->setEnabled(!instance().audioSettings().stereo());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::loadConsoleProperties(const Properties& props)
{
  myLeftDiffGroup->setSelected(props.get(PropType::Console_LeftDiff) == "A" ? 0 : 1);
  myRightDiffGroup->setSelected(props.get(PropType::Console_RightDiff) == "A" ? 0 : 1);
  myTVTypeGroup->setSelected(props.get(PropType::Console_TVType) == "BW" ? 1 : 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::loadControllerProperties(const Properties& props)
{
  string controller = props.get(PropType::Controller_Left);
  myLeftPort->setSelected(controller, "AUTO");
  controller = props.get(PropType::Controller_Right);
  myRightPort->setSelected(controller, "AUTO");

  mySwapPorts->setState(props.get(PropType::Console_SwapPorts) == "YES");
  mySwapPaddles->setState(props.get(PropType::Controller_SwapPaddles) == "YES");

  // MouseAxis property (potentially contains 'range' information)
  istringstream m_axis(props.get(PropType::Controller_MouseAxis));
  string m_control, m_range;
  m_axis >> m_control;
  bool autoAxis = BSPF::equalsIgnoreCase(m_control, "AUTO");
  myMouseControl->setState(!autoAxis);
  if(autoAxis)
  {
    myMouseX->setSelectedIndex(0);
    myMouseY->setSelectedIndex(0);
  }
  else
  {
    myMouseX->setSelected(m_control[0] - '0');
    myMouseY->setSelected(m_control[1] - '0');
  }
  myMouseX->setEnabled(!autoAxis);
  myMouseY->setEnabled(!autoAxis);
  if(m_axis >> m_range)
  {
    myMouseRange->setValue(BSPF::stringToInt(m_range));
  }
  else
  {
    myMouseRange->setValue(100);
  }

  updateControllerStates();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::loadCartridgeProperties(const Properties& props)
{
  myName->setText(props.get(PropType::Cart_Name));
  myMD5->setText(props.get(PropType::Cart_MD5));
  myManufacturer->setText(props.get(PropType::Cart_Manufacturer));
  myModelNo->setText(props.get(PropType::Cart_ModelNo));
  myRarity->setText(props.get(PropType::Cart_Rarity));
  myNote->setText(props.get(PropType::Cart_Note));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::loadHighScoresProperties(const Properties& props)
{
  //bool enable = instance().hasConsole() && instance().console().par

  myHighScores->setState(true);
  myPlayers->setSelected(props.get(PropType::Cart_Players));
  myVariations->setText(props.get(PropType::Cart_Variations));



  handleHighScoresWidgets();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::saveConfig()
{
  // Emulation properties
  myGameProperties.set(PropType::Cart_Type, myBSType->getSelectedTag().toString());
  myGameProperties.set(PropType::Cart_StartBank, myStartBank->getSelectedTag().toString());
  myGameProperties.set(PropType::Display_Format, myFormat->getSelectedTag().toString());
  myGameProperties.set(PropType::Display_Phosphor, myPhosphor->getState() ? "YES" : "NO");
  myGameProperties.set(PropType::Display_PPBlend, myPPBlend->getValueLabel() == "Off" ? "0" :
                       myPPBlend->getValueLabel());
  Int32 vcenter = myVCenter->getValue();

  myGameProperties.set(PropType::Display_VCenter, std::to_string(vcenter));
  myGameProperties.set(PropType::Cart_Sound, mySound->getState() ? "STEREO" : "MONO");

  // Console properties
  myGameProperties.set(PropType::Console_TVType, myTVTypeGroup->getSelected() ? "BW" : "COLOR");
  myGameProperties.set(PropType::Console_LeftDiff, myLeftDiffGroup->getSelected() ? "B" : "A");
  myGameProperties.set(PropType::Console_RightDiff, myRightDiffGroup->getSelected() ? "B" : "A");

  // Controller properties
  myGameProperties.set(PropType::Controller_Left, myLeftPort->getSelectedTag().toString());
  myGameProperties.set(PropType::Controller_Right, myRightPort->getSelectedTag().toString());
  myGameProperties.set(PropType::Console_SwapPorts, (mySwapPorts->isEnabled() && mySwapPorts->getState()) ? "YES" : "NO");
  myGameProperties.set(PropType::Controller_SwapPaddles, (/*mySwapPaddles->isEnabled() &&*/ mySwapPaddles->getState()) ? "YES" : "NO");

  // MouseAxis property (potentially contains 'range' information)
  string mcontrol = "AUTO";
  if(myMouseControl->getState())
    mcontrol = myMouseX->getSelectedTag().toString() +
               myMouseY->getSelectedTag().toString();
  string range = myMouseRange->getValueLabel();
  if(range != "100")
    mcontrol += " " + range;
  myGameProperties.set(PropType::Controller_MouseAxis, mcontrol);

  // Cartridge properties
  myGameProperties.set(PropType::Cart_Name, myName->getText());
  myGameProperties.set(PropType::Cart_Manufacturer, myManufacturer->getText());
  myGameProperties.set(PropType::Cart_ModelNo, myModelNo->getText());
  myGameProperties.set(PropType::Cart_Rarity, myRarity->getText());
  myGameProperties.set(PropType::Cart_Note, myNote->getText());

  // Always insert; if the properties are already present, nothing will happen
  instance().propSet().insert(myGameProperties);
  instance().saveConfig();

  // In any event, inform the Console
  if(instance().hasConsole())
  {
    instance().console().setProperties(myGameProperties);

    // update 'Emulation' tab settings immediately
    instance().console().setFormat(myFormat->getSelected());
    instance().frameBuffer().tiaSurface().enablePhosphor(myPhosphor->getState(), myPPBlend->getValue());
    instance().console().updateVcenter(vcenter);
    instance().console().initializeAudio();

    // update 'Console' tab settings immediately
    instance().console().switches().setTvColor(myTVTypeGroup->getSelected() == 0);
    instance().console().switches().setLeftDifficultyA(myLeftDiffGroup->getSelected() == 0);
    instance().console().switches().setRightDifficultyA(myRightDiffGroup->getSelected() == 0);

    // update 'Controllers' tab settings immediately
    instance().console().setControllers(myGameProperties.get(PropType::Cart_MD5));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::setDefaults()
{
  // Load the default properties
  Properties defaultProperties;
  const string& md5 = myGameProperties.get(PropType::Cart_MD5);

  instance().propSet().getMD5(md5, defaultProperties, true);

  switch(myTab->getActiveTab())
  {
    case 0: // Emulation properties
      loadEmulationProperties(defaultProperties);
      break;

    case 1: // Console properties
      loadConsoleProperties(defaultProperties);
      break;

    case 2: // Controller properties
      loadControllerProperties(defaultProperties);
      break;

    case 3: // Cartridge properties
      loadCartridgeProperties(defaultProperties);
      break;

    case 4: // High Scores properties
      loadHighScoresProperties(defaultProperties);
      break;

    default: // make the compiler happy
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::updateControllerStates()
{
  bool swapPorts = mySwapPorts->getState();
  bool autoDetect = false;
  ByteBuffer image;
  string md5 = myGameProperties.get(PropType::Cart_MD5);
  size_t size = 0;

  // try to load the image for auto detection
  if(!instance().hasConsole())
  {
    const FilesystemNode& node = FilesystemNode(instance().launcher().selectedRom());

    autoDetect = node.exists() && !node.isDirectory() && (image = instance().openROM(node, md5, size)) != nullptr;
  }
  string label = "";
  Controller::Type type = Controller::getType(myLeftPort->getSelectedTag().toString());

  if(type == Controller::Type::Unknown)
  {
    if(instance().hasConsole())
      label = (!swapPorts ? instance().console().leftController().name()
               : instance().console().rightController().name()) + " detected";
    else if(autoDetect)
      label = ControllerDetector::detectName(image.get(), size, type,
                                             !swapPorts ? Controller::Jack::Left : Controller::Jack::Right,
                                             instance().settings()) + " detected";
  }
  myLeftPortDetected->setLabel(label);

  label = "";
  type = Controller::getType(myRightPort->getSelectedTag().toString());

  if(type == Controller::Type::Unknown)
  {
    if(instance().hasConsole())
      label = (!swapPorts ? instance().console().rightController().name()
               : instance().console().leftController().name()) + " detected";
    else if(autoDetect)
      label = ControllerDetector::detectName(image.get(), size, type,
                                             !swapPorts ? Controller::Jack::Right : Controller::Jack::Left,
                                             instance().settings()) + " detected";
  }
  myRightPortDetected->setLabel(label);

  const string& contrLeft = myLeftPort->getSelectedTag().toString();
  const string& contrRight = myRightPort->getSelectedTag().toString();
  bool enableEEEraseButton = false;

  // Compumate bankswitching scheme doesn't allow to select controllers
  bool enableSelectControl = myBSType->getSelectedTag() != "CM";
  // Enable Swap Paddles checkbox only for paddle games
  bool enableSwapPaddles = BSPF::startsWithIgnoreCase(contrLeft, "PADDLES") ||
    BSPF::startsWithIgnoreCase(contrRight, "PADDLES") ||
    BSPF::startsWithIgnoreCase(myLeftPortDetected->getLabel(), "Paddles") ||
    BSPF::startsWithIgnoreCase(myRightPortDetected->getLabel(), "Paddles");

  if(instance().hasConsole())
  {
    const Controller& lport = instance().console().leftController();
    const Controller& rport = instance().console().rightController();

    // we only enable the button if we have a valid previous and new controller.
    bool enableBtnForLeft =
      (contrLeft == "AUTO" || contrLeft == "SAVEKEY" || contrLeft == "ATARIVOX") &&
      (lport.type() == Controller::Type::SaveKey || lport.type() == Controller::Type::AtariVox);
    bool enableBtnForRight =
      (contrRight == "AUTO" || contrRight == "SAVEKEY" || contrRight == "ATARIVOX") &&
      (rport.type() == Controller::Type::SaveKey || rport.type() == Controller::Type::AtariVox);
    enableEEEraseButton = enableBtnForLeft || enableBtnForRight;
  }

  myLeftPortLabel->setEnabled(enableSelectControl);
  myRightPortLabel->setEnabled(enableSelectControl);
  myLeftPort->setEnabled(enableSelectControl);
  myRightPort->setEnabled(enableSelectControl);

  mySwapPorts->setEnabled(enableSelectControl);
  mySwapPaddles->setEnabled(enableSwapPaddles);

  myEraseEEPROMLabel->setEnabled(enableEEEraseButton);
  myEraseEEPROMButton->setEnabled(enableEEEraseButton);
  myEraseEEPROMInfo->setEnabled(enableEEEraseButton);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::eraseEEPROM()
{
  Controller& lport = instance().console().leftController();
  Controller& rport = instance().console().rightController();

  if(lport.type() == Controller::Type::SaveKey || lport.type() == Controller::Type::AtariVox)
  {
    SaveKey& skey = static_cast<SaveKey&>(lport);
    skey.eraseCurrent();
  }

  if(rport.type() == Controller::Type::SaveKey || rport.type() == Controller::Type::AtariVox)
  {
    SaveKey& skey = static_cast<SaveKey&>(rport);
    skey.eraseCurrent();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::handleHighScoresWidgets()
{
  bool enable = myHighScores->getState();
  int players = myPlayers->getSelected() + 1;
  bool enablePlayers = enable && players > 1;
  bool enableVars = enable && myVariations->getText() > "1";
  int numAddr = (myScoreDigits->getSelected() - myScoreMult->getSelected() + 2) / 2;
  bool enable1 = enable && numAddr >= 2;
  bool enable2 = enable && numAddr >= 3;

  myPlayersLabel->setEnabled(enable);
  myPlayers->setEnabled(enable);
  myPlayersAddressLabel->setEnabled(enablePlayers);
  myPlayersAddress->setEnabled(enablePlayers);
  myPlayersAddress->setEditable(enablePlayers);
  myPlayersAddressVal->setEnabled(enablePlayers);

  myVariationsLabel->setEnabled(enable);
  myVariations->setEnabled(enable);
  myVariations->setEditable(enable);
  myVarAddressLabel->setEnabled(enableVars);
  myVarAddress->setEnabled(enableVars);
  myVarAddress->setEditable(enableVars);
  myVarAddressVal->setEnabled(enableVars);
  myVarFormat->setEnabled(enableVars);
  myVarZeroBased->setEnabled(enableVars);

  myScoresLabel->setEnabled(enable);
  myScoreDigitsLabel->setEnabled(enable);
  myScoreDigits->setEnabled(enable);
  myScoreFormat->setEnabled(enable);
  myScoreMultLabel->setEnabled(enable);
  myScoreMult->setEnabled(enable);

  myP1AddressLabel->setEnabled(enable);
  myP1Address0->setEnabled(enable);
  myP1Address0->setEditable(enable);
  myP1Address0Val->setEnabled(enable);
  myP1Address1->setEnabled(enable1);
  myP1Address1->setEditable(enable1);
  myP1Address1Val->setEnabled(enable1);
  myP1Address2->setEnabled(enable2);
  myP1Address2->setEditable(enable2);
  myP1Address2Val->setEnabled(enable2);

  enable &= players >= 2;
  enable1 &= enable; enable2 &= enable;
  myP2AddressLabel->setEnabled(enable);
  myP2Address0->setEnabled(enable);
  myP2Address0->setEditable(enable);
  myP2Address0Val->setEnabled(enable);
  myP2Address1->setEnabled(enable1);
  myP2Address1->setEditable(enable1);
  myP2Address1Val->setEnabled(enable1);
  myP2Address2->setEnabled(enable2);
  myP2Address2->setEditable(enable2);
  myP2Address2Val->setEnabled(enable2);

  enable &= players >= 3;
  enable1 &= enable; enable2 &= enable;
  myP3AddressLabel->setEnabled(enable);
  myP3Address0->setEnabled(enable);
  myP3Address0->setEditable(enable);
  myP3Address0Val->setEnabled(enable);
  myP3Address1->setEnabled(enable1);
  myP3Address1->setEditable(enable1);
  myP3Address1Val->setEnabled(enable1);
  myP3Address2->setEnabled(enable2);
  myP3Address2->setEditable(enable2);
  myP3Address2Val->setEnabled(enable2);

  enable &= players >= 4;
  enable1 &= enable; enable2 &= enable;
  myP4AddressLabel->setEnabled(enable);
  myP4Address0->setEnabled(enable);
  myP4Address0->setEditable(enable);
  myP4Address0Val->setEnabled(enable);
  myP4Address1->setEnabled(enable1);
  myP4Address1->setEditable(enable1);
  myP4Address1->setEditable(enable1);
  myP4Address1Val->setEnabled(enable1);
  myP4Address2->setEnabled(enable2);
  myP4Address2->setEditable(enable2);
  myP4Address2Val->setEnabled(enable2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::handleCommand(CommandSender* sender, int cmd,
                                   int data, int id)
{
  switch (cmd)
  {
    case GuiObject::kOKCmd:
      saveConfig();
      close();
      break;

    case GuiObject::kDefaultsCmd:
      setDefaults();
      break;

    case TabWidget::kTabChangedCmd:
      if(data == 2)  // 'Controllers' tab selected
        updateControllerStates();

      // The underlying dialog still needs access to this command
      Dialog::handleCommand(sender, cmd, data, 0);
      break;

    case kLeftCChanged:
    case kRightCChanged:
      updateControllerStates();
      break;

    case kEEButtonPressed:
      eraseEEPROM();
      break;

    case kPhosphorChanged:
    {
      bool status = myPhosphor->getState();
      myPPBlend->setEnabled(status);
      break;
    }

    case kPPBlendChanged:
      if(myPPBlend->getValue() == 0)
      {
        myPPBlend->setValueLabel("Off");
        myPPBlend->setValueUnit("");
      }
      else
        myPPBlend->setValueUnit("%");
      break;

    case kVCenterChanged:
      if (myVCenter->getValue() == 0)
      {
        myVCenter->setValueLabel("Default");
        myVCenter->setValueUnit("");
      }
      else
        myVCenter->setValueUnit("px");
      break;

    case kMCtrlChanged:
    {
      bool state = myMouseControl->getState();
      myMouseX->setEnabled(state);
      myMouseY->setEnabled(state);
      break;
    }

    case kHiScoresChanged:
    case kPlayersChanged:
    case kScoreDigitsChanged:
    case kScoreMultChanged:
      handleHighScoresWidgets();
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}
