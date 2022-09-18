#include <array>
#include <unordered_map>
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
  UnmatchedBrackets,
  FileOpenFailed,
};

enum Flag : uint32_t { 
  AllowBufferUnderflow = 1 << 0
};

class Interpreter {
  static constexpr size_t maxSize = 30000;

  std::array<uint8_t, maxSize> arr{};

  // key: bracket position value: position to jump to
  std::unordered_map<size_t,size_t> bracketJumps{};

  std::string code{};

  size_t relPtr{};
  size_t commandInd{};
  uint32_t flags{};

  ReturnCode parseBrackets() {
    // positions of '['
    std::stack<size_t> brackets{};
    for (size_t ind = 0;ind < code.length();++ind) {
      if (code[ind] == '[')
        brackets.push(ind);
      if (code[ind] == ']') {
        bracketJumps[brackets.top()] = ind;
        bracketJumps[ind] = brackets.top();
        brackets.pop();
      }
    }
    if (!brackets.empty()) return UnmatchedBrackets;
    return ExitSuccess;
  }

public:
  ReturnCode executeCode(std::string_view _c) {
    code = _c;
    relPtr = commandInd = 0;
    arr = {0};
    ReturnCode rc = parseBrackets();
    if (rc) return rc;
    for (; commandInd < code.length(); ++commandInd) {
      switch (code[commandInd]) {
      case '>':
        ++relPtr;
        break;
      case '<':
        --relPtr;
        break;
      case '+':
        if (relPtr > maxSize - 1 )
          return BufferOverflow;
        if (arr.at(relPtr) >= 0xFF && !(flags & AllowBufferUnderflow))
          return BufferUnderflow;
        ++arr.at(relPtr);
        break;
      case '-':
        if (relPtr > maxSize - 1 )
          return BufferOverflow;
        if (arr.at(relPtr) <= 0x00 && !(flags & AllowBufferUnderflow))
          return BufferUnderflow;
        --arr.at(relPtr);
        break;
      case '.':
        std::putchar(arr.at(relPtr));
        break;
      case ',':
        std::cin >> arr.at(relPtr);
        break;
      case '[':
        if (arr.at(relPtr) == 0) {
          commandInd = bracketJumps[commandInd];
//          fmt::print("Jumped to {}", relPtr);
        }
        break;
      case ']':
        if (arr.at(relPtr) != 0) {
          commandInd = bracketJumps[commandInd];
//          fmt::print("Jumped to {}", relPtr);
        }
        break;
      }
    }
    return ExitSuccess;
  }
  void enableFlag(Flag f) {
    flags |= f;
  }
};

void printUsage() {
  fmt::print("Usage: bf file_name\n");
  std::exit(0);
}

std::ostream &operator<<(std::ostream &os, const ReturnCode &rc) {
  switch (rc) {
  case BufferUnderflow:
    os << "[ERROR]   Buffer underflow.";
    break;
  case BufferOverflow:
    os << "[ERROR]   Buffer overflow.";
    break;
  case UnmatchedBrackets:
    os << "[ERROR]   Unmatched brackets.";
    break;
  case FileOpenFailed:
    os << "[ERROR]   Couldn't open the file.";
    break;
  case ExitSuccess:
    os << "Program interpreted successfully!";
    break;
  }
  return os;
}
template <>
struct fmt::formatter<ReturnCode> : ostream_formatter {};

int main(int argc, char **argv) {
  if (argc <= 1)
    printUsage();
  Interpreter bf;
  ReturnCode result;
  std::string file_name{};
  for (int ind = 1;ind<argc;++ind) {
    if (strcmp(argv[ind],"--Aunderflow") == 0 || strcmp(argv[ind],"-uf") == 0) {
      bf.enableFlag(AllowBufferUnderflow); continue;
    }
    file_name = argv[ind];
  }
  std::ifstream ins(file_name);
  if (ins.is_open()) {
    std::stringstream ss{};
    ss << ins.rdbuf();
    result = bf.executeCode(ss.str());
  }
  else result = FileOpenFailed;
  fmt::print("{}\n", result);
  return result;
}
