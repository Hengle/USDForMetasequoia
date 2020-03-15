﻿#include "pch.h"
#include "mqusd.h"
#include "mqusdRecorderPlugin.h"
#include "mqusdRecorderWindow.h"


bool mqusdRecorderPlugin::OpenUSD(const std::string& path)
{
    m_scene = CreateUSDScene();
    if (!m_scene)
        return false;

    if (!m_scene->create(path.c_str())) {
        m_scene = {};
        return false;
    }


    // create nodes
    auto node_name = mu::GetFilename_NoExtension(m_mqo_path.c_str());
    if (node_name.empty())
        node_name = mu::GetFilename_NoExtension(path.c_str());
    if (node_name.empty())
        node_name = "Untitled";

    auto root_node = m_scene->root_node;
    auto mesh_node = m_scene->createNode(root_node, node_name.c_str(), Node::Type::Mesh);
    m_mesh_node = static_cast<MeshNode*>(mesh_node);

    m_recording = true;
    m_dirty = true;

    LogInfo("succeeded to open %s\nrecording started", path.c_str());
    return true;
}

bool mqusdRecorderPlugin::CloseUSD()
{
    if (!m_scene)
        return false;

    WaitFlush();

    if (m_settings.capture_materials) {
        WriteMaterials();
    }

    // flush archive
    m_scene->save();
    m_scene = {};

    m_frame = 0;
    m_time = 0.0;
    m_start_time = m_last_flush = 0;
    m_recording = false;

    m_obj_records.clear();
    m_material_records.clear();

    LogInfo("recording finished");
    return true;
}

const std::string& mqusdRecorderPlugin::GetMQOPath() const
{
    return m_mqo_path;
}
const std::string& mqusdRecorderPlugin::GetUSDPath() const
{
    return m_scene->path;
}

bool mqusdRecorderPlugin::IsArchiveOpened() const
{
    return m_scene != nullptr;
}

bool mqusdRecorderPlugin::IsRecording() const
{
    return m_recording && m_scene;
}
void mqusdRecorderPlugin::SetRecording(bool v)
{
    m_recording = v;
}

void mqusdRecorderPlugin::SetInterval(double v)
{
    m_interval = mu::S2NS(v);
}
double mqusdRecorderPlugin::GetInterval() const
{
    return mu::NS2Sd(m_interval);
}

mqusdRecorderSettings& mqusdRecorderPlugin::GetSettings()
{
    return m_settings;
}


void mqusdRecorderPlugin::MarkSceneDirty()
{
    m_dirty = true;
}

void mqusdRecorderPlugin::CaptureFrame(MQDocument doc)
{
    if (!IsRecording() || !m_dirty)
        return;

    auto t = mu::Now();
    if (t - m_last_flush < m_interval)
        return;

    m_dirty = false;
    m_last_flush = t;
    if (m_start_time == 0)
        m_start_time = t;

    WaitFlush();

    // prepare
    int nobjects = doc->GetObjectCount();
    m_obj_records.resize(nobjects);
    for (int oi = 0; oi < nobjects; ++oi) {
        auto& rec = m_obj_records[oi];
        rec.mqdocument = doc;

        auto obj = doc->GetObject(oi);
        if ((obj->GetMirrorType() != MQOBJECT_MIRROR_NONE && m_settings.freeze_mirror) ||
            (obj->GetLatheType() != MQOBJECT_LATHE_NONE && m_settings.freeze_lathe) ||
            (obj->GetPatchType() != MQOBJECT_PATCH_NONE && m_settings.freeze_subdiv))
        {
            rec.mqobject = obj->Clone();

            DWORD freeze_flags = 0;
            if (m_settings.freeze_mirror)
                freeze_flags |= MQOBJECT_FREEZE_MIRROR;
            if (m_settings.freeze_lathe)
                freeze_flags |= MQOBJECT_FREEZE_LATHE;
            if (m_settings.freeze_subdiv)
                freeze_flags |= MQOBJECT_FREEZE_PATCH;
            rec.mqobject->Freeze(freeze_flags);

            rec.need_release = true;
        }
        else {
            rec.mqobject = obj;
        }
    }

    int nmaterials = doc->GetMaterialCount();
    m_material_records.resize(nmaterials);
    for (int mi = 0; mi < nmaterials; ++mi) {
        auto& rec = m_material_records[mi];
        rec.mqdocument = doc;

        auto mtl = doc->GetMaterial(mi);
        rec.mqmaterial = mtl;

        char buf[256];
        mtl->GetName(buf, sizeof(buf));

        auto& dst = rec.material;
        dst.name = buf;
        dst.shader = mtl->GetShader();
        dst.use_vertex_color = mtl->GetVertexColor() == MQMATERIAL_VERTEXCOLOR_DIFFUSE;
        dst.double_sided = mtl->GetDoubleSided();
        dst.color = to_float3(mtl->GetColor());
        dst.diffuse = mtl->GetDiffuse();
        dst.alpha = mtl->GetAlpha();
        dst.ambient_color = to_float3(mtl->GetAmbientColor());
        dst.specular_color = to_float3(mtl->GetSpecularColor());
        dst.emission_color = to_float3(mtl->GetEmissionColor());
    }

    // extract mesh data
    mu::parallel_for(0, nobjects, [this](int oi) {
        ExtractMeshData(m_obj_records[oi]);
    });

    if (m_settings.keep_time)
        m_time = mu::NS2Sd(m_last_flush - m_start_time) * m_settings.time_scale;
    else
        m_time = (1.0 / m_settings.frame_rate) * m_frame;
    ++m_frame;

    // flush usd async
    m_task_write = std::async(std::launch::async, [this]() { FlushUSD(); });

    // log
    int total_vertices = 0;
    int total_faces = 0;
    for (auto& rec : m_obj_records) {
        total_vertices += (int)rec.mesh.points.size();
        total_faces += (int)rec.mesh.counts.size();
    }
    LogInfo("frame %d: %d vertices, %d faces",
        m_frame - 1, total_vertices, total_faces);
}

void mqusdRecorderPlugin::ExtractMeshData(ObjectRecord& rec)
{
    auto obj = rec.mqobject;
    int nfaces = obj->GetFaceCount();
    int npoints = obj->GetVertexCount();
    int nindices = 0;
    for (int fi = 0; fi < nfaces; ++fi)
        nindices += obj->GetFacePointCount(fi);

    auto& dst = rec.mesh;
    dst.resize(npoints, nindices, nfaces);

    auto dst_points = dst.points.data();
    auto dst_normals = dst.normals.data();
    auto dst_uvs = dst.uvs.data();
    auto dst_colors = dst.colors.data();
    auto dst_mids = dst.material_ids.data();
    auto dst_counts = dst.counts.data();
    auto dst_indices = dst.indices.data();

    dst.name = obj->GetName();

    // points
    obj->GetVertexArray((MQPoint*)dst_points);

    int fc = 0; // 'actual' face count
    for (int fi = 0; fi < nfaces; ++fi) {
        // counts
        // GetFacePointCount() may return 0 for unknown reason. skip it.
        int count = obj->GetFacePointCount(fi);
        if (count == 0)
            continue;
        dst_counts[fc] = count;

        // material IDs
        dst_mids[fc] = obj->GetFaceMaterial(fi);

        // indices
        obj->GetFacePointArray(fi, dst_indices);
        dst_indices += count;

        // uv
        obj->GetFaceCoordinateArray(fi, (MQCoordinate*)dst_uvs);
        dst_uvs += count;

        for (int ci = 0; ci < count; ++ci) {
            // vertex color
            *(dst_colors++) = mu::Color32ToFloat4(obj->GetFaceVertexColor(fi, ci));

#if MQPLUGIN_VERSION >= 0x0460
            // normal
            BYTE flags;
            obj->GetFaceVertexNormal(fi, ci, flags, (MQPoint&)*(dst_normals++));
#endif
        }

        ++fc;
    }
    mu::InvertV(dst.uvs.data(), dst.uvs.size());

    if (nfaces != fc) {
        // refit
        dst.counts.resize(fc);
        dst.material_ids.resize(fc);
    }

    dst.applyScale(m_settings.scale_factor);
}

void mqusdRecorderPlugin::FlushUSD()
{
    // make merged mesh
    auto& dst = m_mesh_node->mesh;
    dst.clear();
    for (auto& rec : m_obj_records)
        dst.merge(rec.mesh);

    // do write
    m_scene->write(m_time);
}

void mqusdRecorderPlugin::WaitFlush()
{
    if (!m_task_write.valid())
        return;

    m_task_write.wait();
    m_task_write = {};

    for (auto& rec : m_obj_records) {
        if (rec.need_release) {
            rec.need_release = false;
            rec.mqobject->DeleteThis();
            rec.mqobject = nullptr;
        }
    }

}

void mqusdRecorderPlugin::WriteMaterials()
{
    if (m_material_records.empty())
        return;

}
