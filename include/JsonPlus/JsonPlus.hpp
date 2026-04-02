#pragma once

#include <cstdio>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <unordered_set>
#include <filesystem>
#include <variant>

namespace JsonPlus
{

    inline nlohmann::ordered_json _LoadJsonFile(const std::filesystem::path& path)
    {
        std::filesystem::path full_path = path;
        if (path.is_relative()) { full_path = std::filesystem::current_path() / std::filesystem::relative(path); }

        if (!std::filesystem::exists(full_path))
        {
            std::cout << "File not found: " << full_path.string() << "\n";
            return nlohmann::ordered_json{};
        }

        std::ifstream file(full_path);
        if (!file.is_open())
        {
            std::cout << "Failed to open: " << path << "\n";
            return nlohmann::ordered_json{};
        }

        try
        {
            nlohmann::ordered_json j;
            file >> j;
            return j;
        }
        catch (const std::exception& e)
        {
            std::cout << "Error parsing " << path << ": " << e.what() << "\n";
            return nlohmann::ordered_json{};
        }
    }

    inline void DeepMerge(nlohmann::ordered_json& dst, const nlohmann::ordered_json& src)
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

    inline std::optional<std::string> Propagate(const std::variant<std::string, nlohmann::ordered_json>& v,
                                                nlohmann::ordered_json& out)
    {
        if (std::holds_alternative<std::string>(v)) return std::get<std::string>(v);

        out = std::get<nlohmann::ordered_json>(v);
        return std::nullopt;
    }

    inline std::optional<std::string> ResolveIncludes(nlohmann::ordered_json& node,
                                                      const std::filesystem::path& basePath,
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
                    if (entry.is_string()) includes.push_back(entry.get<std::string>());
                    else if (entry.is_array())
                        for (auto& v: entry) includes.push_back(v.get<std::string>());
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

                        nlohmann::ordered_json included;
                        auto raw = _LoadJsonFile(canon);
                        if (auto err = Propagate(raw, included)) return err;

                        if (auto err = ResolveIncludes(included, canon.parent_path(), loaded)) return err;

                        DeepMerge(node, included);
                    }

                    didInclude = true;
                }

                // -------- include --------
                if (node.contains("include"))
                {
                    auto entry = node["include"];
                    node.erase("include");

                    std::vector<std::string> includes;
                    if (entry.is_string()) includes.push_back(entry.get<std::string>());
                    else if (entry.is_array())
                        for (auto& v: entry) includes.push_back(v.get<std::string>());
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

                        nlohmann::ordered_json included;
                        auto raw = _LoadJsonFile(canon);
                        if (auto err = Propagate(raw, included)) return err;

                        if (auto err = ResolveIncludes(included, canon.parent_path(), loaded)) return err;

                        node[canon.stem().string()] = included;
                    }

                    didInclude = true;
                }

                if (!didInclude) break;
            }

            // recurse into children
            for (auto& [_, v]: node.items())
            {
                if (auto err = ResolveIncludes(v, basePath, loaded)) return err;
            }
        }
        // ---- array ----
        else if (node.is_array())
        {
            for (auto& element: node)
            {
                if (auto err = ResolveIncludes(element, basePath, loaded)) return err;
            }
        }

        return std::nullopt;
    }

#include <iostream>

    inline std::variant<nlohmann::ordered_json, std::string> _LoadJsonFile(const std::filesystem::path& path,
                                                                           std::unordered_set<std::string>& loaded)
    {
        std::filesystem::path file_path = path.is_relative() ? std::filesystem::current_path() / path : path;

        if (!std::filesystem::exists(file_path)) return "File not found: " + file_path.string();

        std::string canonical = std::filesystem::canonical(file_path).string();
        if (loaded.contains(canonical)) return "Circular include detected: " + canonical;

        loaded.insert(canonical);

        nlohmann::ordered_json result;
        try
        {
            result = _LoadJsonFile(file_path);
            auto includeResult = ResolveIncludes(result, file_path.parent_path(), loaded);

            if (includeResult.has_value()) { return includeResult; }

            return result;
        }
        catch (const std::exception& e)
        {
            return std::string("JSON error in ") + file_path.string() + ": " + e.what();
        }
    }

    inline std::variant<nlohmann::ordered_json, std::string> Load(const std::filesystem::path& path,
                                                                  bool debugConsoleDump = false)
    {
        std::unordered_set<std::string> loaded;

        auto result = _LoadJsonFile(path, loaded);

        if (debugConsoleDump && std::holds_alternative<nlohmann::ordered_json>(result))
        {
            std::cout << std::get<nlohmann::ordered_json>(result).dump(4) << std::endl;
        }

        return result;
    }
}// namespace JsonPlus