/**
 * Copyright (C) 2024 Yahya Al-Shamali
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
/**
 * \file Lexer.cc
 */
#include "Lexer.h"
#include <iostream>

bool isSpace(char c) noexcept {
  switch (c) {
  case ' ':
  case '\t':
  case '\r':
  case '\n':
    return true;
  default:
    return false;
  }
}

bool isScale(char c) noexcept {
  switch (c) {
  case '(':
  case ')':
  case '{':
  case '}':
    return true;
  default:
    return false;
  }
}

// moves the current pointer to the next space character
void fysh::FyshLexer::gotoEndOfToken() noexcept {
  while (!isSpace(peek())) {
    get();
  }
}


bool fysh::FyshLexer::isFyshEye(char c) noexcept {
  switch (c) {
  case 'o':
    // case '°': UNICODE
    return true;
  default:
    return false;
  }
}

fysh::Fysh fysh::FyshLexer::tryUnicode(const char *bytes, Species s) noexcept {
  bool touched{false};
  for (size_t i = 0; (unsigned char)bytes[i] != 0x00; i++) {
    if ((unsigned char)peek() == (unsigned char)bytes[i]) {
      get();
      touched = true;
    } else {
      if (touched) {
        return Fysh{Species::INVALID};
      } else {
        // Contiue looping through the characters
        return Fysh{Species::CONTINUE};
      }
    }
  }
  return Fysh{s};
}

struct Unicode {
  const char *codePoint;
  fysh::Species species;
};

/**
 * Try to match the token to Unicode characters.
 *
 * This assumes that none of these characters start with the same byte, or
 * else trying to check the next character will not work anymore because it
 * will already be marked as invalid.
 * Example:
 * ♡  - U+2661 - 0xE2 0x99 0xA1
 * ♥  - U+2665 - 0xE2 0x99 0xA5
 * These two start with the same bytes 0xE2 and 0x99.
 * if we add ♥ to the list of valid tokens, it will never be checked because
 * the lexer will match the first two bytes of ♡, see that 0xA5 != 0xA1,
 * and return an invalid token (instead of continuing to check).
 * Maybe we can implement this with a Trie, but it doesn't seem worth it
 * for only two tokens.
 */
fysh::Fysh fysh::FyshLexer::unicode() noexcept {
  static const struct Unicode chars[] = {
      {"💔", Species::HEART_DIVIDE},
      {"♡", Species::HEART_MULTIPLY},
  };
  for (const struct Unicode &c : chars) {
    Fysh fysh{tryUnicode(c.codePoint, c.species)};
    // We either get an invalid token or a non-continue token
    if (fysh == Species::INVALID || !(fysh == Species::CONTINUE)) {
      return fysh;
    }
  }
  std::cout << peek() << '(' << +peek() << ')' << std::endl;
  return Fysh(Species::INVALID);
}

fysh::Fysh fysh::FyshLexer::heart() noexcept {
  get();
  return fysh::Fysh(Species::HEART_MULTIPLY);
}

fysh::Fysh fysh::FyshLexer::heartBreak() noexcept {
  const char *start = current;
  get();
    if (peek() == '3') {
      get();
      return Fysh(Species::HEART_DIVIDE, start, current);
    }
    return Fysh(Species::INVALID);
}


// opening for error handling ><!@#$>
fysh::Fysh fysh::FyshLexer::openWTF() noexcept {
  const char *start = current;
  if (get() == '!' && get() == '@' && get() == '#' && get() == '$' &&
      get() == '>') {
    return Fysh(Species::WTF_OPEN, start, current);
  }
  return Fysh(Species::INVALID);
}

// closing for error handling <!@#$><
fysh::Fysh fysh::FyshLexer::closeWTF() noexcept {
  const char *start = current;
  if (get() == '!' && get() == '@' && get() == '#' && get() == '$' &&
      get() == '>' && get() == '<'){
    return Fysh(Species::WTF_CLOSE, start, current);
  }
  return Fysh(Species::INVALID);
}

fysh::Fysh fysh::FyshLexer::slashOrComment() noexcept {
  return Fysh(Species::INVALID);
}

fysh::Fysh fysh::FyshLexer::identifier() noexcept {
  return Fysh(Species::INVALID);
}

fysh::Fysh fysh::FyshLexer::nextFysh() noexcept {
  while (isSpace(peek()))
    get();
  switch (peek()) {
  case '\0':
    return Fysh(Species::END);
  case '<':
  case '>':
    return fyshOutline();
  default:
    return unicode();
  }
}
fysh::Fysh fysh::FyshLexer::fyshOpen() noexcept {
  get();
  return Fysh(Species::FYSH_OPEN);
}

fysh::Fysh fysh::FyshLexer::fyshClose() noexcept {
  get(); 
  if (peek() == '<') {
    return Fysh(Species::FYSH_CLOSE);
  }
  return Fysh(Species::INVALID);
}

fysh::Fysh fysh::FyshLexer::fyshOutline() noexcept {
  char c = get();

  switch (c) {
  case ('<'):
    switch (peek()) {
      case ('3'):
        return heart();
      case ('/'):
        return heartBreak();
      default:
        return swimLeft();
    }
  case '>':
    switch (peek()) {
      case ('<'):
        return swimRight(); 
      default:
        return Fysh(Species::INVALID);
    }
  default:
    return Fysh(Species::END);
  }
}

fysh::Fysh fysh::FyshLexer::swimLeft() noexcept {
  // current is the 2nd character of the token
  switch (peek()) {
    case ('{'):
    case ('('):
    case ('}'):
    case (')'):
    case ('o'):
      return scales(false); // false for negative
    case ('>'):
      return fyshClose();
    case ('!'):
      return closeWTF();
  }
  return Fysh(Species::INVALID);
}

// all fysh that start with >< are swimming right e.g. ><>
fysh::Fysh fysh::FyshLexer::swimRight() noexcept {
  get(); // 2nd swim right character '<'
  switch (peek()) {
    case ('{'):
    case ('('):
    case ('}'):
    case (')'):
      return scales(true); // true for positive
    case ('>'):
      return fyshOpen();
    case ('!'):
      return openWTF();
    case ('/'):
      return slashOrComment();
    case ('#'):
      return random();
  }
  return Fysh(Species::INVALID);
}

fysh::Fysh fysh::FyshLexer::random() noexcept {
  for (size_t i = 0; i < 3; i++) {
    if (get() != '#') {
      gotoEndOfToken();
      return Fysh(Species::INVALID);
    }
  }
  if (peek() == '>') {
    get();
    return Fysh(Species::RANDOM);
  }
  gotoEndOfToken();
  return Fysh(Species::INVALID);
}



fysh::Fysh fysh::FyshLexer::scales(bool positive = true) noexcept {
  auto c{get()};
  uint32_t value{c == '{' || c == '}'};
  while (isScale(peek())) {
    auto c{get()};
    value = (value << 1) | (c == '{' || c == '}');
  }

  /*
  validate end of fysh
  valid ends:
  >
  o>
  ><
  */

  c = get(); // eye or end

  // check if the current character is an eye or >
  if (!isFyshEye(c) && c != '>') {
    gotoEndOfToken();
    return Fysh{Species::INVALID};
  }

  // check if its the end of the token or not (2nd end character)
  if (!isSpace(peek())) {
    c = get();
    if ((positive && (c != '>' || c == '<')) ||
        (!positive && (c != '<' || c == '>'))) {
      gotoEndOfToken();
      return Fysh{Species::INVALID};
    }
  }

  // make sure the token ends
  if (!isSpace(peek()) && peek() != '\0') {
    gotoEndOfToken();
    return Fysh{Species::INVALID};
  }

  if (positive) {
    return Fysh{value};
  } else {
    return Fysh{~value};
  }
}

