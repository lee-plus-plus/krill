#include "krill/AttrDict.h"
#include "krill/utils.h"

#define DECLARE_TYPE_REGISTERY(T)                                              \
    {                                                                          \
        std::type_index(typeid(T)), [](const std::any &v) {                    \
            return krill::utils::to_string<T>(any_cast<T>(v));                 \
        }                                                                      \
    }


namespace krill::utils {

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
            retval.emplace(k, it->second(a));
        } else {
            retval.emplace(k, tid.name());
        }
    }
    return move(retval);
}

string AttrDict::str() const {
    return "AttrDict{" + utils::ToString{}(ToStrDict()) + "}";
}


} // namespace krill::utils
