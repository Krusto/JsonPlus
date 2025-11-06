#pragma once

#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <unordered_set>
#include <filesystem>
#include <variant>

namespace JsonPlus
{

    inline nlohmann::json _LoadJsonFile(const std::filesystem::path& path)
    {
        std::filesystem::path full_path = path;
        if (path.is_relative()) { full_path = std::filesystem::current_path() / std::filesystem::relative(path); }

        if (!std::filesystem::exists(full_path))
        {
            std::cout << "File not found: " << full_path.string() << "\n";
            return nlohmann::json{};
        }

        std::ifstream file(full_path);
        if (!file.is_open())
        {
            std::cout << "Failed to open: " << path << "\n";
            return nlohmann::json{};
        }

        try
        {
            nlohmann::json j;
            file >> j;
            return j;
        }
        catch (const std::exception& e)
        {
            std::cout << "Error parsing " << path << ": " << e.what() << "\n";
            return nlohmann::json{};
        }
    }

    inline std::variant<nlohmann::json, std::string> _LoadJsonFile(const std::filesystem::path& path,
                                                                   std::unordered_set<std::string>& loaded)
    {
        std::filesystem::path file_path = path.is_relative() ? std::filesystem::current_path() / path : path;

        if (!std::filesystem::exists(file_path)) return "File not found: " + file_path.string();

        std::string canonical = std::filesystem::canonical(file_path).string();
        if (loaded.contains(canonical)) return "Circular include detected: " + canonical;

        loaded.insert(canonical);

        nlohmann::json result;
        try
        {
            result = _LoadJsonFile(file_path);
        }
        catch (const std::exception& e)
        {
            return std::string("JSON error in ") + file_path.string() + ": " + e.what();
        }

        // --- Handle "include" key ---
        if (result.contains("include"))
        {
            auto includeEntry = result["include"];
            result.erase("include");

            std::vector<std::string> includes;
            if (includeEntry.is_string()) includes.push_back(includeEntry.get<std::string>());
            else if (includeEntry.is_array())
                for (auto& v: includeEntry) includes.push_back(v.get<std::string>());

            for (const auto& inc: includes)
            {
                std::filesystem::path includePath = file_path.parent_path() / inc;
                auto included_result = _LoadJsonFile(includePath, loaded);

                if (std::holds_alternative<std::string>(included_result))
                    return std::get<std::string>(included_result);// Propagate error

                std::string key = includePath.stem().string();
                result[key] = std::get<nlohmann::json>(included_result);
            }
        }

        // --- Recursively process nested objects ---
        for (auto& [key, value]: result.items())
        {
            if (value.is_object())
            {
                // Recurse into subobjects that might have their own "include"
                std::unordered_set<std::string> subLoaded = loaded;
                auto subVariant = _LoadJsonFile(file_path.parent_path() / (key + ".nlohmann::json"), subLoaded);

                if (std::holds_alternative<nlohmann::json>(subVariant)) value = std::get<nlohmann::json>(subVariant);
            }
        }

        return result;
    }

    inline std::variant<nlohmann::json, std::string> Load(const std::filesystem::path& path)
    {
        std::unordered_set<std::string> loaded;
        return _LoadJsonFile(path, loaded);
    }
}// namespace JsonPlus