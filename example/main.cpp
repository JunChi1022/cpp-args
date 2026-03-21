#include "options.h"

int main(int argc, char **argv) {
  cppargs::ArgumentParser Parser(MyAppTable, MyAppTableGroups);
  Parser.SetAliasTable(MyAliasTable);
  if (!Parser.Parse(argc, argv)) {
    return 1;
  }


  if (Parser.HasArg(OPT_h)) {
    Parser.PrintHelp();
    return 0;
  }

  std::cout << "Render all recevied options:" << std::endl;
  for (int Opt = 0; Opt < MyApp::MyApp_COUNT; ++Opt) {
    if (Parser.HasArg(Opt)) {
        std::vector<std::string> Render;
        Parser.Render(Opt, Render);
        for (const auto &R : Render) {
            std::cout << R << " ";
        }
        std::cout << std::endl;
    }
  }

  return 0;
}
