/* Arduino SdFat Library
 * Copyright (C) 2009 by William Greiman
 *
 * This file is part of the Arduino SdFat Library
 *
 * This Library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the Arduino SdFat Library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <float.h>
#include <istream.h>
#include <ctype.h>
//------------------------------------------------------------------------------
/**
 * Extract a character if one is available.
 *
 * \return The character or -1 if a failure occurs.  A failure is indicated
 * by the stream state.
 */
int istream::get() {
  int16_t c;
  gcount_ = 0;
  c = getch();
  if (c < 0) {
    setstate(failbit);
  } else {
    gcount_ = 1;
  }
  return c;
}
//------------------------------------------------------------------------------
/**
 * Extract a character if one is available.
 *
 * \param[out] c location to receive the extracted character.
 *
 * \return always returns *this. A failure is indicated by the stream state.
 */
istream& istream::get(char& c) {
  int16_t tmp = get();
  if (tmp >= 0) c = tmp;
  return *this;
}
//------------------------------------------------------------------------------
/**
 * Extract characters. 
 *
 * \param[out] str Location to receive extracted characters.
 * \param[in] n Size of str.
 * \param[in] delim Delimiter
 *
 * Characters are extracted until extraction fails, n is less than 1,
 * n-1 characters are extracted, or the next character equals
 * \a delim (delim is not extracted). If no characters are extracted
 * failbit is set.  If end-of-file occurs the eofbit is set.
 *
 * \return always returns *this. A failure is indicated by the stream state.
 */
istream& istream::get(char *str, streamsize n, char delim) {
  int c;
  fpos_t pos;
  gcount_ = 0;
  while ((gcount_ + 1)  < n) {
    c = getch(&pos);
    if (c < 0) {
      break;
    }
    if (c == delim) {
      setpos(&pos);
      break;
    }
    str[gcount_++] = c;
  }
  if (n > 0) str[gcount_] = '\0';
  if (gcount_ == 0) setstate(failbit);
  return *this;
}
//------------------------------------------------------------------------------
void istream::getBool(bool *b) {
  if ((flags() & boolalpha) == 0) {
    getNumber(b);
    return;
  }
  PGM_P truePtr = PSTR("true");
  const uint8_t true_len = 4;
  PGM_P falsePtr = PSTR("false");
  const uint8_t false_len = 5;
  bool trueOk = true;
  bool falseOk = true;
  uint8_t i = 0;
  int c = readSkip();
  while (1) {
//    if (c < 0) break;  // not required
    falseOk = falseOk && c == pgm_read_byte(falsePtr + i);
    trueOk = trueOk && c == pgm_read_byte(truePtr + i);
    if (trueOk == false && falseOk == false) break;
    i++;
    if (trueOk && i == true_len) {
      *b = true;
      return;
    }
    if (falseOk && i == false_len) {
      *b = false;
      return;
    }
    c = getch();
  }
  setstate(failbit);
}
//------------------------------------------------------------------------------
void istream::getChar(char* ch) {
  int16_t c = readSkip();
  if (c < 0) {
    setstate(failbit);
  } else {
    *ch = c;
  }
}
//------------------------------------------------------------------------------
//
// http://www.exploringbinary.com/category/numbers-in-computers/
//
int16_t const EXP_LIMIT = 100;
static const uint32_t uint32_max = (uint32_t)-1;
bool istream::getFloat(float* value) {
  bool got_digit = false;
  bool got_dot = false;
  bool neg;
  int16_t c;
  bool expNeg = false;
  int16_t exp = 0;
  int16_t fracExp = 0;
  uint32_t frac = 0;
  fpos_t endPos;
  float pow10;
  float v;

  getpos(&endPos);
  c = readSkip();
  neg = c == '-';
  if (c == '-' || c == '+') {
    c = getch();
  }
  while (1) {
    if (isdigit(c)) {
      got_digit = true;
      if (frac < uint32_max/10) {
        frac = frac * 10 + (c  - '0');
        if (got_dot) fracExp--;
      } else {
        if (!got_dot) fracExp++;
      }
    } else if (!got_dot && c == '.') {
      got_dot = true;
    } else {
      break;
    }
    if (fracExp < -EXP_LIMIT || fracExp > EXP_LIMIT) goto fail;
    c = getch(&endPos);
  }
  if (!got_digit) goto fail;
  if (c == 'e' || c == 'E') {
    c = getch();
    expNeg = c == '-';
    if (c == '-' || c == '+') {
      c = getch();
    }
    while (isdigit(c)) {
      if (exp > EXP_LIMIT) goto fail;
      exp = exp * 10 + (c - '0');
      c = getch(&endPos);
    }
  }
  v = static_cast<float>(frac);
  exp = expNeg ? fracExp - exp : fracExp + exp;
  expNeg = exp < 0;
  if (expNeg) exp = -exp;
  pow10 = 10.0;
  while (exp) {
    if (exp & 1) {
      if (expNeg) {
        // check for underflow
        if (v < FLT_MIN * pow10  && frac != 0) goto fail;
        v /= pow10;
      } else {
        // check for overflow
        if (v > FLT_MAX / pow10) goto fail;
        v *= pow10;
      }
    }
    pow10 *= pow10;
    exp >>= 1;
  }
  setpos(&endPos);
  *value = neg ? -v : v;
  return true;

 fail:
  // error restore position to last good place
  setpos(&endPos);
  setstate(failbit);
  return false;
}
//------------------------------------------------------------------------------
/**
 * Extract characters
 *
 * \param[out] str Location to receive extracted characters.
 * \param[in] n Size of str.
 * \param[in] delim Delimiter
 *
 * Characters are extracted until extraction fails,
 * the next character equals \a delim (delim is extracted), or n-1
 * characters are extracted.
 *
 * The failbit is set if no characters are extracted or n-1 characters
 * are extracted.  If end-of-file occurs the eofbit is set.
 *
 * \return always returns *this. A failure is indicated by the stream state.
 */
istream& istream::getline(char *str, streamsize n, char delim) {
  fpos_t pos;
  int c;
  gcount_ = 0;
  if (n > 0) str[0] = '\0';
  while (1) {
    c = getch(&pos);
    if (c < 0) {
      break;
    }
    if (c == delim) {
      gcount_++;
      break;
    }
    if ((gcount_ + 1)  >=  n) {
      setpos(&pos);
      setstate(failbit);
      break;
    }
    str[gcount_++] = c;
    str[gcount_] = '\0';
  }
  if (gcount_ == 0) setstate(failbit);
  return *this;
}
//------------------------------------------------------------------------------
bool istream::getNumber(uint32_t posMax, uint32_t negMax, uint32_t* num) {
  int16_t c;
  int8_t any = 0;
  int8_t have_zero = 0;
  uint8_t neg;
  uint32_t val = 0;
  uint32_t cutoff;
  uint8_t cutlim;
  fpos_t endPos;
  uint8_t f = flags() & basefield;
  uint8_t base = f == oct ? 8 : f != hex ? 10 : 16;
  getpos(&endPos);
  c = readSkip();

  neg = c == '-' ? 1 : 0;
  if (c == '-' || c == '+') {
    c = getch();
  }

  if (base == 16 && c == '0') {  // TESTSUITE
    c = getch(&endPos);
    if (c == 'X' || c == 'x') {
      c = getch();
      // remember zero in case no hex digits follow x/X
      have_zero = 1;
    } else {
      any = 1;
    }
  }
  // set values for overflow test
  cutoff = neg ? negMax : posMax;
  cutlim = cutoff % base;
  cutoff /= base;

  while (1) {
    if (isdigit(c)) {
      c -= '0';
    } else if (isalpha(c)) {
      c -= isupper(c) ? 'A' - 10 : 'a' - 10;
    } else {
      break;
    }
    if (c >= base) {
      break;
    }
    if (val > cutoff || (val == cutoff && c > cutlim)) {
      // indicate overflow error
      any = -1;
      break;
    }
    val = val * base + c;
    c = getch(&endPos);
    any = 1;
  }
  setpos(&endPos);
  if (any > 0 || (have_zero && any >= 0)) {
    *num =  neg ? -val : val;
    return true;
  }
  setstate(failbit);
  return false;
}
//------------------------------------------------------------------------------
/**
 *
 */
void istream::getStr(char *str) {
  fpos_t pos;
  uint16_t i = 0;
  uint16_t m = width() ? width() - 1 : 0XFFFE;
  if (m != 0) {
    getpos(&pos);
    int c = readSkip();

    while (i < m) {
      if (c < 0) {
        break;
      }
      if (isspace(c)) {
        setpos(&pos);
        break;
      }
      str[i++] = c;
      c = getch(&pos);
    }
  }
  str[i] = '\0';
  if (i == 0) setstate(failbit);
  width(0);
}
//------------------------------------------------------------------------------
/**
 * Extract characters and discard them.
 *
 * \param[in] n maximum number of characters to ignore.
 * \param[in] delim Delimiter.
 *
 * Characters are extracted until extraction fails, \a n characters
 * are extracted, or the next input character equals \a delim
 * (the delimiter is extracted).  If end-of-file occurs the eofbit is set.
 *
 * Failures are indicated by the state of the stream.
 *
 * \return *this
 *
 */
istream& istream::ignore(streamsize n, int delim) {
  int c;
  gcount_ = 0;
  while (gcount_ < n) {
    c = getch();
    if (c < 0) {
      break;
    }
    gcount_++;
    if (c == delim) break;
  }
  return *this;
}
//------------------------------------------------------------------------------
/**
 * Return the next available character without consuming it.
 *
 * \return The character if the stream state is good else -1;
 *
 */
int istream::peek() {
  int16_t c;
  fpos_t pos;
  gcount_ = 0;
  getpos(&pos);
  c = getch();
  if (c < 0) {
    if (!bad()) setstate(eofbit);
  } else {
    setpos(&pos);
  }
  return c;
}
//------------------------------------------------------------------------------
int16_t istream::readSkip() {
  int16_t c;
  do {
    c = getch();
  } while (isspace(c) && (flags() & skipws));
  return c;
}
//------------------------------------------------------------------------------
/** used to implement ws() */
void istream::skipWhite() {
  int c;
  fpos_t pos;
  do {
    c = getch(&pos);
  } while (isspace(c));
  setpos(&pos);
}
