#ifndef LOADUI_H
#define LOADUI_H

#include <string>
using namespace std;

namespace VTxx {
struct LoadData {
  string platform;
  string filename;
};

LoadData show_load_ui(int argc, char *argv[]);

}; // namespace VTxx

#endif /* end of include guard: LOADUI_H */
