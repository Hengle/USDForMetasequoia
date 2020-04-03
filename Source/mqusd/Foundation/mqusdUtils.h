#pragma once
#include "mqusd.h"

namespace mqusd {

#ifdef  MQPluginH

inline float3 to_float3(const MQColor& v) { return (float3&)v; }
inline float3 to_float3(const MQPoint& v) { return (float3&)v; }
inline float4x4 to_float4x4(const MQMatrix& v) { return (float4x4&)v; }
inline MQColor to_color(const float3& v) { return (MQColor&)v; }
inline MQPoint to_point(const float3& v) { return (MQPoint&)v; }

inline float3 to_eular(const MQAngle& ang, bool flip_head = false)
{
    auto ret = flip_head ?
        float3{ ang.pitch, -ang.head + 180.0f,  ang.bank } :
        float3{ ang.pitch, ang.head, ang.bank };
    return ret * mu::DegToRad;
}
inline quatf to_quat(const MQAngle& ang)
{
    return mu::rotate_zxy(to_eular(ang));
}
inline MQAngle to_angle(const quatf& q)
{
    auto r = mu::to_euler_zxy(q);
    r = float3{ r.y, r.x, r.z } * mu::RadToDeg;
    return (MQAngle&)r;
}


namespace detail {

template<class Body, class R>
struct each_object
{
    void operator()(MQDocument doc, const Body& body)
    {
        int n = doc->GetObjectCount();
        for (int i = 0; i < n; ++i) {
            if (auto obj = doc->GetObject(i))
                body(obj);
        }
    }
};

template<class Body>
struct each_object<Body, bool>
{
    void operator()(MQDocument doc, const Body& body)
    {
        int n = doc->GetObjectCount();
        for (int i = 0; i < n; ++i) {
            if (auto obj = doc->GetObject(i)) {
                if (!body(obj))
                    break;
            }
        }
    }
};

} // namespace detail

template<class Body>
inline void each_object(MQDocument doc, const Body& body)
{
    detail::each_object<Body, decltype(body(nullptr))>()(doc, body);
}

inline std::string GetName(MQObject obj)
{
    char buf[256] = "";
    obj->GetName(buf, sizeof(buf));
    return buf;
}

inline std::string GetPath(MQDocument doc, MQObject obj)
{
    std::string ret;
    if (auto parent = doc->GetParentObject(obj))
        ret += GetPath(doc, parent);
    ret += '/';

    char buf[256] = "";
    obj->GetName(buf, sizeof(buf));
    ret += buf;
    return ret;
}

inline std::string GetName(MQMaterial obj)
{
    char buf[256] = "";
    obj->GetName(buf, sizeof(buf));
    return buf;
}

#endif // MQPluginH


template<class T>
inline bool is_uniform(const T* data, size_t data_size, size_t element_size)
{
    auto* base = data;
    size_t n = data_size / element_size;
    for (size_t di = 1; di < n; ++di) {
        data += element_size;
        for (size_t ei = 0; ei < element_size; ++ei) {
            if (data[ei] != base[ei])
                return false;
        }
    }
    return true;
}
template<class Container, class Body>
inline bool is_uniform(const Container& container, const Body& body)
{
    for (auto& e : container) {
        if (!body(e))
            return false;
    }
    return true;
}
template<class T>
inline void expand_uniform(T* dst, size_t data_size, const T* src, size_t element_size)
{
    size_t n = data_size / element_size;
    for (size_t di = 0; di < n; ++di) {
        for (size_t ei = 0; ei < element_size; ++ei)
            dst[ei] = src[ei];
        dst += element_size;
    }
}

template<class DstContainer, class SrcContainer, class Convert>
inline void transform_container(DstContainer& dst, const SrcContainer& src, const Convert& c)
{
    size_t n = src.size();
    dst.resize(n);
    for (size_t i = 0; i < n; ++i)
        c(dst[i], src[i]);
}

template<class Container, class Body>
inline void each_with_index(Container& dst, const Body& body)
{
    int i = 0;
    for (auto& v : dst)
        body(v, i++);
}

template<class Container, class Condition>
inline void erase_if(Container& dst, const Condition& cond)
{
    dst.erase(
        std::remove_if(dst.begin(), dst.end(), cond),
        dst.end());
}

template<class Container>
inline auto append(Container& dst, const Container& src)
{
    size_t pos = dst.size();
    if (dst.empty())
        dst = src; // share if Container is SharedVector
    else
        dst.insert(dst.end(), src.begin(), src.end());
    return dst.begin() + pos;
}

template<class T>
inline void add(T* dst, size_t size, T v)
{
    for (size_t i = 0; i < size; ++i)
        *(dst++) += v;
}

std::string SanitizeNodeName(const std::string& name);
std::string SanitizeNodePath(const std::string& path);
std::string GetParentPath(const std::string& path);
const char* GetLeafName(const std::string& path);


} // namespace mqusd
