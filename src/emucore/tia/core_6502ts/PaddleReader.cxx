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
// Copyright (c) 1995-2016 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include <cmath>

#include "PaddleReader.hxx"


static double expApprox(double x)
{
  double  x2 = x  * x / 2,
          x3 = x2 * x / 3,
          x4 = x3 * x / 4;

  return 1 + x + x2 + x3 + x4;
}

static constexpr double
  C = 68e-9,
  RPOT = 1e6,
  R0 = 1.8e3,
  USUPP = 5;

static constexpr double TRIPPOINT_LINES = 380;

namespace TIA6502tsCore {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PaddleReader::PaddleReader()
{
  reset(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PaddleReader::reset(double timestamp)
{
  myU = 0;
  myIsDumped = false;

  myValue = 0;
  myTimestamp = timestamp;

  setTvMode(TvMode::ntsc);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PaddleReader::vblank(uInt8 value, double timestamp)
{
  bool oldIsDumped = myIsDumped;

  if (value & 0x80) {
    myIsDumped = true;
    myU = 0;
    myTimestamp = timestamp;
  } else if (oldIsDumped) {
    myIsDumped = false;
    myTimestamp = timestamp;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 PaddleReader::inpt(double timestamp)
{
  updateCharge(timestamp);

  bool state = myIsDumped ? false : myU > myUThresh;

  return state ? 0x80 : 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PaddleReader::update(double value, double timestamp, TvMode tvMode)
{
  if (tvMode != myTvMode) {
    setTvMode(tvMode);
  }

  if (value != myValue) {
    myValue = value;
    updateCharge(timestamp);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PaddleReader::setTvMode(TvMode mode)
{
    myTvMode = mode;

    myClockFreq = myTvMode == TvMode::ntsc ? 60 * 228 * 262 : 50 * 228 * 312;
    myUThresh = USUPP * (1. - exp(-TRIPPOINT_LINES * 228 / myClockFreq  / (RPOT + R0) / C));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PaddleReader::updateCharge(double timestamp)
{
  if (myIsDumped) return;

  myU = USUPP * (1 - (1 - myU / USUPP) *
    expApprox(-(timestamp - myTimestamp) / (myValue * RPOT + R0) / C / myClockFreq));

  myTimestamp = timestamp;
}

} // namespace TIA6502tsCore