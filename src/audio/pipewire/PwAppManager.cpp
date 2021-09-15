#include "PwAppManager.h"

PwAppManager::PwAppManager(PwPipelineManager* mgr) : mgr(mgr)
{
    mgr->stream_output_added.connect(sigc::mem_fun(*this, &PwAppManager::onAppAdded));
    mgr->stream_output_changed.connect(sigc::mem_fun(*this, &PwAppManager::onAppChanged));
    mgr->stream_output_removed.connect(sigc::mem_fun(*this, &PwAppManager::onAppRemoved));
}

void PwAppManager::onAppAdded(const uint id, const std::string name, const std::string media_class)
{
    for (int n = 0; n < apps.length(); n++) {
      if (apps[n].id == id) {
        return;
      }
    }

    NodeInfo node_info;

    try {
      node_info = mgr->node_map.at(id);
    } catch (...) {
      return;
    }

    apps.append(node_info);
    emit appAdded(node_info);
}

void PwAppManager::onAppChanged(const uint id)
{
    for (int n = 0; n < apps.length(); n++) {
        if (auto item = apps[n]; item.id == id) {
          try {
            item = mgr->node_map.at(id);
          } catch (...) {
            return;
          }

          emit appChanged(item);
          break;
        }
    }
}

void PwAppManager::onAppRemoved(const uint id)
{
    for (int n = 0; n < apps.length(); n++)
    {
      if (apps[n].id == id)
      {
          apps.removeAt(n);
          emit appRemoved(id);
          break;
      }
    }
}
