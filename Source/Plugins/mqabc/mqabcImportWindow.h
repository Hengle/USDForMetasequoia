#pragma once
#include "MQWidget.h"
#include "mqCommon/mqTWindow.h"
#include "mqCommon/mqDocumentImporter.h"

namespace mqusd {

class mqabcImportWindow : public mqTWindow<mqabcImportWindow>
{
using super = mqTWindow<mqabcImportWindow>;
friend mqabcImportWindow* super::create(mqabcPlugin* plugin);
protected:
    mqabcImportWindow(mqabcPlugin* plugin, MQWindowBase& parent);

public:
    BOOL OnShow(MQWidgetBase* sender, MQDocument doc);
    BOOL OnHide(MQWidgetBase* sender, MQDocument doc);
    BOOL OnOpenClicked(MQWidgetBase* sender, MQDocument doc);
    BOOL OnSampleEdit(MQWidgetBase* sender, MQDocument doc);
    BOOL OnSampleSlide(MQWidgetBase* sender, MQDocument doc);
    BOOL OnSettingsUpdate(MQWidgetBase* sender, MQDocument doc);

    void SyncSettings();
    void LogInfo(const char *message);

    void SetAdditive(bool v);
    bool Open(MQDocument doc, const std::string& path);
    bool Close();
    void Seek(MQDocument doc, double t);
    void Refresh(MQDocument doc);
    bool IsOpened() const;
    double GetTimeRange() const;

private:
    mqabcPlugin* m_plugin = nullptr;

    MQFrame* m_frame_open = nullptr;
    MQButton* m_button_open = nullptr;
    MQFrame* m_frame_play = nullptr;
    MQEdit* m_edit_time = nullptr;
    MQSlider* m_slider_time = nullptr;
    MQEdit* m_edit_scale = nullptr;

    MQCheckBox* m_check_flip_x = nullptr;
    MQCheckBox* m_check_flip_yz = nullptr;
    MQCheckBox* m_check_flip_faces = nullptr;

    MQCheckBox* m_check_merge = nullptr;
    MQMemo* m_log = nullptr;


    bool m_additive = false;
    ScenePtr m_scene;
    ImportOptions m_options;
    DocumentImporterPtr m_importer;
    double m_seek_time = 0;
};

} // namespace mqusd
