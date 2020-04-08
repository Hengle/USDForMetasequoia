#include "pch.h"
#include "mqabcPlugin.h"
#include "mqabcImportWindow.h"
#include "SceneGraph/ABC/sgabc.h"

namespace mqusd {

mqabcImportWindow::mqabcImportWindow(mqabcPlugin* plugin, MQWindowBase& parent)
    : super(parent)
{
    setlocale(LC_ALL, "");

    m_plugin = plugin;

    SetTitle(L"Import Alembic");
    SetOutSpace(0.4);

    double outer_margin = 0.2;
    double inner_margin = 0.1;


    {
        MQFrame* vf = CreateVerticalFrame(this);
        vf->SetOutSpace(outer_margin);
        vf->SetInSpace(inner_margin);
        m_frame_open = vf;

        m_button_open = CreateButton(vf, L"Open");
        m_button_open->AddClickEvent(this, &mqabcImportWindow::OnOpenClicked);
    }
    {
        MQFrame* vf = CreateVerticalFrame(this);
        vf->SetOutSpace(outer_margin);
        vf->SetInSpace(inner_margin);
        vf->SetVisible(false);
        m_frame_play = vf;

        MQFrame* hf = CreateHorizontalFrame(vf);
        CreateLabel(hf, L"Time");
        m_edit_time = CreateEdit(hf);
        m_edit_time->SetNumeric(MQEdit::NUMERIC_INT);
        m_edit_time->SetText(L"0");
        m_edit_time->AddChangedEvent(this, &mqabcImportWindow::OnSampleEdit);

        m_slider_time = CreateSlider(vf);
        m_slider_time->AddChangingEvent(this, &mqabcImportWindow::OnSampleSlide);
    }
    {
        MQFrame* vf = CreateVerticalFrame(this);
        vf->SetOutSpace(outer_margin);
        vf->SetInSpace(inner_margin);

        MQFrame* hf = CreateHorizontalFrame(vf);
        CreateLabel(hf, L"Scale Factor");
        m_edit_scale = CreateEdit(hf);
        m_edit_scale->SetNumeric(MQEdit::NUMERIC_DOUBLE);
        m_edit_scale->AddChangedEvent(this, &mqabcImportWindow::OnSettingsUpdate);

        m_check_flip_x = CreateCheckBox(vf, L"Flip X");
        m_check_flip_x->AddChangedEvent(this, &mqabcImportWindow::OnSettingsUpdate);

        m_check_flip_yz = CreateCheckBox(vf, L"Flip YZ");
        m_check_flip_yz->AddChangedEvent(this, &mqabcImportWindow::OnSettingsUpdate);

        m_check_flip_faces = CreateCheckBox(vf, L"Flip Faces");
        m_check_flip_faces->AddChangedEvent(this, &mqabcImportWindow::OnSettingsUpdate);

        m_check_merge = CreateCheckBox(vf, L"Merge Meshes");
        m_check_merge->AddChangedEvent(this, &mqabcImportWindow::OnSettingsUpdate);
    }

    {
        MQFrame* vf = CreateVerticalFrame(this);
        vf->SetOutSpace(outer_margin);
        vf->SetInSpace(inner_margin);

        m_log = CreateMemo(vf);
        m_log->SetHorzBarStatus(MQMemo::SCROLLBAR_OFF);
        m_log->SetVertBarStatus(MQMemo::SCROLLBAR_OFF);
    }

    this->AddShowEvent(this, &mqabcImportWindow::OnShow);
    this->AddHideEvent(this, &mqabcImportWindow::OnHide);
}

BOOL mqabcImportWindow::OnShow(MQWidgetBase* sender, MQDocument doc)
{
    SyncSettings();
    return 0;
}

BOOL mqabcImportWindow::OnHide(MQWidgetBase* sender, MQDocument doc)
{
    Close();
    m_plugin->WindowClose();

    m_frame_open->SetVisible(true);
    m_frame_play->SetVisible(false);
    return 0;
}

BOOL mqabcImportWindow::OnOpenClicked(MQWidgetBase* sender, MQDocument doc)
{
    if (!IsOpened()) {
        MQOpenFileDialog dlg(*this);
        dlg.AddFilter(L"Alembic Files (*.abc)|*.abc");
        dlg.AddFilter(L"All Files (*.*)|*.*");
        dlg.SetDefaultExt(L"usd");
        if (dlg.Execute()) {
            auto path = dlg.GetFileName();
            if (Open(doc, mu::ToMBS(path))) {
                m_slider_time->SetMin(0.0);
                m_slider_time->SetMax(GetTimeRange());

                m_frame_open->SetVisible(false);
                m_frame_play->SetVisible(true);
            }
        }
    }
    else {
    }

    return 0;
}

BOOL mqabcImportWindow::OnSampleEdit(MQWidgetBase* sender, MQDocument doc)
{
    auto str = mu::ToMBS(m_edit_time->GetText());
    auto value = std::atof(str.c_str());
    m_slider_time->SetPosition(value);
    Repaint(true);

    Seek(doc, value);
    return 0;
}

BOOL mqabcImportWindow::OnSampleSlide(MQWidgetBase* sender, MQDocument doc)
{
    const size_t buf_len = 128;
    wchar_t buf[buf_len];
    auto value = m_slider_time->GetPosition();
    swprintf(buf, buf_len, L"%lf", value);
    m_edit_time->SetText(buf);
    Repaint(true);

    Seek(doc, value);
    return 0;
}

BOOL mqabcImportWindow::OnSettingsUpdate(MQWidgetBase* sender, MQDocument doc)
{
    auto& opt = m_options;

    {
        auto str = mu::ToMBS(m_edit_scale->GetText());
        auto value = std::atof(str.c_str());
        if (value != 0.0)
            opt.scale_factor = (float)value;
    }

    opt.flip_x = m_check_flip_x->GetChecked();
    opt.flip_yz = m_check_flip_yz->GetChecked();
    opt.flip_faces = m_check_flip_faces->GetChecked();
    opt.merge_meshes = m_check_merge->GetChecked();

    Refresh(doc);
    return 0;
}

void mqabcImportWindow::SyncSettings()
{
    const size_t buf_len = 128;
    wchar_t buf[buf_len];
    auto& opt = m_options;

    swprintf(buf, buf_len, L"%.2f", opt.scale_factor);
    m_edit_scale->SetText(buf);

    m_check_flip_x->SetChecked(opt.flip_x);
    m_check_flip_yz->SetChecked(opt.flip_yz);
    m_check_flip_faces->SetChecked(opt.flip_faces);
    m_check_merge->SetChecked(opt.merge_meshes);
}

void mqabcImportWindow::LogInfo(const char* message)
{
    if (m_log) {
        m_log->SetText(mu::ToWCS(message));
    }
}


void mqabcImportWindow::SetAdditive(bool v)
{
    m_additive = v;
    if (v)
        SetTitle(L"Insert Alembic");
    else
        SetTitle(L"Import Alembic");
}

bool mqabcImportWindow::Open(MQDocument doc, const std::string& path)
{
    Close();

    m_scene = CreateABCIScene();
    if (!m_scene)
        return false;

    if (!m_scene->open(path.c_str())) {
        mqusdLog(
            "failed to open %s\n"
            "it may not an usd file"
            , path.c_str());
        m_scene = {};
        return false;
    }

    m_importer.reset(new DocumentImporter(m_plugin, m_scene.get(), &m_options));
    m_importer->initialize(doc, m_additive);

    return true;
}

bool mqabcImportWindow::Close()
{
    m_importer = {};
    m_scene = {};
    m_seek_time = 0;
    return true;
}

void mqabcImportWindow::Seek(MQDocument doc, double t)
{
    if (!m_importer)
        return;

    m_seek_time = t + m_scene->time_start;
    m_importer->read(doc, m_seek_time);

    // repaint
    MQ_RefreshView(nullptr);
}

void mqabcImportWindow::Refresh(MQDocument doc)
{
    Seek(doc, m_seek_time);
}

bool mqabcImportWindow::IsOpened() const
{
    return m_scene != nullptr;
}

double mqabcImportWindow::GetTimeRange() const
{
    return m_scene ? m_scene->time_end - m_scene->time_start : 0.0;
}

} // namespace mqusd
