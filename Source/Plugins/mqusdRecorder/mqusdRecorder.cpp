#include "mqusd.h"
#include "mqCommon/mqDummyPlugins.h"

MQBasePlugin* (*_mqusdGetRecorderPlugin)();

MQBasePlugin* GetPluginClass()
{
    if (!_mqusdGetRecorderPlugin) {
        auto mod = mu::GetModule(mqusdModuleFile);
        if (!mod) {
            std::string path = mqusd::GetMiscDir();
            path += mqusdModuleFile;
            mod = mu::LoadModule(path.c_str());
        }
        if (mod) {
            (void*&)_mqusdGetRecorderPlugin = mu::GetSymbol(mod, "mqusdGetRecorderPlugin");
        }
    }
    if (_mqusdGetRecorderPlugin)
        return _mqusdGetRecorderPlugin();
    return &mqusd::DummyStationPlugin::getInstance();
}

mqusdAPI void mqusdDummy()
{
    // dummy to prevent mqsdk stripped by linker
    DWORD a, b;
    MQGetPlugInID(&a, &b);
}
