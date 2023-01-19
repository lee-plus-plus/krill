#ifndef UTILS_H
#define UTILS_H
#include "magic_enum.hpp"
#include <algorithm>
#include <cstring>
#include <functional>
#include <map>
#include <numeric>
#include <ostream>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

namespace krill::utils {

using magic_enum::enum_name; // NOLINT

template <typename T> inline std::string to_string(const T &v) {
    std::stringstream ss;
    ss << v;
    return ss.str();
}

inline void replace_all(std::string &str, const std::string &from,
                        const std::string &to) {
    for (std::string::size_type pos = str.find(from); pos != std::string::npos;
         pos                        = str.find(from)) {
        str.replace(pos, from.length(), to);
    }
}

inline std::string unescape(std::string str) {
    replace_all(str, "\t", "\\t");
    replace_all(str, "\r", "\\r");
    replace_all(str, "\n", "\\n");
    replace_all(str, "\v", "\\v");
    replace_all(str, "\a", "\\a");
    return str;
}

inline std::vector<std::string> split(std::string str,
                                      const char *delim = " \t\r\n\0") {
    std::vector<std::string> res;
    char                    *strc = &str[0];
    char                    *temp = strtok(strc, delim);
    while (temp != NULL) {
        res.push_back(std::string(temp));
        temp = strtok(NULL, delim);
    }
    return res;
}

inline void ltrim(std::string &str, const std::string &val = " \r\n\0") {
    str.erase(0, str.find_first_not_of(val));
}

inline void rtrim(std::string &str, const std::string &val = " \r\n\0") {
    str.erase(str.find_last_not_of(val) + 1);
}

inline void trim(std::string &str, const std::string &val = " \r\n\0") {
    ltrim(str, val);
    rtrim(str, val);
}

inline std::string trimmed(std::string        str,
                           const std::string &val = " \r\n\0") {
    trim(str, val);
    return str;
}

template <typename T> inline std::vector<T> to_vector(std::stack<T> s) {
    std::vector<T> v;
    while (s.size() > 0) {
        v.insert(v.begin(), s.top());
        s.pop();
    }
    return v;
}

template <typename T> inline std::vector<T> to_vector(std::set<T> s) {
    std::vector<T> v;
    v.assign(s.begin(), s.end());
    return v;
}

template <typename T1, typename T2>
inline std::vector<std::pair<T1, T2>> to_vector(std::map<T1, T2> m) {
    std::vector<std::pair<T1, T2>> v;
    v.assign(m.begin(), m.end());
    return v;
}

template <typename T> inline std::set<T> to_set(std::vector<T> v) {
    return std::set<T>(v.begin(), v.end());
}

template <typename T1, typename T2>
inline std::vector<T2> apply_map(std::vector<T1> v, const std::map<T1, T2> &m) {
    std::vector<T2> r;
    transform(v.begin(), v.end(), back_inserter(r),
              [&m](const T1 &x) { return m.at(x); });
    return r;
}

template <typename T, typename F>
inline auto apply_map(std::vector<T> v, F func) {
#ifndef __cplusplus
#error "Not cpp error."
#else
#if __cplusplus < 201703L
    std::vector<typename std::result_of<F(T &)>::type> r;
#else
    std::vector<typename std::invoke_result<F, T&>::type> r;
#endif
#endif
    transform(v.begin(), v.end(), back_inserter(r), func);
    return r;
}

template <typename Container, typename R, typename F>
inline R apply_reduce(Container v, R init, F func) {
    return accumulate(v.begin(), v.end(), init, func);
}

template <typename T, typename F>
inline std::vector<T> apply_filter(const std::vector<T> &v, F func) {
    std::vector<T> r;
    copy_if(v.begin(), v.end(), back_inserter(r), func);
    return r;
}

template <typename T>
inline std::vector<T> slice(const std::vector<T> &v, int st, int ed) {
    int size = static_cast<int>(v.size());
    if (st < 0) { st = size + st; }
    if (ed < 0) { ed = size + size; }
    if (st >= size || ed < 0 || st >= ed) {
        return std::vector<T>();
    } else {
        if (st < 0) { st = 0; }
        if (ed > size) { ed = size; }
        return std::vector<T>(v.begin() + st, v.begin() + ed);
    }
}

inline std::string slice(const std::string &s, int st, int ed) {
    int size = static_cast<int>(s.size());
    if (st < 0) { st = size + st; }
    if (ed < 0) { ed = size + size; }
    if (st >= size || ed < 0 || st >= ed) {
        return std::string("");
    } else {
        if (st < 0) { st = 0; }
        if (ed > size) { ed = size; }
        return s.substr(st, ed - st);
    }
}

template <typename T> inline T slice(const T &s, int st) {
    return slice(s, st, s.size());
}

template <typename T1, typename T2>
inline std::map<T2, T1> reverse(std::map<T1, T2> m) {
    std::map<T2, T1> m_reversed;
    for (auto [key, value] : m) { m_reversed[value] = key; }
    return m_reversed;
}

template <typename T> inline std::vector<T> reverse(std::vector<T> m) {
    std::vector<T> m_reversed;
    std::reverse_copy(m.begin(), m.end(), back_inserter(m_reversed));
    return m_reversed;
}

template <typename T> inline std::vector<T> get_top(std::stack<T> s, size_t size) {
    assert(size >= 0);
    std::vector<T> v(size);
    for (size_t i = 0; i < size; i++) {
        v[i] = s.top();
        s.pop();
    }
    return v;
}

struct pair_hash {
    template <class T1, class T2>
    size_t operator()(std::pair<T1, T2> const &p) const {
        size_t h1 = std::hash<T1>()(p.first);
        size_t h2 = std::hash<T2>()(p.second);
        return h1 ^ (h2 << 1);
    }
};

inline unsigned bit_count(unsigned v) {
    unsigned int c;
    for (c = 0; v; c++) { v &= v - 1; }
    return c;
}

inline unsigned bit_high_pos(unsigned v) {
    assert(v != 0);
    unsigned int c;
    for (c = -1; v; c++) { v = v >> 1; }
    return c;
}

template <class> inline constexpr bool is_vector = false;
template <class T, class A>
inline constexpr bool is_vector<std::vector<T, A>> = true;

template <class> inline constexpr bool            is_set = false;
template <class T, class A> inline constexpr bool is_set<std::set<T, A>> = true;

template <typename T> class Appender {
  public:
    T &v_;
    Appender(T &v) : v_(v) {
        static_assert(is_vector<T> || is_set<T>, "not support yet");
    }
    inline Appender &append(const T &v) {
        if constexpr (is_vector<T>) {
            v_.insert(v_.end(), v.begin(), v.end());
        } else if constexpr (is_set<T>) {
            v_.insert(v.begin(), v.end());
        }
        return *this;
    }
};

template <typename T> class SetOprter {
  public:
    T s_;
    inline SetOprter(const T &s = {}) : s_(s) {
        static_assert(is_set<T>, "must be std::set");
    }
    inline SetOprter operator|(const T &s) {
        T r;
        set_union(s_.begin(), s_.end(), s.begin(), s.end(),
                  inserter(r, r.begin()));
        s_ = std::move(r);
        return *this;
    }
    inline SetOprter operator&(const T &s) {
        T r;
        set_intersection(s_.begin(), s_.end(), s.begin(), s.end(),
                         inserter(r, r.begin()));
        s_ = std::move(r);
        return *this;
    }
    inline SetOprter operator-(const T &s) {
        T r;
        set_difference(s_.begin(), s_.end(), s.begin(), s.end(),
                       inserter(r, r.begin()));
        s_ = std::move(r);
        return *this;
    }
    inline operator T() const { return s_; }
};

// Jerry Yang's magic,
// don't touch!
struct ToString {
    std::stringstream ss;
    using type = std::string;
    template <typename T>
    typename std::enable_if<!std::is_class<T>::value, type>::type
    operator()(const T &v) {
        ss << v;
        return ss.str();
    }

  private:
    type __reduce(const type &h) const { return h; }
    type __combine(const type &l, const type &r) const { return l + "," + r; }
    template <typename... SZ>
    type __reduce(const type &h, const SZ &...args) const {
        return __combine(h, __reduce(args...));
    }

    template <typename T> type comb(const T &t) const { return ToString{}(t); }

    template <typename T, typename... Args>
    type comb(const T &t, const Args &...args) const {
        return __reduce(ToString{}(t), comb(args...));
    }

  public:
    template <typename T1, typename T2, typename... Args>
    type operator()(const T1 &t1, const T2 &t2, const Args &...args) {
        return "<" + comb(t1, t2, args...) + ">";
    }

    template <typename L, typename R>
    type operator()(const std::pair<L, R> p) const {
        return "<" + comb(p.first, p.second) + ">";
    }

    template <typename I, typename C = typename I::const_iterator>
    type operator()(const I &it) const {
        std::string hv = "";
        for (const auto &i : it) { hv = hv + ToString{}(i) + ","; }
        return "[" + hv + "]";
    }

    type operator()(const std::string &it) const {
        // return "\"" + unescape(it) + "\"";
        return unescape(it);
    }

    type operator()(const std::vector<bool> &bs) const {
        std::string hv = "";
        for (size_t i = 0; i < bs.size(); ++i) {
            hv = ToString{}((int) bs[i]) + ", ";
        }
        return "[" + hv + "]";
    }

    template <typename... Args>
    size_t operator()(const std::tuple<Args...> &tup) {
        return apply(
            [](Args... v) -> size_t { return ToString{}.comb<Args...>(v...); },
            tup);
    }
    template <
        typename ToStringable,
        typename = std::is_same<
            decltype(std::declval<ToStringable>().ToString()), std::string>>
    std::string operator()(const ToStringable &v) {
        return v.ToString();
    }
};

} // namespace krill::utils
#endif
