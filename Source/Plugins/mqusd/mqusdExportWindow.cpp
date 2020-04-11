#include "pch.h"
#include "mqusd.h"
#include "mqusdExportWindow.h"

namespace mqusd {

mqusdExportWindow::mqusdExportWindow(MQBasePlugin* plugin, MQWindowBase& parent)
    : super(parent)
{
    m_plugin = plugin;

    setlocale(LC_ALL, "");
    SetTitle(L"Export USD");
    SetOutSpace(0.4);

    double outer_margin = 0.2;
    double inner_margin = 0.1;

    {
        MQFrame* vf = CreateVerticalFrame(this);
        vf->SetOutSpace(outer_margin);
        vf->SetInSpace(inner_margin);

        {
            MQFrame* hf = CreateHorizontalFrame(vf);
            CreateLabel(hf, L"Scale Factor");
            m_edit_scale = CreateEdit(hf);
            m_edit_scale->SetNumeric(MQEdit::NUMERIC_DOUBLE);
            m_edit_scale->AddChangedEvent(this, &mqusdExportWindow::OnSettingsUpdate);
        }

        m_check_normals = CreateCheckBox(vf, L"Export Normals");
        m_check_normals->AddChangedEvent(this, &mqusdExportWindow::OnSettingsUpdate);

        m_check_colors = CreateCheckBox(vf, L"Export Vertex Colors");
        m_check_colors->AddChangedEvent(this, &mqusdExportWindow::OnSettingsUpdate);

        m_check_mids = CreateCheckBox(vf, L"Export Material IDs");
        m_check_mids->AddChangedEvent(this, &mqusdExportWindow::OnSettingsUpdate);

        m_check_blendshapes = CreateCheckBox(vf, L"Export Morphs");
        m_check_blendshapes->AddChangedEvent(this, &mqusdExportWindow::OnSettingsUpdate);

        m_check_skeletons = CreateCheckBox(vf, L"Export Bones");
        m_check_skeletons->AddChangedEvent(this, &mqusdExportWindow::OnSettingsUpdate);

        m_check_mirror = CreateCheckBox(vf, L"Freeze Mirror");
        m_check_mirror->AddChangedEvent(this, &mqusdExportWindow::OnSettingsUpdate);

        m_check_lathe = CreateCheckBox(vf, L"Freeze Lathe");
        m_check_lathe->AddChangedEvent(this, &mqusdExportWindow::OnSettingsUpdate);

        m_check_subdiv = CreateCheckBox(vf, L"Freeze Subdiv");
        m_check_subdiv->AddChangedEvent(this, &mqusdExportWindow::OnSettingsUpdate);

        m_check_flip_x = CreateCheckBox(vf, L"Flip X");
        m_check_flip_x->AddChangedEvent(this, &mqusdExportWindow::OnSettingsUpdate);

        m_check_flip_yz = CreateCheckBox(vf, L"Flip YZ");
        m_check_flip_yz->AddChangedEvent(this, &mqusdExportWindow::OnSettingsUpdate);

        m_check_flip_faces = CreateCheckBox(vf, L"Flip Faces");
        m_check_flip_faces->AddChangedEvent(this, &mqusdExportWindow::OnSettingsUpdate);

        m_check_merge = CreateCheckBox(vf, L"Merge Meshes");
        m_check_merge->AddChangedEvent(this, &mqusdExportWindow::OnSettingsUpdate);
    }

    {
        MQFrame* vf = CreateVerticalFrame(this);
        vf->SetOutSpace(outer_margin);
        vf->SetInSpace(inner_margin);

        m_button_export = CreateButton(vf, L"Export");
        m_button_export->AddClickEvent(this, &mqusdExportWindow::OnExportClicked);
    }

    {
        MQFrame* vf = CreateVerticalFrame(this);
        vf->SetOutSpace(outer_margin);
        vf->SetInSpace(inner_margin);

        m_log = CreateMemo(vf);
        m_log->SetHorzBarStatus(MQMemo::SCROLLBAR_OFF);
        m_log->SetVertBarStatus(MQMemo::SCROLLBAR_OFF);
    }

    this->AddShowEvent(this, &mqusdExportWindow::OnShow);
    this->AddHideEvent(this, &mqusdExportWindow::OnHide);
}

BOOL mqusdExportWindow::OnShow(MQWidgetBase* sender, MQDocument doc)
{
    m_log->SetText(L"");
    SyncSettings();
    return 0;
}

BOOL mqusdExportWindow::OnHide(MQWidgetBase* sender, MQDocument doc)
{
    return 0;
}

BOOL mqusdExportWindow::OnSettingsUpdate(MQWidgetBase* sender, MQDocument doc)
{
    auto& opt = m_options;
    {
        auto str = mu::ToMBS(m_edit_scale->GetText());
        auto value = std::atof(str.c_str());
        if (value != 0.0)
            opt.scale_factor = (float)value;
    }
    opt.freeze_mirror = m_check_mirror->GetChecked();
    opt.freeze_lathe = m_check_lathe->GetChecked();
    opt.freeze_subdiv = m_check_subdiv->GetChecked();
    opt.export_normals = m_check_normals->GetChecked();
    opt.export_colors = m_check_colors->GetChecked();
    opt.export_material_ids = m_check_mids->GetChecked();
    opt.export_blendshapes = m_check_blendshapes->GetChecked();
    opt.export_skeletons = m_check_skeletons->GetChecked();

    opt.flip_x = m_check_flip_x->GetChecked();
    opt.flip_yz = m_check_flip_yz->GetChecked();
    opt.flip_faces = m_check_flip_faces->GetChecked();
    opt.merge_meshes = m_check_merge->GetChecked();

    return 0;
}

BOOL mqusdExportWindow::OnExportClicked(MQWidgetBase* sender, MQDocument doc)
{
    if (DoExport(doc)) {
        SetVisible(false);
        return true;
    }
    return false;
}

void mqusdExportWindow::SyncSettings()
{
    const size_t buf_len = 128;
    wchar_t buf[buf_len];
    auto& opt = m_options;

    swprintf(buf, buf_len, L"%.3f", opt.scale_factor);
    m_edit_scale->SetText(buf);

    m_check_mirror->SetChecked(opt.freeze_mirror);
    m_check_lathe->SetChecked(opt.freeze_lathe);
    m_check_subdiv->SetChecked(opt.freeze_subdiv);

    m_check_normals->SetChecked(opt.export_normals);
    m_check_colors->SetChecked(opt.export_colors);
    m_check_mids->SetChecked(opt.export_material_ids);
    m_check_blendshapes->SetChecked(opt.export_blendshapes);
    m_check_skeletons->SetChecked(opt.export_skeletons);

    m_check_flip_x->SetChecked(opt.flip_x);
    m_check_flip_yz->SetChecked(opt.flip_yz);
    m_check_flip_faces->SetChecked(opt.flip_faces);
    m_check_merge->SetChecked(opt.merge_meshes);
}

void mqusdExportWindow::SetOutputPath(const std::wstring& path)
{
    m_out_path = path;
}

bool mqusdExportWindow::DoExport(MQDocument doc)
{
    auto scene = CreateUSDScene();
    if (!scene) {
        // todo: log
        return false;
    }

    if (!scene->create(mu::ToMBS(m_out_path).c_str())) {
        scene = {};
        // todo: log
        return false;
    }

    auto exporter = std::make_shared<DocumentExporter>(m_plugin, scene.get(), &m_options);
    exporter->initialize(doc);
    exporter->write(doc, true);
    return true;
}

void mqusdExportWindow::LogInfo(const char* message)
{
    if (m_log && message)
        m_log->SetText(mu::ToWCS(message));
}

} // namespace mqusd
