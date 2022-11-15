#ifndef UTILS_H
#define UTILS_H
#include "defs.h"
#include <cstring>
#include <map>
#include <ostream>
#include <set>
#include <tuple>
#include <sstream>
#include <string>
#include <vector>
#include <functional>
using std::ostream;
using std::pair;
using std::string;
using std::vector, std::set, std::tuple, std::is_same;
// using namespace std;
// using namespace krill::automata;
// using namespace krill::grammar;

namespace krill::utils {

template <typename T> inline std::string to_string(const T &v) {
    std::stringstream ss;
    ss << v;
    return ss.str();
}

inline std::vector<std::string> split(std::string str, const char *delim) {
    std::vector<std::string> res;
    char *                   strc = &str[0];
    char *                   temp = strtok(strc, delim);
    while (temp != NULL) {
        res.push_back(std::string(temp));
        temp = strtok(NULL, delim);
    }
    return res;
}

inline void ltrim(std::string &str, const string &val = " \r\n\0") {
    str.erase(0, str.find_first_not_of(val));
}

inline void rtrim(std::string &str, const string &val = " \r\n\0") {
    str.erase(str.find_last_not_of(val) + 1);
}

inline void trim(std::string &str, const string &val = " \r\n\0") {
    ltrim(str, val);
    rtrim(str, val);
}

template <typename T1, typename T2>
inline std::map<T2, T1> reverse(std::map<T1, T2> m) {
    std::map<T2, T1> m_reversed;
    for (auto[key, value] : m) { m_reversed[value] = key; }
    return m_reversed;
}

// Jerry Yang's magic, 
// don't touch!
struct ToString {
    std::stringstream ss;
    using type = std::string;
    template <typename T>
    typename std::enable_if<!std::is_class<T>::value, type>::type operator()(const T &v) {
        ss << v;
        return ss.str();
    }

  private:
    type __reduce(const type &h) const { return h; }
    type __combine(const type &l, const type &r) const { return l + "," + r; }
    template <typename... SZ>
    type __reduce(const type &h, const SZ &... args) const {
        return __combine(h, __reduce(args...));
    }

    template <typename T> type comb(const T &t) const { return ToString{}(t); }

    template <typename T, typename... Args>
    type comb(const T &t, const Args &... args) const {
        return __reduce(ToString{}(t), comb(args...));
    }

  public:
    template <typename T1, typename T2, typename... Args>
    type operator()(const T1 &t1, const T2 &t2, const Args &... args) {
        return "<" + comb(t1, t2, args...) + ">";
    }

    template <typename L, typename R>
    type operator()(const pair<L, R> p) const {
        return "<" + comb(p.first, p.second) + ">";
    }

    template <typename I, typename C = typename I::const_iterator>
    type operator()(const I &it) const {
        string hv = "";
        for (const auto &i : it) { hv = hv + ToString{}(i) + ","; }
        return "[" + hv + "]";
    }

    type operator()(const std::vector<bool> &bs) const {
        string hv = "";
        for (int i = 0; i < bs.size(); ++i) {
            hv = ToString{}((int) bs[i]) + ",";
        }
        return "[" + hv + "]";
    }

    template <typename... Args> size_t operator()(const tuple<Args...> &tup) {
        return apply(
            [](Args... v) -> size_t { return ToString{}.comb<Args...>(v...); },
            tup);
    }
    template <typename ToStringable,
              typename = is_same<
                  decltype(std::declval<ToStringable>().ToString()), string>>
    string operator()(const ToStringable &v) {
        return v.ToString();
    }
};

} // namespace krill::utils
#endif