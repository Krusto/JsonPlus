#pragma once

#include <cstdio>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <unordered_set>
#include <filesystem>
#include <expected>
#include <optional>

namespace JsonPlus
{

    template <typename T = nlohmann::ordered_json>
    inline std::expected<T, std::string> _LoadJsonFile(const std::filesystem::path& path)
    {
        std::filesystem::path full_path = path;
        if (path.is_relative()) { full_path = std::filesystem::current_path() / std::filesystem::relative(path); }

        if (!std::filesystem::exists(full_path)) { return std::unexpected("File not found: " + full_path.string()); }

        std::ifstream file(full_path);
        if (!file.is_open()) { return std::unexpected("Failed to open: " + path.string()); }

        try
        {
            T j;
            file >> j;
            return j;
        }
        catch (const std::exception& e)
        {
            return std::unexpected("Error parsing " + path.string() + ": " + e.what());
        }
    }

    template <typename T = nlohmann::ordered_json>
    inline void DeepMerge(T& dst, const T& src)
    {
        for (auto& [key, value]: src.items())
        {
            if (dst.contains(key) && dst[key].is_object() && value.is_object()) { DeepMerge(dst[key], value); }
            else
            {
                dst[key] = value;
            }
        }
    }

    template <typename T = nlohmann::ordered_json>
    inline std::optional<std::string> Propagate(const std::expected<T, std::string>& v, T& out)
    {
        if (v.has_value()) { out = v.value(); }
        else
        {
            return v.error();
        }
        return std::nullopt;
    }

    template <typename T = nlohmann::ordered_json>
    inline std::optional<std::string> ResolveIncludes(T& node, const std::filesystem::path& basePath,
                                                      std::unordered_set<std::string>& loaded)
    {
        // ---- object ----
        if (node.is_object())
        {
            while (true)
            {
                bool didInclude = false;

                // -------- include_in_place --------
                if (node.contains("include_in_place"))
                {
                    auto entry = node["include_in_place"];
                    node.erase("include_in_place");

                    std::vector<std::string> includes;
                    if (entry.is_string()) includes.push_back(entry.template get<std::string>());
                    else if (entry.is_array())
                        for (auto& v: entry) includes.push_back(v.template get<std::string>());
                    else
                        return "include_in_place must be string or array";

                    for (const auto& inc: includes)
                    {
                        std::filesystem::path p = basePath / inc;

                        std::error_code ec;
                        auto canon = std::filesystem::canonical(p, ec);
                        if (ec) return "Invalid include path: " + p.string();

                        if (!loaded.insert(canon.string()).second)
                            return "Circular include detected: " + canon.string();

                        T included;
                        auto raw = _LoadJsonFile<T>(canon);
                        if (auto err = Propagate<T>(raw, included)) return err;

                        if (auto err = ResolveIncludes(included, canon.parent_path(), loaded)) return err;

                        if (included.is_array())
                        {
                            if (node.empty()) node = T::array();
                            if (!node.is_array()) return "Cannot merge an array include with object fields";
                            for (auto& value: included) node.push_back(std::move(value));
                        }
                        else
                        {
                            if (node.is_array()) return "Cannot merge an object include with an array";
                            DeepMerge(node, included);
                        }
                    }

                    didInclude = true;
                }

                // -------- include --------
                if (node.contains("include"))
                {
                    auto entry = node["include"];
                    node.erase("include");

                    std::vector<std::string> includes;
                    if (entry.is_string()) includes.push_back(entry.template get<std::string>());
                    else if (entry.is_array())
                        for (auto& v: entry) includes.push_back(v.template get<std::string>());
                    else
                        return "include must be string or array";

                    for (const auto& inc: includes)
                    {
                        std::filesystem::path p = basePath / inc;

                        std::error_code ec;
                        auto canon = std::filesystem::canonical(p, ec);
                        if (ec) return "Invalid include path: " + p.string();

                        if (!loaded.insert(canon.string()).second)
                            return "Circular include detected: " + canon.string();

                        T included;
                        auto raw = _LoadJsonFile<T>(canon);
                        if (auto err = Propagate<T>(raw, included)) return err;

                        if (auto err = ResolveIncludes<T>(included, canon.parent_path(), loaded)) return err;

                        node[canon.stem().string()] = included;
                    }

                    didInclude = true;
                }

                if (!didInclude) break;
            }

            // recurse into children
            for (auto& [_, v]: node.items())
            {
                if (auto err = ResolveIncludes<T>(v, basePath, loaded)) return err;
            }
        }
        // ---- array ----
        else if (node.is_array())
        {
            T resolved = T::array();
            for (auto& element: node)
            {
                const bool splice = element.is_object() && element.size() == 1 && element.contains("include_in_place");
                if (auto err = ResolveIncludes<T>(element, basePath, loaded)) return err;
                if (splice && element.is_array())
                {
                    for (auto& value: element) resolved.push_back(std::move(value));
                }
                else
                {
                    resolved.push_back(std::move(element));
                }
            }
            node = std::move(resolved);
        }

        return std::nullopt;
    }

    template <typename T = nlohmann::ordered_json>
    inline std::expected<T, std::string> _LoadJsonFile(const std::filesystem::path& path,
                                                       std::unordered_set<std::string>& loaded)
    {
        std::filesystem::path file_path = path.is_relative() ? std::filesystem::current_path() / path : path;

        if (!std::filesystem::exists(file_path)) return std::unexpected("File not found: " + file_path.string());

        std::string canonical = std::filesystem::canonical(file_path).string();
        if (loaded.contains(canonical)) return std::unexpected("Circular include detected: " + canonical);

        loaded.insert(canonical);

        T result;
        try
        {
            auto loadResult = _LoadJsonFile<T>(file_path);
            if (loadResult.has_value()) { result = loadResult.value(); }
            else
            {
                return std::unexpected(loadResult.error());
            }

            auto includeResult = ResolveIncludes<T>(result, file_path.parent_path(), loaded);

            if (includeResult.has_value()) { return std::unexpected(includeResult.value()); }

            return result;
        }
        catch (const std::exception& e)
        {
            return std::unexpected(std::string("JSON error in ") + file_path.string() + ": " + e.what());
        }
    }

    inline std::expected<nlohmann::ordered_json, std::string> Load(const std::filesystem::path& path,
                                                                   bool debugConsoleDump = false)
    {
        std::unordered_set<std::string> loaded;

        auto result = _LoadJsonFile<>(path, loaded);

        if (debugConsoleDump && result.has_value()) { std::cout << result.value().dump(4) << std::endl; }


        return result;
    }

    inline std::expected<nlohmann::json, std::string> LoadUnordered(const std::filesystem::path& path,
                                                                    bool debugConsoleDump = false)
    {
        std::unordered_set<std::string> loaded;

        auto result = _LoadJsonFile<nlohmann::json>(path, loaded);

        if (debugConsoleDump && result.has_value()) { std::cout << result.value().dump(4) << std::endl; }

        return result;
    }

}// namespace JsonPlus
