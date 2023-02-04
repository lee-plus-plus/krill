#include "krill/attrdict.h"
#include "krill/utils.h"
#include "fmt/format.h"

#define DECLARE_TYPE_REGISTERY(T)                                              \
    {                                                                          \
        std::type_index(typeid(T)), [](const std::any &v) {                    \
            return krill::utils::to_string<T>(any_cast<T>(v));                 \
        }                                                                      \
    }


namespace krill::type {

map<std::type_index, std::function<std::string(const std::any &v)>>
    type_registery{
        DECLARE_TYPE_REGISTERY(int),    DECLARE_TYPE_REGISTERY(float),
        DECLARE_TYPE_REGISTERY(double), DECLARE_TYPE_REGISTERY(size_t),
        DECLARE_TYPE_REGISTERY(char),   DECLARE_TYPE_REGISTERY(std::string),
        DECLARE_TYPE_REGISTERY(long),
    };


map<string, string> AttrDict::ToStrDict() const {
    map<string, string> retval;
    for (const auto & [ k, a ] : attr_dict_) {
        auto tid = std::type_index(a.type());
        if (const auto it = type_registery.find(tid);
            it != type_registery.end()) {
            // str => "str"
            if (tid == std::type_index(typeid(std::string))) {
                string s = "\"";
                s += krill::utils::unescape(any_cast<string>(a));
                s += "\"";
                retval.emplace(k, s);
            } else {
                retval.emplace(k, it->second(a));
            }
        } else {
            // unsupport type
            // retval.emplace(k, tid.name());
            retval.emplace(k, "?");
        }
    }
    return retval;
}

string AttrDict::str() const {
    return "AttrDict{" + utils::ToString{}(ToStrDict()) + "}";
}

string AttrDict::str2() const {
    std::stringstream ss;
    for (const auto &[k, a] : ToStrDict()) {
        ss << fmt::format(".{}={} ", k, a);
    }
    return ss.str();
}

} // namespace krill::utils
