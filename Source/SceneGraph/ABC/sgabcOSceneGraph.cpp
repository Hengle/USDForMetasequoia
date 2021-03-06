#include "pch.h"
#include "sgabcOSceneGraph.h"

namespace sg {

template<class NodeT>
static inline NodeT* CreateNode(ABCONode* parent, Abc::OObject* obj)
{
    auto ret = ABCOScene::getCurrent()->getHostScene()->createNodeImpl(
        parent ? parent->m_node : nullptr,
        obj->getName().c_str(),
        NodeT::node_type);
    return static_cast<NodeT*>(ret);
}

template<class T>
static inline void PadSamples(Abc::OTypedScalarProperty<T>& dst, uint32_t n, const typename Abc::OTypedScalarProperty<T>::value_type& default_sample = {})
{
    while (dst.getNumSamples() < n)
        dst.set(default_sample);
}
template<class T>
static inline void PadSamples(Abc::OTypedArrayProperty<T>& dst, uint32_t n, const typename Abc::OTypedArrayProperty<T>::sample_type& default_sample = {})
{
    while (dst.getNumSamples() < n)
        dst.set(default_sample);
}
template<class T>
static inline void PadSamples(T& dst, uint32_t n, const typename T::Sample& default_sample = {})
{
    while (dst.getNumSamples() < n)
        dst.set(default_sample);
}

static ABCONode* GetABCNode(Node* n)
{
    return static_cast<ABCONode*>(n->impl);
}

static std::string GetABCName(Node* n)
{
    return GetABCNode(n)->getName();
}

static std::string GetABCPath(Node* n)
{
    return GetABCNode(n)->getPath();
}



ABCONode::ABCONode(ABCONode* parent, Abc::OObject* obj, bool create_node)
    : m_obj(obj)
    , m_scene(ABCOScene::getCurrent())
    , m_parent(parent)
{
    if (m_parent)
        m_parent->m_children.push_back(this);
    if (create_node)
        setNode(CreateNode<Node>(parent, obj));
}

ABCONode::~ABCONode()
{
}

void ABCONode::beforeWrite()
{
}

void ABCONode::write(double /*t*/)
{
    const auto& src = *getNode();

    // display name
    if (!m_display_name_prop && !src.display_name.empty() && src.display_name != getName()) {
        m_display_name_prop = Abc::OStringProperty(m_obj->getProperties(), sgabcAttrDisplayName, 1);
        PadSamples(m_display_name_prop, m_scene->getWriteCount(), src.display_name);
    }
    if (m_display_name_prop)
        m_display_name_prop.set(src.display_name);
}

void ABCONode::setNode(Node* node)
{
    m_node = node;
    m_node->impl = this;
}

const std::string& ABCONode::getName() const
{
    return m_obj->getName();
}

const std::string& ABCONode::getPath() const
{
    return m_obj->getFullName();
}


std::string ABCONode::makeUniqueName(const std::string& name) const
{
    return MakeUniqueName(name, [this](const std::string& n) {
        for (auto c : m_children) {
            if (n == c->getName())
                return false;
        }
        return true;
    });
}



ABCORootNode::ABCORootNode(Abc::OObject* obj)
    : super(nullptr, obj, false)
{
    setNode(CreateNode<RootNode>(nullptr, obj));
}


ABCOXformNode::ABCOXformNode(ABCONode* parent, Abc::OObject* obj)
    : super(parent, obj, false)
{
    m_schema = dynamic_cast<AbcGeom::OXform*>(m_obj)->getSchema();
    setNode(CreateNode<XformNode>(parent, obj));

    m_visibility_prop = AbcGeom::CreateVisibilityProperty(*m_obj, 1);
}

void ABCOXformNode::write(double t)
{
    super::write(t);
    const auto& src = *getNode<XformNode>();

    // visibility
    int8_t vis = int8_t(src.visibility ? AbcGeom::kVisibilityVisible : AbcGeom::kVisibilityHidden);
    m_visibility_prop.set(vis);

    // transform
    double4x4 mat;
    mat.assign(src.local_matrix);
    m_sample.setMatrix((abcM44d&)mat);
    m_schema.set(m_sample);
}


ABCOMeshNode::ABCOMeshNode(ABCONode* parent, Abc::OObject* obj)
    : super(parent, obj, false)
{
    m_schema = dynamic_cast<AbcGeom::OPolyMesh*>(m_obj)->getSchema();
    setNode(CreateNode<MeshNode>(parent, obj));

    m_visibility_prop = AbcGeom::CreateVisibilityProperty(*m_obj, 1);
}

void ABCOMeshNode::beforeWrite()
{
    super::beforeWrite();
}

void ABCOMeshNode::write(double t)
{
    super::write(t);
    const auto& src = *getNode<MeshNode>();
    uint32_t wc = (uint32_t)m_schema.getNumSamples();

    // visibility
    int8_t vis = int8_t(src.visibility ? AbcGeom::kVisibilityVisible : AbcGeom::kVisibilityHidden);
    m_visibility_prop.set(vis);

    m_sample.reset();
    m_sample.setFaceIndices(Abc::Int32ArraySample(src.indices.cdata(), src.indices.size()));
    m_sample.setFaceCounts(Abc::Int32ArraySample(src.counts.cdata(), src.counts.size()));
    m_sample.setPositions(Abc::P3fArraySample((const abcV3*)src.points.cdata(), src.points.size()));
    {
        m_normals.setVals(Abc::V3fArraySample((const abcV3*)src.normals.cdata(), src.normals.size()));
        m_sample.setNormals(m_normals);
    }
    {
        m_uvs.setVals(Abc::V2fArraySample((const abcV2*)src.uvs.cdata(), src.uvs.size()));
        m_sample.setUVs(m_uvs);
    }
    m_schema.set(m_sample);

    if(!src.colors.empty()){
        if (!m_rgba_param)
            m_rgba_param = AbcGeom::OC4fGeomParam(m_schema.getArbGeomParams(), sgabcAttrVertexColor, false, AbcGeom::GeometryScope::kFacevaryingScope, 1, 1);

        PadSamples(m_rgba_param, wc);
        m_rgba.setVals(Abc::C4fArraySample((const abcC4*)src.colors.cdata(), src.colors.size()));
        m_rgba_param.set(m_rgba);
    }


    for (auto& fs : src.facesets) {
        auto mat = fs->material;
        if (!mat)
            continue;

        auto& data = m_facesets[mat->id];
        if (!data.faceset) {
            auto ofs = m_schema.createFaceSet(GetABCName(mat));
            data.faceset = ofs.getSchema();
            data.faceset.setTimeSampling(1);

            auto binding = AbcGeom::OStringProperty(data.faceset.getArbGeomParams(), sgabcAttrMaterialBinding, 1);
            binding.set(GetABCPath(mat));
        }

        // first sample of OFaceSet must contain faces. so, add dummy.
        // it will be ignored when import.
        int dummy[] = { 0 };
        data.sample.setFaces(Abc::Int32ArraySample(dummy, 1));
        PadSamples(data.faceset, wc, data.sample);

        data.sample.setFaces(Abc::Int32ArraySample(fs->faces.cdata(), fs->faces.size()));
    }
    for (auto& kvp : m_facesets) {
        auto& data = kvp.second;
        data.faceset.set(data.sample);
        data.sample = {};
    }
}


ABCOMaterialNode::ABCOMaterialNode(ABCONode* parent, Abc::OObject* obj)
    : super(parent, obj, false)
{
    m_schema = dynamic_cast<AbcMaterial::OMaterial*>(m_obj)->getSchema();
    setNode(CreateNode<MaterialNode>(parent, obj));
}

void ABCOMaterialNode::beforeWrite()
{
    super::beforeWrite();
    const auto& src = *getNode<MaterialNode>();

    auto shader_type = ToString(src.shader_type);
    m_schema.setShader(mqabcMaterialTarget, shader_type, src.getName());

    m_shader_params = m_schema.getShaderParameters(mqabcMaterialTarget, shader_type);

    auto create_prop = [this](auto& prop, const char *name) {
        using PropType = std::remove_reference_t<decltype(prop)>;
        prop = PropType(m_shader_params, name, 1);
    };
    create_prop(m_use_vertex_color_prop, sgabcAttrUseVertexColor);
    create_prop(m_double_sided_prop, sgabcAttrDoubleSided);
    create_prop(m_diffuse_color_prop, sgabcAttrDiffuseColor);
    create_prop(m_diffuse_prop, sgabcAttrDiffuse);
    create_prop(m_opacity_prop, sgabcAttrOpacity);
    create_prop(m_roughness_prop, sgabcAttrRoughness);
    create_prop(m_ambient_color_prop, sgabcAttrAmbientColor);
    create_prop(m_specular_color_prop, sgabcAttrSpecularColor);
    create_prop(m_emissive_color_prop, sgabcAttrEmissiveColor);
}

void ABCOMaterialNode::write(double t)
{
    super::write(t);
    if (!m_shader_params)
        return;

    const auto& src = *getNode<MaterialNode>();
    uint32_t wc = (uint32_t)m_diffuse_color_prop.getNumSamples();

    m_use_vertex_color_prop.set(src.use_vertex_color);
    m_double_sided_prop.set(src.double_sided);
    m_diffuse_color_prop.set((abcV3&)src.diffuse_color);
    m_diffuse_prop.set(src.diffuse);
    m_opacity_prop.set(src.opacity);
    m_roughness_prop.set(src.roughness);
    m_ambient_color_prop.set((abcV3&)src.ambient_color);
    m_specular_color_prop.set((abcV3&)src.specular_color);
    m_emissive_color_prop.set((abcV3&)src.emissive_color);

    auto set_texture = [this, wc](auto& prop, const char *prop_name, const TexturePtr& tex) {
        using PropType = std::remove_reference_t<decltype(prop)>;
        if (tex) {
            if (!prop)
                prop = PropType(m_shader_params, prop_name, 1);
            // keep consistent sample count
            PadSamples(prop, wc);
            prop.set(tex->file_path);
        }
    };
    set_texture(m_diffuse_texture_prop, sgabcAttrDiffuseTexture, src.diffuse_texture);
    set_texture(m_opacity_texture_prop, sgabcAttrOpacityTexture, src.opacity_texture);
    set_texture(m_bump_texture_prop, sgabcAttrBumpTexture, src.bump_texture);
}


static thread_local ABCOScene* g_current_scene;

ABCOScene* ABCOScene::getCurrent()
{
    return g_current_scene;
}

ABCOScene::ABCOScene(Scene* scene)
    : m_scene(scene)
{
}

ABCOScene::~ABCOScene()
{
    close();
}

bool ABCOScene::open(const char* /*path*/)
{
    // not supported
    return false;
}

bool ABCOScene::create(const char* path)
{
    try
    {
        m_archive = Abc::OArchive(Alembic::AbcCoreOgawa::WriteArchive(), path);
        m_abc_path = path;
    }
    catch (Alembic::Util::Exception e)
    {
        sgDbgPrint("failed to open %s\nexception: %s", path, e.what());
        return false;
    }

    // add dummy time sampling
    auto tsi = getTimeSampling(0.0);
    g_current_scene = this;
    auto top = std::make_shared<AbcGeom::OObject>(m_archive, AbcGeom::kTop, tsi);
    m_objects.push_back(top);
    m_root = new ABCORootNode(top.get());
    registerNode(m_root);

    sgDbgPrint("succeeded to open %s\nrecording started", m_abc_path.c_str());
    return true;
}

bool ABCOScene::save()
{
    // not supported
    return true;
}

void ABCOScene::close()
{
    if (m_keep_time) {
        auto ts = Abc::TimeSampling(Abc::TimeSamplingType(Abc::TimeSamplingType::kAcyclic), m_timeline);
        *m_archive.getTimeSampling(1) = ts;
    }

    m_root = nullptr;
    m_node_table.clear();
    m_nodes.clear();
    m_objects.clear();
    m_archive.reset();
    m_abc_path.clear();
    m_timeline.clear();
}

void ABCOScene::read()
{
    // not supported
}

void ABCOScene::write()
{
    g_current_scene = this;
    double time = m_scene->time_current;
    if (IsDefaultTime(time))
        time = 0.0;

    for (auto& n : m_nodes) {
        if (n->m_write_count++ == 0)
            n->beforeWrite();
        n->write(time);
    }
    ++m_write_count;

    if (!std::isnan(time)) {
        if (m_keep_time)
            m_timeline.push_back(time);
        if (time > m_max_time)
            m_max_time = time;
    }
}

void ABCOScene::registerNode(ABCONode* n)
{
    if (n) {
        m_nodes.push_back(ABCONodePtr(n));
        m_node_table[n->getPath()] = n;
        m_scene->registerNode(n->m_node);
    }
}

template<class NodeT>
inline ABCONode* ABCOScene::createNodeImpl(ABCONode* parent, const char* name_)
{
    std::string name = EncodeNodeName(name_);
    if (parent)
        name = parent->makeUniqueName(name);

    auto abc = std::make_shared<typename NodeT::AbcType>(*parent->m_obj, name, 1);
    if (abc->valid()) {
        m_objects.push_back(abc);
        return new NodeT(parent, abc.get());
    }
    return nullptr;
}

bool ABCOScene::isNodeTypeSupported(Node::Type type)
{
    switch (type) {
    case Node::Type::Mesh:     // 
    case Node::Type::Xform:    // 
    case Node::Type::Material: // fall through
        return true;
    default:
        return false;
    }
}

Node* ABCOScene::createNode(Node* parent_, const char* name, Node::Type type)
{
    auto parent = parent_ ? (ABCONode*)parent_->impl : nullptr;
    ABCONode* ret = nullptr;
    switch (type) {
    case Node::Type::Mesh: ret = createNodeImpl<ABCOMeshNode>(parent, name); break;
    case Node::Type::Xform: ret = createNodeImpl<ABCOXformNode>(parent, name); break;
    case Node::Type::Material: ret = createNodeImpl<ABCOMaterialNode>(parent, name); break;
    default: ret = createNodeImpl<ABCONode>(parent, name); break;
    }

#ifdef mqusdDebug
    sgDbgPrint("%s\n", ret->getPath().c_str());
#endif
    registerNode(ret);
    return ret ? ret->m_node : nullptr;
}

bool ABCOScene::wrapNode(Node* /*node*/)
{
    // not supported
    return false;
}

double ABCOScene::frameToTime(int frame)
{
    return (1.0 / m_scene->frame_rate) * frame + m_scene->time_start;
}

Scene* ABCOScene::getHostScene()
{
    return m_scene;
}

uint32_t ABCOScene::getWriteCount() const
{
    return m_write_count;
}

uint32_t ABCOScene::getTimeSampling(double start_time)
{
    // in most cases what to return is the last element. so, search from back to front.
    // (0th element is reserved and not considered)
    uint32_t n = (uint32_t)m_archive.getNumTimeSamplings();
    for (uint32_t i = n - 1; i >= 1; --i) {
        auto ts = m_archive.getTimeSampling(i);
        double t = ts->getStoredTimes().front();
        if (t == start_time)
            return i;
        else if (start_time > t)
            break;
    }

    // not found. add new time sampling.
    return m_archive.addTimeSampling(Abc::TimeSampling(1.0 / m_scene->frame_rate, start_time));
}

ABCONode* ABCOScene::findABCNodeImpl(const std::string& path)
{
    auto it = m_node_table.find(path);
    return it == m_node_table.end() ? nullptr : it->second;
}

Node* ABCOScene::findNodeImpl(const std::string& path)
{
    if (ABCONode* n = findABCNodeImpl(path))
        return n->m_node;
    return nullptr;
}


ScenePtr CreateABCOScene()
{
    auto ret = new Scene();
    ret->impl.reset(new ABCOScene(ret));
    return ScenePtr(ret);
}

} // namespace sg
