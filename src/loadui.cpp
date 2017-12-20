#include "loadui.hpp"
#include <wx/wx.h>

#include <cstdlib>
#include <utility>
#include <vector>
namespace VTxx {

static const vector<pair<string, string>> plat_list = {
    {"vt168", "Vanilla VT168"},
    {"miwi2", "Macro Winners MiWi 2"},
    {"miwi2", "InterAct G5400"}};
wxApp *app;

static LoadData dat;

class LoadUI : public wxApp {
  bool OnInit() {
    dat.platform = display_plat_picker();
    dat.filename = display_rom_picker();
    return false;
  }

  string display_plat_picker() {
    wxArrayString choices;
    for (auto plat : plat_list)
      choices.Add(plat.second);
    int sel = wxGetSingleChoiceIndex("Please select a platform to emulate",
                                     "Platforms", choices);
    if (sel == -1)
      exit(2);
    return plat_list.at(sel).first;
  };

  string display_rom_picker() {
    wxFileDialog dlg(NULL, _("Select ROM"), "", "", "VTxxx ROMs (*.bin)|*.bin",
                     wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    int result = dlg.ShowModal();
    if (result != wxID_OK)
      exit(1);
    return string(dlg.GetPath());
  };
};

wxIMPLEMENT_APP_NO_MAIN(LoadUI);

LoadData show_load_ui(int argc, char *argv[]) {
  wxApp *app = new LoadUI();
  wxApp::SetInstance(app);
  wxEntry(argc, argv);
  while (app->IsMainLoopRunning()) {
    app->MainLoop();
  }
  return dat;
}

}; // namespace VTxx
