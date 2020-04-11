#include "mqusd.h"
#include "mqsdk/sdk_Include.h"
#include "mqCommon/mqDummyPlugins.h"
#include "MeshUtils/MeshUtils.h"

MQBasePlugin* (*_mqusdGetExportPlugin)();

MQBasePlugin* GetPluginClass()
{
    if (!_mqusdGetExportPlugin) {
        auto mod = mu::GetModule(mqusdPluginFile);
        if (!mod) {
            std::string path = mqusd::GetPluginsDir();
            path += "Misc" muPathSep mqusdPluginFile;
            mod = mu::LoadModule(path.c_str());
        }
        if (mod) {
            (void*&)_mqusdGetExportPlugin = mu::GetSymbol(mod, "mqusdGetExportPlugin");
        }
    }
    if (_mqusdGetExportPlugin)
        return _mqusdGetExportPlugin();
    return &mqusd::DummyExportPlugin::getInstance();
}

mqusdAPI void mqusdDummy()
{
    // dummy to prevent mqsdk stripped by linker
    DWORD a, b;
    MQGetPlugInID(&a, &b);
}
