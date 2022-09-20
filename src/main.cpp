#include <array>
#include <unordered_map>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stack>
#include <string_view>
#include <vector>
#include <fmt/core.h>
#include <fmt/ostream.h>

enum ReturnCode : uint32_t {
  ExitSuccess,
  BufferOverflow,
  BufferUnderflow,
  UnmatchedBrackets,
  FileOpenFailed,
};

enum Flag : uint32_t { 
  ErrorBufferUnderflow = 1 << 0
};

enum CommandType : uint8_t {
  Increment = '+',
  Decrement = '-',
  MoveLeft  = '<',
  MoveRight = '>',
  LoopBegin = '[',
  LoopEnd   = ']',
  Input     = ',',
  Output    = '.',
  SetZero   = 0x08, // backspace char that probably won't be used
};

struct Command {
  CommandType type{};
  uint32_t count{1};
  Command(CommandType _t) : type(_t) {}
};

class Interpreter {
  static constexpr size_t maxSize = 30000;

  std::array<uint8_t, maxSize> arr{};

  std::vector<Command> commands;
  
  size_t relPtr{};

  ReturnCode parseCode(std::string_view code) {
    // positions of '['
    std::stack<size_t> brackets{};

    fmt::print("Parsing code...\n");

    for (size_t ind = 0;ind < code.length();++ind) {
      auto type = CommandType(code[ind]);
      switch (type) {
        case '[':
          brackets.push(commands.size());
          commands.push_back(Command(type));
          break;
        case ']':
          commands.push_back(Command(type));
          commands[brackets.top()].count = commands.size() - 1;
          commands.back().count = brackets.top();
          brackets.pop();
          break;
        default:
          if (!commands.empty() && type == commands.back().type)
            ++commands.back().count;
          else commands.push_back(Command(type));
          break;
      }
    }
    if (!brackets.empty()) return UnmatchedBrackets;
    fmt::print("Parsed successfully.\n");
    return ExitSuccess;
  }

public:
  ReturnCode executeCode(std::string_view _c, uint32_t flags = 0) {
    size_t commandInd{};
    relPtr = 0;
    arr = {0};
    ReturnCode rc = parseCode(_c);
    if (rc) return rc;
    for (; commandInd < commands.size(); ++commandInd) {
      const Command& cur = commands[commandInd];
      switch (cur.type) {
      case '>':
        relPtr+=cur.count;
        break;
      case '<':
        relPtr-=cur.count;
        break;
      case '+':
        if (relPtr > maxSize - 1)
          return BufferOverflow;
        if (arr.at(relPtr) >= 0xFF - cur.count + 1 && (flags & ErrorBufferUnderflow))
          return BufferUnderflow;
        arr.at(relPtr)+=cur.count;
        break;
      case '-':
        if (relPtr > maxSize - 1)
          return BufferOverflow;
        if (arr.at(relPtr) <= 0x00 + cur.count - 1 && (flags & ErrorBufferUnderflow))
          return BufferUnderflow;
        arr.at(relPtr)-=cur.count;
        break;
      case '.':
        for (size_t i = 0;i<cur.count;++i)
          std::putchar(arr.at(relPtr));
        break;
      case ',':
        for (size_t i = 0;i<cur.count;++i)
          arr.at(relPtr) = std::cin.get();
        break;
      case '[':
        if (arr.at(relPtr) == 0)
          commandInd = cur.count;
        break;
      case ']':
        if (arr.at(relPtr) != 0) 
          commandInd = cur.count;
        break;
      // Optimizer generated command '[-]' '[+]'
      case SetZero:
        arr.at(relPtr) = 0;
      }
    }
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
  uint32_t flags{};

  for (int ind = 1;ind<argc;++ind) {
    if (strcmp(argv[ind],"--Eunderflow") == 0 || strcmp(argv[ind],"-uf") == 0) {
      flags |= ErrorBufferUnderflow;
      continue;
    }
    file_name = argv[ind];
  }
  std::ifstream ins(file_name);
  if (ins.is_open()) {
    std::stringstream ss{};
    ss << ins.rdbuf();
    result = bf.executeCode(ss.str(), flags);
  }
  else result = FileOpenFailed;
  fmt::print("{}\n", result);
  return result;
}
