// split_profile.cpp
// 把 ClassIsland 档案文件（Profile）按顶层字段拆分成三部分：
//   subjects.json    —— 保留 Subjects
//   timelayouts.json —— 保留 TimeLayouts
//   classplans.json  —— 保留 ClassPlans / ClassPlanGroups
// 其余顶层字段原样保留，其它集合字段输出为空对象 {}。
//
// 用法: split_profile.exe <档案.json> [输出目录]
// 编译: cl /utf-8 /EHsc /std:c++17 /O2 /Fe:split_profile.exe split_profile.cpp
//       （/utf-8 必需：源文件含中文注释）

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

struct Field {
    std::string key;
    size_t vs;          // 值起始（已跳过前导空白）
    size_t ve;          // 值结束（不含分隔符/换行）
};

static void skipWs(const std::string& s, size_t& i) {
    while (i < s.size() && (unsigned char)s[i] <= ' ') ++i;
}

// 扫描一个 JSON 值，返回结束位置（不含尾随分隔符）
static size_t scanValue(const std::string& s, size_t i) {
    skipWs(s, i);
    if (i >= s.size()) return i;
    char c = s[i];
    if (c == '"') {                       // 字符串
        ++i;
        while (i < s.size()) {
            if (s[i] == '\\') { i += 2; continue; }
            if (s[i] == '"') { ++i; break; }
            ++i;
        }
        return i;
    }
    if (c == '{' || c == '[') {           // 对象 / 数组
        int depth = 0;
        bool inStr = false;
        for (; i < s.size(); ++i) {
            char ch = s[i];
            if (inStr) {
                if (ch == '\\') { ++i; continue; }
                if (ch == '"') inStr = false;
                continue;
            }
            if (ch == '"') { inStr = true; continue; }
            if (ch == '{' || ch == '[') ++depth;
            else if (ch == '}' || ch == ']') { if (--depth == 0) { ++i; break; } }
        }
        return i;
    }
    while (i < s.size() && s[i] != ',' && s[i] != '}' && s[i] != '\n' && s[i] != '\r') ++i;
    return i;
}

// 解析顶层对象的所有键值对（仅切分第一层，值保留原始文本）
static std::vector<Field> parseTopLevel(const std::string& s) {
    std::vector<Field> out;
    size_t i = 0;
    if (s.size() >= 3 && (unsigned char)s[0] == 0xEF &&
        (unsigned char)s[1] == 0xBB && (unsigned char)s[2] == 0xBF) i = 3;   // 跳过 BOM
    skipWs(s, i);
    if (i >= s.size() || s[i] != '{') return out;
    ++i;
    while (i < s.size()) {
        skipWs(s, i);
        if (i >= s.size() || s[i] == '}') break;
        if (s[i] == ',') { ++i; continue; }
        if (s[i] != '"') { ++i; continue; }
        size_t ks = i;
        ++i;
        while (i < s.size()) {            // 键名
            if (s[i] == '\\') { i += 2; continue; }
            if (s[i] == '"') { ++i; break; }
            ++i;
        }
        std::string key = s.substr(ks + 1, i - ks - 2);
        skipWs(s, i);
        if (i < s.size() && s[i] == ':') ++i;
        size_t vs = i;
        skipWs(s, vs);
        size_t ve = scanValue(s, i);
        out.push_back({ key, vs, ve });
        i = ve;
        skipWs(s, i);
        if (i < s.size() && s[i] == ',') ++i;
    }
    return out;
}

static bool isCollection(const std::string& k) {
    return k == "TimeLayouts" || k == "ClassPlans" || k == "Subjects" ||
           k == "ClassPlanGroups" || k == "OrderedSchedules";
}

static bool contains(const std::vector<std::string>& v, const std::string& k) {
    for (const auto& x : v) if (x == k) return true;
    return false;
}

// 统计值文本中的一级条目数（用于运行提示）
static size_t countEntries(const std::string& v) {
    size_t n = 0;
    if (v.empty() || v[0] != '{') return 0;
    int depth = 0;
    bool inStr = false;
    for (size_t i = 0; i < v.size(); ++i) {
        char ch = v[i];
        if (inStr) {
            if (ch == '\\') { ++i; continue; }
            if (ch == '"') inStr = false;
            continue;
        }
        if (ch == '"') { inStr = true; continue; }
        if (ch == '{' || ch == '[') { if (++depth == 2) ++n; }   // 一级条目
        else if (ch == '}' || ch == ']') --depth;
    }
    return n;
}

static bool writePart(const std::string& src, const std::vector<Field>& fields,
                      const std::string& path, const std::vector<std::string>& keep) {
    FILE* f = fopen(path.c_str(), "wb");
    if (!f) { fprintf(stderr, "  [fail] cannot write %s\n", path.c_str()); return false; }
    fputs("{\n", f);
    size_t keptBytes = 0, entries = 0;
    for (size_t n = 0; n < fields.size(); ++n) {
        const Field& fd = fields[n];
        std::string val;
        if (contains(keep, fd.key)) {
            val = src.substr(fd.vs, fd.ve - fd.vs);
            keptBytes += val.size();
            entries += countEntries(val);
        } else if (isCollection(fd.key)) {
            val = "{}";
        } else {
            val = src.substr(fd.vs, fd.ve - fd.vs);
        }
        fprintf(f, "    \"%s\": %s", fd.key.c_str(), val.c_str());
        if (n + 1 < fields.size()) fputc(',', f);
        fputc('\n', f);
    }
    fputs("}\n", f);
    fclose(f);
    printf("  [ok] %-18s fields=%zu  kept=%zu bytes  entries=%zu\n",
           fs::path(path).filename().u8string().c_str(),
           fields.size(), keptBytes, entries);
    return true;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <profile.json> [output_dir]\n", argv[0]);
        return 1;
    }
    std::string inPath = argv[1];
    std::string outDir = (argc >= 3) ? argv[2] : ".";

    FILE* f = fopen(inPath.c_str(), "rb");
    if (!f) { fprintf(stderr, "cannot open: %s\n", inPath.c_str()); return 1; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    std::string src((size_t)sz, '\0');
    size_t rd = fread(&src[0], 1, (size_t)sz, f);
    fclose(f);
    src.resize(rd);

    std::vector<Field> fields = parseTopLevel(src);
    if (fields.empty()) { fprintf(stderr, "not a json object or empty: %s\n", inPath.c_str()); return 1; }

    std::error_code ec;
    fs::create_directories(outDir, ec);

    printf("input : %s (%zu bytes, %zu top-level fields)\n", inPath.c_str(), src.size(), fields.size());
    bool ok = true;
    ok &= writePart(src, fields, (fs::path(outDir) / "subjects.json").string(),
                    { "Subjects" });
    ok &= writePart(src, fields, (fs::path(outDir) / "timelayouts.json").string(),
                    { "TimeLayouts" });
    ok &= writePart(src, fields, (fs::path(outDir) / "classplans.json").string(),
                    { "ClassPlans", "ClassPlanGroups" });
    printf("%s -> %s\n", ok ? "done" : "error", outDir.c_str());
    return ok ? 0 : 1;
}
