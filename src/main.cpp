#include <array>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stack>
#include <string_view>
#include <fmt/core.h>
#include <fmt/ostream.h>

enum ReturnCode : uint8_t {
  ExitSuccess,
  BufferOverflow,
  BufferUnderflow,
  UnmatchedBrackets
};

struct Interpreter {
  static constexpr size_t maxSize = 30000;

  std::array<char, maxSize> arr{};

  std::stack<size_t> bracketInd{};

  std::string code;

  size_t relPtr{};
  size_t commandInd{};

  ReturnCode handleBracket() {
    if (code[commandInd] == '[') {
      bracketInd.push(commandInd++);
      if (arr.at(relPtr) == 0) {
        for (; !bracketInd.empty(); ++commandInd) {
          if (commandInd >= code.length())
            return UnmatchedBrackets;
          if (code[commandInd] == '[')
            bracketInd.push(commandInd);
          if (code[commandInd] == ']')
            bracketInd.pop();
        }
      }
    }

    if (arr.at(relPtr) == 0 && code[commandInd] == ']') {
      ++commandInd;
      if (bracketInd.empty())
        return UnmatchedBrackets;
      bracketInd.pop();
    }

    if (arr.at(relPtr) != 0 && code[commandInd] == ']') {
      if (bracketInd.empty())
        return UnmatchedBrackets;
      commandInd = bracketInd.top();
    }
    return ExitSuccess;
  }

  ReturnCode executeCode(std::string_view _c) {
    code = _c;
    relPtr = commandInd = 0;
    arr = {0};

    for (; commandInd < code.length(); ++commandInd) {
      switch (code[commandInd]) {
      case '>':
        if (relPtr == maxSize - 1)
          return BufferOverflow;
        ++relPtr;
        break;
      case '<':
        if (relPtr == 0)
          return BufferUnderflow;
        --relPtr;
        break;
      case '+':
        ++arr.at(relPtr);
        break;
      case '-':
        --arr.at(relPtr);
        break;
      case '.':
        std::putchar(arr.at(relPtr));
        break;
      case ',':
        std::cout << '>';
        std::cin >> arr.at(relPtr);
        break;
      case '[':
      case ']':
        ReturnCode result = handleBracket();
        if (result) return result;
        break;
      }
    }
    if (!bracketInd.empty())
      return UnmatchedBrackets;
    return ExitSuccess;
  }
};

void printUsage() {
  fmt::print("Usage: bf file_name\n");
  std::exit(0);
}

std::ostream &operator<<(std::ostream &os, const ReturnCode &rc) {
  switch (rc) {
  case BufferUnderflow:
    os << "[ERROR]   Buffer underflow";
    break;
  case BufferOverflow:
    os << "[ERROR]   Buffer overflow";
    break;
  case UnmatchedBrackets:
    os << "[ERROR]   Unmatched brackets";
    break;
  case ExitSuccess:
    os << "Program interpreted successfully!";
  }
  return os;
}

template <>
struct fmt::formatter<ReturnCode> : ostream_formatter {};

int main(int argc, char **argv) {
  if (argc <= 1)
    printUsage();
  Interpreter bf;
  std::ifstream ins(argv[1]);
  std::stringstream ss{};
  ss << ins.rdbuf();
  fmt::print("Executing...\n");
  ReturnCode result = bf.executeCode(ss.str());
  fmt::print("{}\n", result);
  return result;
}
